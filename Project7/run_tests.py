#!/usr/bin/env python3
import subprocess
from subprocess import call, check_call, check_output
import os.path
from os.path import exists
import functools
import os
import glob
import sys
import itertools
import re
import argparse
import contextlib
import collections

PROJECT_EXECUTABLE = "tube7"
REFERENCE_EXECUTABLE = "reference_" + PROJECT_EXECUTABLE
TUBECODE_PATH = "../TubeCode/tubecode"
TUBEIC_PATH = "../TubeCode/TubeIC"
TEST_SUITE_FOLDER = "Test_Suite"
TEST_FILES = ["Test_Suite/good*.tube", "Test_Suite/fail*.tube"]
EXTRA_CREDIT_TEST_FILES = ["Test_Suite/extra*.tube"]
TEST_TIMEOUT = 5
MAKE_TIMEOUT = 20

def error(message):
    print(message, file=sys.stderr)
    exit(1)

class TestResult(Exception): pass

class TestFailed(TestResult): pass

class TestPassed(TestResult): pass

class InternalTestSuiteException(Exception):
    def __init__(self, message):
        super(InternalTestSuiteException, self).__init__(message)
        long_message = """
Test Suite or Script is broken:
{}
Contact instructors on Piazza so they can fix it (entirely their fault).
Please include this message and stack trace to assist debugging.""".format(message)
        self.args = [long_message]


def rm(path):
    try:
        os.remove(path)
    except OSError:
        pass

def make_executable():
    call_and_get_output(["make", "clean"], timeout=MAKE_TIMEOUT)
    rm(PROJECT_EXECUTABLE)
    try:
        call_and_get_output(["make", PROJECT_EXECUTABLE], timeout=MAKE_TIMEOUT)
    except Exception as e:
        raise TestFailed("make failed")
    if not exists(PROJECT_EXECUTABLE):
        raise TestFailed("make did not create " + PROJECT_EXECUTABLE)


def check_needed_files():
    if not exists("example.tube"):
        raise TestFailed("example.tube doesn't exist")
    if not exists("Makefile"):
        raise TestFailed("Makefile doesn't exist")

    readme_files = glob.glob("README*")
    if not readme_files:
        raise TestFailed("README file doesn't exist")
    if len(readme_files) > 1:
        raise TestFailed("Only one README file allowed") 


def call_and_get_output(args, timeout=None):
    try:
        output = check_output(args, stderr=subprocess.STDOUT,
                              universal_newlines=True, timeout=timeout)
        returncode = 0
    except subprocess.CalledProcessError as cpe:
        output = cpe.output
        returncode = cpe.returncode
    except subprocess.TimeoutExpired as te:
        output = ["Command: " + " ".join(args) + " took too long.\n",
                  "Process took longer than " + str(te.timeout) + " seconds. Killing.\n",
                  "Failed (execution took too long)"]
        raise TestFailed(output)
    except OSError as ose:
        output = ["Command: " + " ".join(args) + " likely outputs non-ascii characters",
                  "Failed (output non-ascii character)"]
        raise TestFailed(output)
        
    return output, returncode

def clean_up(paths):
    def wrap(func):
        @functools.wraps(func) 
        def wrapped(*args, **kwargs):
            try:
                result = func(*args, **kwargs)
            except Exception as e:
                for path in paths:
                    rm(path)
                raise e
            return result
        return wrapped
    return wrap


