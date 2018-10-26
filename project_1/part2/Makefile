UNAME := $(shell uname)
HOSTNAME := $(shell hostname)
ifeq ($(UNAME), Darwin)
CCC=mpic++ 
CCFLAGS= -O3 -std=c++11 
CCLIBS= -lboost_filesystem -lboost_iostreams
endif
ifeq ($(HOSTNAME), ix-dev)
CCC= mpic++
CCFLAGS= -O3 -std=c++11  
CCLIBS= -lboost_filesystem -lboost_iostreams
endif

RM = /bin/rm -f

project1: project1.o funcs.o parser.o
	${CCC} ${CCFLAGS} project1.o funcs.o parser.o -o project1 ${CCLIBS} 

project1.o: project1.cpp
	${CCC} ${CCFLAGS} -c project1.cpp 
funcs.o: funcs.cpp funcs.h
	${CCC} ${CCFLAGS} -c funcs.cpp 
parser.o: parser.cpp
	${CCC} -std=c++0x -g -O3 -isystem -march-native -c parser.cpp 

clean:
	/bin/rm -f *.o project1
