#! /bin/bash

# This file can be executed by calling "bash run_tests.sh"
# It will then test tube1 against the reference_implementation for
# each tube file in the test-suite
project=tube5
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
    ./$project $1 $project.tic > $project.cout;
    Test_Suite/reference_$project $1 ref.tic > ref.cout;
    grep -qi "ERROR" $project.cout 
    student=$?
    grep -qi "ERROR" ref.cout
    ref=$?
    rm $project.cout ref.cout;
    if [ $student -ne $ref ]; then
	echo $1 " failed to raise ERROR or raised ERROR wrongly";
	rm $project.tic ref.tic 2> /dev/null;
	exit 1;
    fi;
    if [ $ref -eq 0 ]; then
	echo $1 " passed";
	rm $project.tic ref.tic 2> /dev/null;
	return 0;
    fi;


    ../TubeCode/TubeIC $project.tic > $project.out;
    ../TubeCode/TubeIC ref.tic > ref.out;
    diff -w ref.out $project.out;
    result=$?;
    rm $project.out ref.out 2> /dev/null
    rm $project.tic ref.tic 2> /dev/null
    if [ $result -ne 0 ]; then
	echo $1 "failed different executed result on TubeIC";
	exit 1;
    else
	echo $1 "passed";
	return 0;
    fi;
}


for F in Test_Suite/good*.tube; do 
	run_error_test $F
done

for F in Test_Suite/fail*.tube; do 
	run_error_test $F
done


echo Extra Credit Results:

for F in Test_Suite/extra.*.tube; do 
	run_error_test $F
done




