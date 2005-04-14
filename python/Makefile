
CXX =		g++
#CXX =		/usr/local/gcc33/bin/g++
PRFX =		/usr
SFSTAG =	shdbg
RPCC =		/disk/max/d/sfslite/rpcc/rpcc


PYINC =		-I$(PRFX)/include/python2.3
SFSINCDIR =	/usr/local/include/sfslite/$(SFSTAG)
SFSLIBDIR =	/usr/local/lib/sfslite/shdbg

SFSINC =	-I$(SFSINCDIR) -I/usr/local/include

SFSLIB = 	-L$(SFSLIBDIR) -lasync -L$(SFSLIBDIR) -larpc \
		-L$(SFSLIBDIR) -lsfscrypt -lgmp \
		-Xlinker -rpath $(SFSLIBDIR)

default:	example5.so

example0: example0.i example0.C
	swig -c++ -python example0.i
	$(CXX) -g $(PYINC) -c example0.C
	$(CXX) -g $(PYINC) -c example0_wrap.cxx
	$(CXX) -shared example0.o example0_wrap.o -o _example0.so

example1: example1.C
	$(CXX) -g $(PYINC) $(SFSINC) -c example1.C
	$(CXX) -shared example1.o $(SFSLIB) -o example1.so

example3: example3.x
	rpcc -c -o example3.C example3.x
	rpcc -h -o example3.h example3.x
	rpcc -python -o example3.py example3.x

noddy3.so:	noddy3.o
	gcc -shared -o noddy3.so noddy3.o
noddy3.o:	noddy3.c
	gcc -c -g $(PYINC) -o noddy3.o noddy3.c

noddy2.so:	noddy2.o
	gcc -shared -o noddy2.so noddy2.o
noddy2.o:	noddy2.c
	gcc -c -g $(PYINC) -o noddy2.o noddy2.c

example5.C:	example5.x $(RPCC)
	$(RPCC) -q -o example5.C example5.x

.C.o:
	$(CXX) -g $(PYINC) $(SFSINC) -o $@ -c $< 

example5.so:	example5.o py_rpctypes.o
	$(CXX) -shared example5.o py_rpctypes.o $(SFSLIB) -o example5.so


clean:	
	rm -f *.o *.pyc *.so *.core *_wrap.cxx
