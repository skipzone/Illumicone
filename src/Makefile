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

# Automatic dependency checking method is from http://make.mad-scientist.net/papers/advanced-auto-dependency-generation/
# on 28 April 2019.

# Can build widgetRcvr only if we're on a system that supports the nRF24L01+ radio module.
ifneq (,$(wildcard /usr/local/include/RF24))
  PROGS = logTest patternController stringTester unitTests widgetRcvr
else
  PROGS = logTest patternController stringTester unitTests
endif

SRCS = $(wildcard *.cpp) 

INCLUDE=../include

DEPDIR := .d
$(shell mkdir -p $(DEPDIR) >/dev/null)
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.Td

# using g++ for CC so that g++ will be used to link C++ programs.
CC=g++
CXX=g++
CFLAGS=-Wall -g -I$(INCLUDE)
CXXFLAGS=-Wall -g -std=c++11 -pthread -I$(INCLUDE)

COMPILE.c = $(CC) $(DEPFLAGS) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c
COMPILE.cc = $(CXX) $(DEPFLAGS) $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c
POSTCOMPILE = @mv -f $(DEPDIR)/$*.Td $(DEPDIR)/$*.d && touch $@

%.o : %.c
%.o : %.c $(DEPDIR)/%.d
	$(COMPILE.c) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)

%.o : %.cc
%.o : %.cc $(DEPDIR)/%.d
	$(COMPILE.cc) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)

%.o : %.cxx
%.o : %.cxx $(DEPDIR)/%.d
	$(COMPILE.cc) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)

%.o : %.cpp
%.o : %.cpp $(DEPDIR)/%.d
	$(COMPILE.cc) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)

all: $(PROGS)

jsonTester: jsonTester.o hsv2rgb.o json11.o Log.o illumiconeUtility.o illumiconePixelUtility.o ConfigReader.o WidgetId.o

logTest: logTest.o hsv2rgb.o Log.o illumiconeUtility.o

patternController: patternController.o AnnoyingFlashingPattern.o BatonWidget.o BellsWidget.o BoogieBoardWidget.o ConfigReader.o \
                   ContortOMaticWidget.o EyeWidget.o FillAndBurstPattern.o FourPlay42Widget.o FourPlay43Widget.o \
                   HorizontalStripePattern.o IndicatorRegion.o IndicatorRegionsPattern.o Log.o MidiActivatedRegionsPattern.o \
                   ParticlesPattern.o Pattern.o PumpWidget.o QueuedWidgetChannel.o RainbowExplosionPattern.o RainstickWidget.o \
                   RgbVerticalPattern.o SchroedersPlaythingWidget.o SimpleBlockIndicator.o SparklePattern.o SpinnahWidget.o \
                   SpinnerPattern.o SpiralPattern.o SwitchActivatedRegionsPattern.o TriObeliskWidget.o Widget.o WidgetChannel.o \
                   WidgetId.o colorpalettes.o colorutils.o hsv2rgb.o illumiconePixelUtility.o illumiconeUtility.o \
                   indicatorRegionFactory.o json11.o lib8tion.o patternFactory.o widgetFactory.o

stringTester: stringTester.o hsv2rgb.o json11.o Log.o illumiconeUtility.o illumiconePixelUtility.o ConfigReader.o

unitTests: unitTests.o hsv2rgb.o json11.o Log.o illumiconeUtility.o illumiconePixelUtility.o ConfigReader.o

widgetRcvr: widgetRcvr.o ConfigReader.o hsv2rgb.o illumiconeUtility.o illumiconePixelUtility.o json11.o Log.o WidgetId.o

clean:
	rm -f $(wildcard *.o) $(PROGS)

$(DEPDIR)/%.d: ;
.PRECIOUS: $(DEPDIR)/%.d

include $(wildcard $(patsubst %,$(DEPDIR)/%.d,$(basename $(SRCS))))

