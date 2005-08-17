
import getopt
import sys
from distutils.sysconfig import *
import os.path

mf = get_makefile_filename ()
d = parse_makefile (mf)

optlist, args = getopt.getopt (sys.argv[1:], "IvlcxF")
for o in optlist:
    opt = o[0]
    if opt == '-I':
        print get_python_inc ()
        exit
    elif opt == '-v':
        print get_python_version ()
        exit
    elif opt == '-l':
        dir,_ = os.path.split (mf)
        v = get_python_version ()
        libs = d['LIBS']
        syslibs = d['SYSLIBS']
        print "-L%s -lpython%s %s %s" % (dir, v, libs, syslibs)
    elif opt == '-c':
        print d['CC']
    elif opt == '-x':
        print d['CXX']
    elif opt == '-F':
        print d['LDFLAGS']
    
        
