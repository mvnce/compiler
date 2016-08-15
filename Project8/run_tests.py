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

PROJECT_EXECUTABLE = "tube8"
REFERENCE_EXECUTABLE = "reference_" + PROJECT_EXECUTABLE
TUBECODE_PATH = "../TubeCode/tubecode"
TUBEIC_PATH = "../TubeCode/TubeIC"
TEST_SUITE_FOLDER = "Test_Suite"
TEST_FILES = ["Test_Suite/good*declare*.tube", "Test_Suite/fail*.tube", "Test_Suite/*optim*.tube"]
EXTRA_CREDIT_TEST_FILES = ["Test_Suite/extra*.tube"]
TEST_TIMEOUT = 5
MAKE_TIMEOUT = 20
TEST_TIC_OUTPUT = False
TEST_TCA_OUTPUT = True
OPTIMIZATION_MODE = True #For CSE 450 Project 8

def error(message):
    print(message, file=sys.stderr)
    exit(1)

TestOutcome = collections.namedtuple('TestOutcome', ['file', 'passed', 'output'])

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



def calculate_grade(outcomes, print_output=True):
    def grade_tests(file_token, test_to_passed, weight, proportional=False):
        score = 0
        pattern_tca = re.compile(file_token)
        tca_tests = []
        output = []
        output.append("Grading tests with {} in their name.".format(file_token))
        for test, passed in test_to_passed.items():
            if pattern_tca.search(test):
                tca_tests.append(passed)
        if not proportional:
            if all(tca_tests):
                score = weight
                output.append("All tests passed: {0} of {1} points".format(score, weight))
            else:
                output.append("Not all tests tests passed: {0} of {1} points".format(
                        score, weight))
            return output, score
        number_of_passes = sum(tca_tests)
        number_of_tests = len(tca_tests)
        output.append("Passed {} of {} tests.".format(number_of_passes, number_of_tests))
        score = weight * ( number_of_passes / number_of_tests )
        output.append("Proportional Score is: {} of {} points".format(score, weight))
        return output, score
        
    test_to_passed = {outcome.file : outcome.passed for outcome in outcomes}
    output = ["Grade Calculation (Provisional)"]
    total = 0

    criteria_name = "Pre-test checks for needed files and successful make: "
    has_files = test_to_passed["has_needed_files"]
    make_worked = test_to_passed["make_worked"] 
    if has_files and make_worked:
        total += 1
        output += [criteria_name + "1 of 1 points"]
    else:
        output += [criteria_name + "0 of 1 points"]

    dec_output, dec_score = grade_tests(
        "declare", test_to_passed, weight=1)
    output += dec_output
    total += dec_score

    if dec_score == 1:
        fail_output, fail_score = grade_tests(
            "fail", test_to_passed, weight=1)
        output += fail_output
        total += fail_score
    else:
        output += ["Some of the dependances of the fail tests haven't passed",
                   "So we can't grade the fails.",
                   "You need to pass the tests with declare in their names first.",
                   "0 of 1 points"]

    opt_output, opt_score = grade_tests(
        "optim", test_to_passed, weight=5, proportional=True)
    output += opt_output
    total += opt_score
    
        

    if print_output:
        print("\n".join(output))
        print("Current tentative grade is: {} of 8".format(total)) 
    return total



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
        raise TestFailed(["Failed (make failed)"])
    if not exists(PROJECT_EXECUTABLE):
        raise TestFailed(["Failed (make did not create {})".format(PROJECT_EXECUTABLE)])


