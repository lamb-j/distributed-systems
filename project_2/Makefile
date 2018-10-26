UNAME := $(shell uname)
HOSTNAME := $(shell hostname)
ifeq ($(UNAME), Darwin)
CCC=g++ 
CCFLAGS= -O3 -std=c++11 
CCLIBS= 
endif
ifeq ($(HOSTNAME), ix-dev)
CCC= g++
CCFLAGS= -O3 -std=c++11  
CCLIBS= 
endif

RM = /bin/rm -f

ring:  funcs.o ring.o
	${CCC} ${CCFLAGS} funcs.o ring.o -o ring ${CCLIBS} 

funcs.o: funcs.cpp
	${CCC} ${CCFLAGS} -c funcs.cpp 

clean:
	/bin/rm -f *.o ring 
