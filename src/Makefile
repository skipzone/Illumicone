
PROGS=hello
OBJS=hello.o
LIBS=

INCLUDE=../include

# using g++ for CC so that g++ will be used to link C++ programs.
CC=g++
CFLAGS=-Wall -g -I$(INCLUDE)
CXX=g++
CXXFLAGS=-Wall -g -std=c++11 -I$(INCLUDE)
#LDFLAGS=-stdlib=libc++
#LDLIBS=-L$(LIBS) -lpthread

all: $(PROGS)

# dependencies of all the object files
$(OBJS) : $(INCLUDE)/illumiconeTypes.h

hello: hello.o

# can use something like this if the implicit linking rule is not sufficient.
#	$(CXX) $(LDFLAGS) $(LIBS) -o $@ $< $(LDLIBS)

clean:
	rm -f $(OBJS) $(PROGS)