def check_needed_files():
    if not exists("example.tube"):
        raise TestFailed(["Failed (example.tube doesn't exist)"])
    if not exists("Makefile"):
        raise TestFailed(["Failed (Makefile doesn't exist)"])

    readme_files = glob.glob("README*")
    if not readme_files:
        raise TestFailed(["Failed (README file doesn't exist)"])
    if len(readme_files) > 1:
        raise TestFailed(["Failed (Only one README file allowed)"]) 


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
        output = ["Command: \"" + " ".join(args) + "\" raised OSError",
                  ose.strerror,
                  "Failed (couldn't run command {})".format(" ".join(args))]
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

    def test_output(ic_only, allowed_cycles=None):
        def get_cycles_and_output_from_output(tubecode_output):
            lines = tubecode_output.split('\n')
            last_line = lines[-2] # ignore blank very last line
            #last line looks like "[[ Total CPU cycles used: 110925 ]]"
            match = re.search(r"\d+", last_line)
            if not match:
                raise InternalTestSuiteException("""
tubecode output doesn't include the number of cycles used
""")
            cycles_str = match.group()
            return int(cycles_str), "\n".join(lines[:-2]) + "\n"

        if allowed_cycles is not None:
            assert not ic_only
            executable_flag = '-c'
        else:
            executable_flag = ''
 
        if ic_only:
            executable = TUBEIC_PATH
            extension = '.tic'
        else:
            executable = TUBECODE_PATH
            extension = '.tca'

        ref_output, ref_returncode = call_and_get_output([executable, executable_flag, "ref" + extension], 
                                                         timeout=TEST_TIMEOUT)
        stu_output, stu_returncode = call_and_get_output([executable, executable_flag, "stu" + extension], 
                                                         timeout=TEST_TIMEOUT)

        lines = ["Reference Execution Output:", ref_output,
                  "Student Execution Output:", stu_output]

        if ref_returncode:
            raise InternalTestSuiteException(
                "TubeCode breaks when executing Reference {}".format(extension))
        if stu_returncode:
            raise TestFailed(lines + 
                             ["Failed (student {} caused error in execution)".format(extension)])


        if allowed_cycles is not None:
            ref_cycles, ref_output = get_cycles_and_output_from_output(ref_output)
            stu_cycles, stu_output = get_cycles_and_output_from_output(stu_output)
            if ref_cycles > allowed_cycles:
                message = "Reference Compiler takes too many cycles on {}".format(test_file_path)
                raise InternalTestSuiteException(message)

        if ref_output != stu_output:
            raise TestFailed(lines + 
                             ["Failed (student and reference {} execution output differs)".format(
                        extension)])

        if allowed_cycles is not None:
            if stu_cycles > allowed_cycles:
                message = ["Student compiler took {} cycles.".format(stu_cycles),
                           "Only allowed to take {} cycles.".format(allowed_cycles),
                           "Failed (student compiler runs for too many cycles ({}))".format(
                        stu_cycles)]
                raise TestFailed(lines + message)
            else:
                raise TestPassed([
                        "Passed (Student has correct tubecode output in only {} cycles)".format(
                            stu_cycles)])


        raise TestPassed(lines + 
                         ["Passed (Reference and Student {}s have same output)".format(
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

        optimization_flag = '-O' if OPTIMIZATION_MODE else ''
        

        ref_args = [ref_exe_path, optimization_flag] + common_args + ["ref" + extension]
        stu_args = [stu_exe_path] + common_args + ["stu" + extension]    

        if ic_only:
            ref_args.insert(1, '-ic')
            stu_args.insert(1, '-ic')

        return ref_args, stu_args

    def get_cycles_allowed(test_file_path):
        base_file = os.path.basename(test_file_path)
        no_ext = base_file.split('.')[0]
        cycles_str = no_ext.split('-')[-1]
        return int(cycles_str)


    lines = ["Testing: " + test_file_path]
    ref_args, stu_args = get_ref_stu_args(test_file_path, compiler_flags, ic_only)
        
    with add_lines_to_TestResult(lines):
        lines += run_compilers(ref_args, stu_args)
        check_if_files_created(ic_only)
        if OPTIMIZATION_MODE:
            cycles = get_cycles_allowed(test_file_path)
        test_output(ic_only, allowed_cycles=cycles)

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
            
def get_cmd_args():
    parser = argparse.ArgumentParser(description="""
Tests the student compiler against the reference compiler.
By default it tests all files in the Test_Suite and shows detailed output for the first failure.
""")
    parser.add_argument('test_files', nargs=argparse.REMAINDER,
                        help="""
Add test files (tube source code) to test specific files like example.tube.
""")
    parser.add_argument('--run-machine-mode', action="store_true", help="""
Outputs the test result as 1's and 0's. For instructor use.
""")
    return parser.parse_args()



def get_all_outcomes(tests):
    all_outcomes = []

    check = try_to_outcome_wrapper("has_needed_files", check_needed_files)
    all_outcomes.append(check())

    make = try_to_outcome_wrapper("make_worked", make_executable)
    all_outcomes.append(make())

    if TEST_TIC_OUTPUT:
        tic_outcomes = run_tests(tests, ic_only=True)
        tic_renamed_outcomes = list(map(
                lambda o: TestOutcome(o.file + "_tic_test", o.passed, o.output),
                tic_outcomes))
        all_outcomes += tic_renamed_outcomes        

    tca_outcomes = run_tests(tests)
    all_outcomes += tca_outcomes
    return all_outcomes


def machine_mode():
    def print_outcomes(outcomes):
        lines = []
        for outcome in outcomes:
            file_basename = os.path.basename(outcome.file)
            passed = 1 if outcome.passed else 0
            lines.append("{}, {}".format(file_basename, passed))
        grade = calculate_grade(outcomes, print_output=False)
        lines.append("{}, {}".format("grade", grade))
        print("\n".join(lines))

    tests = TEST_FILES #+ EXTRA_CREDIT_TEST_FILES
    outcomes = get_all_outcomes(tests)
    print_outcomes(outcomes)


def try_to_outcome_wrapper(test_name, func):
    @functools.wraps(func)
    def wrapper(*args, **kwargs):
        passed = TestOutcome(test_name, True, ["Passed"])
        try:
            func(*args, **kwargs)
        except TestFailed as tf:
            return TestOutcome(test_name, False, tf.args[0])
        except TestPassed as tp:
            return passed
        else:
            return passed
    return wrapper


def normal_mode(args):
    print("Starting Tests...")
    sys.stdout.flush()

    if len(args.test_files):
        tests = args.test_files
    else:
        tests = TEST_FILES

    outcomes = get_all_outcomes(tests)
    
    def passed_all(outcomes):
        return all(map(lambda outcome: outcome.passed, outcomes))        
    
    for outcome in outcomes:
        print("{:<40} {}".format(outcome.file, outcome.output[-1]))
    print()
    calculate_grade(outcomes, print_output=True)
    print()
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
