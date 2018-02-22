#!/bin/bash

python np_setup.py --trlibdir $1 \
                   --trincdir $2 \
                   --evlibdir $3 \
                   --evincdir $4 \
                   build_ext -i &> pybuild.log

if grep -q 'error:' pybuild.log; then
   cat pybuild.log
fi

if grep -q 'Error:' pybuild.log; then
   cat pybuild.log
fi

if grep -q 'usage:' pybuild.log; then
   cat pybuild.log
fi