@clean_up(["ref.tca", "stu.tca", "ref.tic", "stu.tic"])
def run_test(test_file_path, compiler_flags=None, ic_only=False):

    @contextlib.contextmanager
    def add_lines_to_TestResult(lines):
        try:
            yield
        except TestResult as tr:
            tr.args = (lines + tr.args[0],)
            raise tr

    def check_reference_compiler_response(ref_said_error, ref_returncode):
        if ((ref_returncode and not ref_said_error) or 
            (not ref_returncode and ref_said_error)):
            message = "Reference Compiler returncode doesn't match error message.\n"
            message += "Returncode: {}\nOutput contained word \"error\": {}\n".format(
                ref_returncode, ref_said_error) 
            raise InternalTestSuiteException(message)

    def check_if_errors_were_said(ref_said_error, stu_said_error):
        if ref_said_error:
            if stu_said_error:
                raise TestPassed(["Passed (both reference and student compilers raise error)"])
            else:
                raise TestFailed(["Failed (student compiler doesn't raise needed error)"])
        elif stu_said_error:
            raise TestFailed(["Failed (student compiler raised an error needlessly)"])
    
    def check_if_files_created(ic_only):
        if ic_only:
            ref_file = 'ref.tic'
            stu_file = 'stu.tic'
        else:
            ref_file = 'ref.tca'
            stu_file = 'stu.tca'

        if not exists(ref_file):
            raise InternalTestSuiteException("Reference Compiler didn't make a {} file".format(ref_file))

        if not exists(stu_file):
            raise TestFailed(["Falied (student compiler didn't create {} file)".format(stu_file)])

    def test_output(ic_only):
        if ic_only:
            executable = TUBEIC_PATH
            extension = '.tic'
        else:
            executable = TUBECODE_PATH
            extension = '.tca'

        ref_output, ref_returncode = call_and_get_output([executable, "ref" + extension], 
                                                         timeout=TEST_TIMEOUT)
        stu_output, stu_returncode = call_and_get_output([executable, "stu" + extension], 
                                                         timeout=TEST_TIMEOUT)
        lines = ["Reference Execution Output:", ref_output,
                  "Student Execution Output:", stu_output]

        if ref_returncode:
            raise InternalTestSuiteException(
                "TubeCode breaks when executing Reference {}".format(extension))
        if stu_returncode:
            raise TestFailed(lines + 
                             ["Failed (student {} caused error in execution)".format(extension)])
        if ref_output != stu_output:
            raise TestFailed(lines + 
                             ["Failed (student and reference {} execution output differs)".format(
                        extension)])

        raise TestPassed(lines + 
                         ["Passed (Reference and Student {}s have same tubecode output)".format(
                    extension)])


    def run_compilers(ref_args, stu_args):
        ref_output, ref_returncode = call_and_get_output(ref_args, timeout=TEST_TIMEOUT)
        stu_output, stu_returncode = call_and_get_output(stu_args, timeout=TEST_TIMEOUT)

        lines = ["Reference Compiler Output:", ref_output,
                 "Student Compiler Output:", stu_output]
    
        contains_error = re.compile("ERROR", re.IGNORECASE)
        ref_said_error = bool(contains_error.search(ref_output))
        stu_said_error = bool(contains_error.search(stu_output) or stu_returncode)    

        with add_lines_to_TestResult(lines): 
            check_reference_compiler_response(ref_said_error, ref_returncode)
            check_if_errors_were_said(ref_said_error, stu_said_error)
        return lines
    
    def get_ref_stu_args(test_file_path, compiler_flags, ic_only):

        ref_exe_path = os.path.join(TEST_SUITE_FOLDER, REFERENCE_EXECUTABLE)
        stu_exe_path = os.path.join(".", PROJECT_EXECUTABLE)
        common_args = compiler_flags if compiler_flags else []
        common_args.append(test_file_path)

        if ic_only:
            extension = '.tic'
        else:
            extension = '.tca'
        ref_args = [ref_exe_path] + common_args + ["ref" + extension]
        stu_args = [stu_exe_path] + common_args + ["stu" + extension]    

        if ic_only:
            ref_args.insert(1, '-ic')
            stu_args.insert(1, '-ic')

        return ref_args, stu_args


    lines = ["Testing: " + test_file_path]
    ref_args, stu_args = get_ref_stu_args(test_file_path, compiler_flags, ic_only)
        
    with add_lines_to_TestResult(lines):
        lines += run_compilers(ref_args, stu_args)
        check_if_files_created(ic_only)
        test_output(ic_only)

def test_files_in_order(test_globs):
    for test_glob in test_globs:
        test_files = sorted(glob.glob(test_glob))
        for test_file in test_files:
            yield test_file

def get_test_result(test_file_path, compiler_flags, ic_only):
    try:
        run_test(test_file_path, compiler_flags, ic_only)
    except TestFailed as tf:
        passed = False
        output = tf.args[0]
    except TestPassed as tp:
        passed = True
        output = tp.args[0]
    else:
        message = "No TestResult Exception Raised On Test: {}".format(test_file_path)
        raise InternalTestSuiteException(message)
    return passed, output

