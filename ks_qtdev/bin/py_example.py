import sys
import numpy as np

import libkshark_wrapper as ks

ofst, cpu, ts, pid, evt, vis = ks.load_data_file(str(sys.argv[1]))
#trace = ks.trace2matrix("trace.dat")

for i in range(10):
    print 'entry: %i  %i  %i  %i' % (cpu[i], ts[i], pid[i], evt[i])
    #print 'task: %i  %i  %i  %i' % (trace[1][i], trace[2][i], trace[3][i], trace[4][i])

