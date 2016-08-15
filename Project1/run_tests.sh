#! /usr/bin/bash

# This file can be executed by calling "bash run_tests.sh"
# It will then test tube1 against the reference_implementation for
# each tube file in the test-suite
make
if [ ! -f tube1 ]; then 
	echo "tube1 not correctly compiled";
	exit 1;
fi;
chmod a+x Test_Suite/reference_tube1

for F in Test_Suite/*.tube; do 
	diff  <(Test_Suite/reference_tube1 $F) <(./tube1 $F);
	if [ $? -ne 0 ]; then
		echo $F " failed";
		exit 1;
	else
		echo $F " passed";
	fi;
done

