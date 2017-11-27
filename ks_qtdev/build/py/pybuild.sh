#!/bin/bash
python np_setup.py -d $1 build_ext -i &> pybuild.log

 if grep -q 'error:' pybuild.log; then
   cat pybuild.log
 fi

if grep -q 'Error:' pybuild.log; then
   cat pybuild.log
 fi
