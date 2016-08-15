#! /usr/bin/bash

# This file can be executed by calling "bash run_tests.sh"
# It will then test tube1 against the reference_implementation for
# each tube file in the test-suite
if [ -f tube2 ]; then 
        rm tube2
fi;
make clean
make
if [ ! -f tube2 ]; then 
	echo "tube2 not correctly compiled";
	exit 1;
fi;
chmod a+x Test_Suite/reference_tube2

function run_test {
	diff  <(Test_Suite/reference_tube2 $1) <(./tube2 $1);
	if [ $? -ne 0 ]; then
		echo $1 " failed";
		exit 1;
	else
		echo $1 " passed";
	fi;
}

for F in `ls Test_Suite | grep -v 'extra\|reference'`; do 
	run_test Test_Suite/$F
done
echo Extra Credit Results:

for F in Test_Suite/*extra*.tube; do 
	run_test $F
done

