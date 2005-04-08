
#
# register a simple libasync callback
#

import time;

def print_i(i):
    print time.ctime (time.time ())
    print "i:", i

def print_j (): 
    print j

def foo (i):
    print time.ctime (time.time ())
    print "i: ", i
    example1.delaycb (1, lambda : foo (i+1))


import example1;

foo (0)

example1.amain ()

