#! /bin/bash

# This file can be executed by calling "bash run_tests.sh"
# It will then test tube1 against the reference_implementation for
# each tube file in the test-suite
project=tube6
make clean
if [ -f $project ]; then
    rm $project;
fi;

make
chmod a+x Test_Suite/reference_$project
if [ ! -f $project ]; then
    echo $project "not correctly compiled";
    exit 1;
fi;
if [ ! -f example.tube ]; then
    echo "example.tube doesn't exist";
    exit 1;
fi;

function run_error_test {
    ./$project $1 $project.tca > $project.cout;
    Test_Suite/reference_$project $1 ref.tca > ref.cout;
    grep -qi "ERROR" $project.cout
    student=$?
    grep -qi "ERROR" ref.cout
    ref=$?
    rm $project.cout ref.cout;
    if [ $student -ne $ref ]; then
	echo $1 " failed to raise ERROR or raised ERROR wrongly";
	rm $project.tca ref.tca 2> /dev/null;
	exit 1;
    fi;
    if [ $ref -eq 0 ]; then
	echo $1 " passed by correctly raising error";
	rm $project.tca ref.tca 2> /dev/null;
	return 0;
    fi;


    ../TubeCode/tubecode $project.tca > $project.out;
    ../TubeCode/tubecode ref.tca > ref.out;
    diff -w ref.out $project.out;
    result=$?;
    rm $project.out ref.out 2> /dev/null
    rm $project.tca ref.tca 2> /dev/null
    if [ $result -ne 0 ]; then
	echo $1 " failed different executed result on tubecode";
	exit 1;
    else
	echo $1 " passed with correct executed result on tubecode";
	return 0;
    fi;
}

function run_extra_project6_error_test {
    ./$project -d $1 $project.tca > $project.cout;
    Test_Suite/reference_$project -d $1 ref.tca > ref.cout;
    grep -qi "ERROR" $project.cout
    student=$?
    grep -qi "ERROR" ref.cout
    ref=$?
    rm $project.cout ref.cout;
    if [ $student -ne $ref ]; then
	echo $1 " failed to raise ERROR or raised ERROR wrongly";
	rm $project.tca ref.tca 2> /dev/null;
	exit 1;
    fi;
    if [ $ref -eq 0 ]; then
	echo $1 " passed by correctly raising error";
	rm $project.tca ref.tca 2> /dev/null;
	return 0;
    fi;


    ../TubeCode/tubecode $project.tca > $project.out;
    ../TubeCode/tubecode ref.tca > ref.out;

    grep -qi "ERROR" $project.out
    student=$?
    grep -qi "ERROR" ref.out
    ref=$?

    rm $project.out ref.out 2> /dev/null;
    rm $project.tca ref.tca 2> /dev/null;
    if [ $student -ne $ref ]; then
	echo $1 " failed to raise ERROR or raised ERROR wrongly in executed tubecode";
	exit 1;
    else
	echo $1 " passed with correct result with executed tubecode";
	return 0;
    fi;
}

if [ $# -gt 0 ]
  then
    for file in "$@"
    do
      run_error_test $file
    done
  exit 1;
fi

for F in Test_Suite/good*.tube; do
  run_error_test $F
done

for F in Test_Suite/fail*.tube; do
  run_error_test $F
done

echo Extra Credit Results:

for F in Test_Suite/extra*.tube; do
  run_extra_project6_error_test $F
done