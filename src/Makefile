#    This file is part of Illumicone.
#
#    Illumicone is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    Illumicone is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with Illumicone.  If not, see <http://www.gnu.org/licenses/>.

# we can use some sort of wildcard to get a list of all cpp sources
# and convert them to .o
PROGS=PatternController
#OBJS=RotaryWidget.o StairWidget.o EyeWidget.o ThreeWheelWidget.o TwistPattern.o SolidBlackPattern.o RgbVerticalPattern.o QuadSlicePattern.o WidgetChannel.o
OBJS=WidgetId.o Widget.o WidgetChannel.o ThreeWheelWidget.o RotaryWidget.o StairWidget.o EyeWidget.o PlungerWidget.o BellsWidget.o FourPlayWidget.o RgbVerticalPattern.o QuadSlicePattern.o SolidBlackPattern.o SparklePattern.o RainbowExplosionPattern.o HorizontalStripePattern.o
LIBS=

INCLUDE=../include

# using g++ for CC so that g++ will be used to link C++ programs.
CC=g++
CFLAGS=-Wall -g -I$(INCLUDE)
CXX=g++
CXXFLAGS=-Wall -g -std=c++0x -pthread -I$(INCLUDE)
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

widgetRcvr: widgetRcvr.cpp WidgetId.o
	g++ $(CXXFLAGS) -I$(INCLUDE) -I/usr/local/include/RF24 -L/usr/local/lib -lrf24 -o $@ $^

widgetTester: widgetTester.cpp
	g++ $(CXXFLAGS) -I$(INCLUDE) -o $@ $^

clean:
	rm -f $(OBJS) $(PROGS)

clean-widgetRcvr:
	rm -f widgetRcvr

clean-widgetTester:
	rm -f widgetTester

clean-GpioTester:
	rm -f GpioTester

clean-WidgetSimulator:
	rm -f WidgetSimulator
