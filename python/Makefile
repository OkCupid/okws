
CXX =		/usr/local/gcc33/bin/g++
PYINC = 	/usr/local/include/python2.3
SFSINC=		-I/usr/local/include/sfslite/dbg33 -I/usr/local/include
SLIBS = 	-L/usr/local/lib/sfslite/dbg33 -lasync \
		-Xlinker -rpath /usr/local/lib/sfslite/dbg33

example0: example0.i example0.C
	swig -c++ -python example0.i
	$(CXX) -g -I$(PYINC) -c example0.C
	$(CXX) -g -I$(PYINC) -c example0_wrap.cxx
	$(CXX) -shared example0.o example0_wrap.o -o _example0.so

example1: example1.C
	$(CXX) -g -I$(PYINC) $(SFSINC) -c example1.C
	$(CXX) -shared example1.o $(SLIBS) -o example1.so

example3: example3.x
	rpcc -c -o example3.C example3.x
	rpcc -h -o example3.h example3.x
	rpcc -python -o example3.py example3.x

noddy3.so:	noddy3.o
	gcc -shared -o noddy3.so noddy3.o
noddy3.o:	noddy3.c
	gcc -c -g -I$(PYINC) -o noddy3.o noddy3.c

noddy2.so:	noddy2.o
	gcc -shared -o noddy2.so noddy2.o
noddy2.o:	noddy2.c
	gcc -c -g -I$(PYINC) -o noddy2.o noddy2.c

example5:	example5.x
	/disk/rael/max/d33/sfs/rpcc/rpcc -q -o example5.C example5.x

clean:	
	rm -f *.o *.pyc *.so *.core *_wrap.cxx
