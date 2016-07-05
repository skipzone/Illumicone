
# we can use some sort of wildcard to get a list of all cpp sources
# and convert them to .o
PROGS=PatternController
OBJS=RotaryWidget.o StairWidget.o EyeWidget.o ThreeWheelWidget.o TwistPattern.o SolidBlackPattern.o RgbVerticalPattern.o QuadSlicePattern.o WidgetChannel.o
LIBS=

INCLUDE=../include

# using g++ for CC so that g++ will be used to link C++ programs.
CC=g++
CFLAGS=-Wall -g -I$(INCLUDE)
CXX=g++
CXXFLAGS=-Wall -g -std=c++0x -I$(INCLUDE)
#LDFLAGS=-stdlib=libc++
#LDLIBS=-L$(LIBS) -lpthread

all: $(PROGS)

# dependencies of all the object files
$(OBJS) : $(INCLUDE)/illumiconeTypes.h

$(PROGS): $(OBJS)

# can use something like this if the implicit linking rule is not sufficient.
#	$(CXX) $(LDFLAGS) $(LIBS) -o $@ $< $(LDLIBS)

WidgetSimulator: WidgetSimulator.cpp
	g++ $(CXXFLAGS) -o $@ $^ -lrt

GpioTester: GpioTester.cpp
	g++ $(CXXFLAGS) -I$(INCLUDE) -o $@ $^

clean:
	rm -f $(OBJS) $(PROGS)

clean-GpioTester:
	rm -f GpioTester

clean-WidgetSimulator:
	rm -f WidgetSimulator
