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
PROGS=patternController
OBJS=WidgetId.o widgetFactory.o Widget.o WidgetChannel.o TriObeliskWidget.o SpinnahWidget.o EyeWidget.o RainstickWidget.o PumpWidget.o BellsWidget.o FourPlay42Widget.o FourPlay43Widget.o Pattern.o AnnoyingFlashingPattern.o HorizontalStripePattern.o RainbowExplosionPattern.o RgbVerticalPattern.o SparklePattern.o FillAndBurstPattern.o ParticlesPattern.o json11.o ConfigReader.o log.o illumiconeUtility.o
LIBS=

INCLUDE=../include

# using g++ for CC so that g++ will be used to link C++ programs.
CC=g++
CFLAGS=-Wall -g -I$(INCLUDE)
CXX=g++
###CXXFLAGS=-Wall -g -std=c++0x -pthread -I$(INCLUDE)
CXXFLAGS=-Wall -g -std=c++11 -pthread -I$(INCLUDE)
#LDFLAGS=-stdlib=libc++
#LDLIBS=-L$(LIBS) -lpthread

all: $(PROGS)

# dependencies of all the object files
$(OBJS) : $(INCLUDE)/illumiconeTypes.h $(INCLUDE)/illumiconeUtility.h

$(PROGS): $(OBJS)

# can use something like this if the implicit linking rule is not sufficient.
#	$(CXX) $(LDFLAGS) $(LIBS) -o $@ $< $(LDLIBS)

widgetRcvr: widgetRcvr.cpp WidgetId.o
	g++ $(CXXFLAGS) -I$(INCLUDE) -I/usr/local/include/RF24 -L/usr/local/lib -lrf24 -o $@ $^

stringTester: stringTester.cpp json11.o ConfigReader.o
	g++ $(CXXFLAGS) -I$(INCLUDE) -o $@ $^

jsonTester: jsonTester.cpp json11.o ConfigReader.o WidgetId.o
	g++ $(CXXFLAGS) -I$(INCLUDE) -o $@ $^

clean:
	rm -f $(OBJS) $(PROGS)

clean-widgetRcvr:
	rm -f widgetRcvr

clean-stringTester:
	rm -f stringTester

clean-jsonTester:
	rm -f jsonTester

