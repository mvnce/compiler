#!/usr/bin/env python3
import subprocess
from subprocess import call, check_call, check_output
import os.path
from os.path import exists
import glob
import sys
import itertools
import re
import argparse

PROJECT_EXECUTABLE = "tube6"
REFERENCE_EXECUTABLE = "reference_" + PROJECT_EXECUTABLE
TUBECODE_PATH = "../TubeCode/tubecode"
TEST_SUITE_FOLDER = "Test_Suite"
TEST_FILES = ["Test_Suite/good*.tube", "Test_Suite/fail*.tube"]
EXTRA_CREDIT_TEST_FILES = ["Test_Suite/extra*.tube"]
TESTING_OUTPUT = None

def error(message):
    print(message, file=sys.stderr)
    exit(1)

def build_executable():
    call(["make", "clean"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    if exists(PROJECT_EXECUTABLE):
        call(["rm", PROJECT_EXECUTABLE])

    try:
        check_call(["make", PROJECT_EXECUTABLE], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    except subprocess.CalledProcessError as cpe:
        error("make " + PROJECT_EXECUTABLE + " failed")

    if not exists(PROJECT_EXECUTABLE):
        error("make did not create " + PROJECT_EXECUTABLE)

def check_needed_files():
    if not exists("example.tube"):
        error("example.tube doesn't exist")

    readme_files = glob.glob("README*")
    if not readme_files:
        error("README file doesn't exist")
    if len(readme_files) > 1:
        error("Only one README file allowed") 


def call_and_get_output(args, timeout=None):
    try:
        output = check_output(args, stderr=subprocess.STDOUT,
                              universal_newlines=True, timeout=timeout)
        returncode = 0
    except subprocess.CalledProcessError as cpe:
        output = cpe.output
        returncode = cpe.returncode
    except subprocess.TimeoutExpired as te:
        output = "Process took longer than " + str(te.timeout) + " seconds. Killing.\n"
        output += te.output
        returncode = 1
    return output, returncode

def reference_compiler_problem(message):
    lines = ["", "Reference Compiler Broken", message,
             "Contact instructors so they can fix it (entirely their fault)"]
    print("\n".join(lines), file=sys.stderr)
    exit(1)

def run_test(test_file_path, compiler_flag=""):

    def clean_up_files(file_names):
        for name in file_names:
            if exists(name):
                call(["rm", name])
                
    test_output_lines = ["Testing: " + test_file_path]
    ref_exe_path = os.path.join(TEST_SUITE_FOLDER, REFERENCE_EXECUTABLE)
    stu_exe_path = os.path.join(".", PROJECT_EXECUTABLE)

    ref_args = [ref_exe_path, test_file_path, "ref.tca"]
    stu_args = [stu_exe_path, test_file_path, "stu.tca"]    

    if compiler_flag:
        ref_args.insert(1, compiler_flag)
        stu_args.insert(1, compiler_flag)
        
    # Run Compilers (Reference and Student)
    ref_output, ref_returncode = call_and_get_output(ref_args)
    stu_output, stu_returncode = call_and_get_output(stu_args, timeout=5)

    test_output_lines += ["Reference Compiler Output:",
                          ref_output,
                          "Student Compiler Output:",
                          stu_output]
    
    contains_error = re.compile("ERROR", re.IGNORECASE)
    ref_error = contains_error.search(ref_output)
    stu_error = contains_error.search(stu_output) or stu_returncode
    
    if (ref_returncode and not ref_error) or (not ref_returncode and ref_error):
        reference_compiler_problem("Reference Compiler returncode doesn't match error message")

    # Check for errors
    passed = None
    if ref_error:
        if stu_error:
            test_output_lines.append("Passed (both reference and student compilers raise error)")
            passed = True
        else:
            test_output_lines.append("Failed (student compiler doesn't raise needed error)")
            passed = False
    elif stu_error:
        test_output_lines.append("Failed (student compiler raised an error needlessly)")
        passed = False

    if passed is not None:
        clean_up_files(["ref.tca", "stu.tca"])
        return passed, test_output_lines
        
    # Both compilers didn't have an error    
    if not exists("ref.tca"):
        reference_compiler_problem("Reference Compiler didn't make a ref.tca file")

    if not exists("stu.tca"):
        clean_up_files(["ref.tca"])
        test_output_lines.append("Falied (student compiler didn't create stu.tca file)")
        return False, test_output_lines
    
    ref_output, ref_returncode = call_and_get_output([TUBECODE_PATH, "ref.tca"])
    stu_output, stu_returncode = call_and_get_output([TUBECODE_PATH, "stu.tca"])
    test_output_lines += ["Reference Execution Output:",
                          ref_output,
                          "Student Execution Output:",
                          stu_output]

    if ref_returncode:
        reference_compiler_problem("TubeCode breaks with executing Reference TCA")

    if stu_returncode:
        clean_up_files(["ref.tca", "stu.tca"])
        test_output_lines.append("Failed (student TCA caused error in execution)")
        return False, test_output_lines

    if ref_output != stu_output:
        clean_up_files(["ref.tca", "stu.tca"])
        test_output_lines.append("Failed (student and reference TCA execution output differs)")
        return False, test_output_lines
        
    test_output_lines.append("Passed (Reference and Student TCAs have same tubecode output)")
    return True, test_output_lines

def test_files_in_order(test_globs):
    for test_glob in test_globs:
        #path = os.path.join(TEST_SUITE_FOLDER, test_glob)
        for test_file in glob.iglob(test_glob):
            yield test_file
        
def run_tests_and_print_tally(test_globs, compiler_flag="", test_runner=run_test):
    pass_list = []
    output_list = []
    for test_file_path in test_files_in_order(test_globs):
        passed, output = run_test(test_file_path, compiler_flag)
        pass_list.append(passed)
        output_list.append(output)
        print(test_file_path + " " + output[-1])
        if not passed:
            global TESTING_OUTPUT
            if not TESTING_OUTPUT:
                TESTING_OUTPUT = output
    total_passed = sum(pass_list)
    total_all = len(pass_list)
    print("Passed: {} / {}".format(total_passed, total_all))
    return all(pass_list), output_list
            
def run_all_tests():
    print("Testing Regular Tests")
    all_reg_tests_passed, output_list = run_tests_and_print_tally(TEST_FILES)
    print()
    print("Testing Extra Credit")
    run_tests_and_print_tally(EXTRA_CREDIT_TEST_FILES, "-d")
    print()
    return all_reg_tests_passed

def run_specific_tests(tests):
    print("Running Particular Tests")
    passed_all, output_list = run_tests_and_print_tally(tests)
    for output in output_list:
        print("\n".join(output))
    
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="""
Tests the student compiler against the reference compiler.
By default it tests all files in the Test_Suite and shows detailed output for the first failure.
""")
    parser.add_argument('test_files', nargs=argparse.REMAINDER,
                        help="Add test files (tube source code) to test specific files like example.tube.")
    args = parser.parse_args()

    build_executable()
    check_needed_files()
    if len(args.test_files):
        run_specific_tests(args.test_files)
        exit(0)
    
    passed_all = run_all_tests()
    if passed_all:
        print("Passes all regular tests!")
    else:
        print("Failed some regular tests.")
    if TESTING_OUTPUT:
        print("First Test Failed:")
        print("\n".join(TESTING_OUTPUT))

    if not passed_all:
        exit(1)
