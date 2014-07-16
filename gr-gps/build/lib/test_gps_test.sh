#!/bin/sh
export GR_DONT_LOAD_PREFS=1
export srcdir=/Users/samu/koodit/gr/gr-gps/lib
export PATH=/Users/samu/koodit/gr/gr-gps/build/lib:$PATH
export DYLD_LIBRARY_PATH=/Users/samu/koodit/gr/gr-gps/build/lib:$DYLD_LIBRARY_PATH
export PYTHONPATH=$PYTHONPATH
test-gps 