TestOutcome = collections.namedtuple('TestOutcome', ['file', 'passed', 'output'])

def print_summary(outcomes):
    for outcome in outcomes:
        print("{:<40} {}".format(outcome.file, outcome.output[-1]))
    total_passed = sum(map(lambda outcome: outcome.passed, outcomes)) 
    print("Passed: {} of {}".format(total_passed, len(outcomes)))

def print_first_failure(outcomes):
    try:
        first_failure = next(itertools.dropwhile(
                lambda outcome: outcome.passed, outcomes))
    except StopIteration as si:
        print("No Failures To Display!")
    else:
        print("\n".join(first_failure.output))

def run_tests(test_globs, compiler_flags=None, test_runner=run_test, ic_only=False):
    outcomes = []
    for test_file_path in test_files_in_order(test_globs):
        passed, output = get_test_result(test_file_path, compiler_flags, ic_only=ic_only)
        outcome = TestOutcome(test_file_path, passed, output)
        outcomes.append(outcome)
    return outcomes
            
def run_tests_and_print_output(title, tests, ic_only):
    print(title)
    sys.stdout.flush()
    outcomes = run_tests(tests, ic_only=ic_only)
    print_summary(outcomes)
    return outcomes

def get_cmd_args():
    parser = argparse.ArgumentParser(description="""
Tests the student compiler against the reference compiler.
By default it tests all files in the Test_Suite and shows detailed output for the first failure.
""")
    parser.add_argument('-ic', 
                        help="Runs the tests using TubeIC instead of tubecode.", 
                        action="store_true")
    parser.add_argument('test_files', nargs=argparse.REMAINDER,
                        help="""
Add test files (tube source code) to test specific files like example.tube.
""")
    parser.add_argument('--run-machine-mode', action="store_true", help="""
Outputs the test result as 1's and 0's. For instructor use.
""")
    return parser.parse_args()



def machine_mode():
    def print_outcomes(outcomes):
        lines = []
        for outcome in outcomes:
            file_basename = os.path.basename(outcome.file)
            passed = 1 if outcome.passed else 0
            lines.append("{}, {}".format(file_basename, passed))
        print("\n".join(lines))

    def get_all_outcomes():
        all_outcomes = []


        try:
            check_needed_files()
        except TestFailed as tf:
            check_files_outcome = TestOutcome("has_needed_files", False, "")
        else:
            check_files_outcome = TestOutcome("has_needed_files", True, "")
        all_outcomes.append(check_files_outcome)


        try:
            make_executable()
        except TestFailed as tf:
            make_outcome = TestOutcome("make_worked", False, "")
        else:
            make_outcome = TestOutcome("make_worked", True, "")
        all_outcomes.append(make_outcome)

        tic_outcomes = run_tests(TEST_FILES + EXTRA_CREDIT_TEST_FILES, ic_only=True)
        tic_renamed_outcomes = list(map(
            lambda o: TestOutcome(o.file + "_tic_only", o.passed, o.output),
            tic_outcomes))

        tca_outcomes = run_tests(TEST_FILES + EXTRA_CREDIT_TEST_FILES)

        all_outcomes += tic_renamed_outcomes + tca_outcomes
        return all_outcomes

    outcomes = get_all_outcomes()
    print_outcomes(outcomes)



def normal_mode(args):
    try:
        check_needed_files()
        print("Making compiler...")
        sys.stdout.flush()
        make_executable()
    except TestFailed as tf:
        error(tf.args[0])


    def passed_all(outcomes):
        return all(map(lambda outcome: outcome.passed, outcomes))        

    if len(args.test_files):
        outcomes = run_tests_and_print_output("Testing Specific Tests:", args.test_files, args.ic)
    else:
        outcomes = run_tests_and_print_output("Testing Regular Tests:", TEST_FILES, args.ic)
        #run_tests_and_print_output("Testing Extra Credit Tests:", 
        #                           EXTRA_CREDIT_TEST_FILES, args.ic)

    if passed_all(outcomes):
        print("Passes all tests!")
        exit(0)
    else:
        print("\nFirst Failure's Details:")
        print_first_failure(outcomes)
        exit(1)


def main():
    args = get_cmd_args()
    if args.run_machine_mode:
        machine_mode()
    else:
        normal_mode(args)

if __name__ == "__main__":
    main()
