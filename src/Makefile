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

# Can build widgetRcvr only if we're on a system that supports the nRF24L01+ radio
# module. Such a system might also want hardware-dependent stuff from sbin, too.
ifneq (,$(wildcard /usr/local/include/RF24))
  PROGS = jsonTester logTest patternController stringTester unitTests widgetRcvr
  INSTALL_PROGS = patternController stringTester widgetRcvr
  LDLIBS = -L/usr/local/lib -lrf24
else
  PROGS = jsonTester logTest patternController stringTester unitTests
  INSTALL_PROGS = patternController stringTester
endif

SRCS = $(wildcard *.cpp) 

SBIN_PATH = ../sbin
INCLUDE=../include

# Usually, we install to ~/bin.
ifeq (,$(DESTDIR))
    ifeq (,$(PREFIX))
        PREFIX := $(HOME)
    endif
endif

DEPDIR := .d
$(shell mkdir -p $(DEPDIR) >/dev/null)
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.Td

# using g++ for CC so that g++ will be used to link C++ programs.
CC=g++
CXX=g++
CFLAGS=-Wall -Wno-unknown-warning-option -g -I$(INCLUDE)
CXXFLAGS=-Wall -Wno-unknown-warning-option -g -std=c++11 -pthread -I$(INCLUDE)

LDFLAGS = -pthread

COMPILE.c = $(CC) $(DEPFLAGS) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c
COMPILE.cc = $(CXX) $(DEPFLAGS) $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c
POSTCOMPILE = @mv -f $(DEPDIR)/$*.Td $(DEPDIR)/$*.d && touch $@

TIMESTAMP = $(shell date "+_%Y%m%d_%H%M%S")

INSTALLMKDIR = install -d
ifeq ("Darwin", "$(shell uname -s)")
	INSTALL = install -bp -B $(TIMESTAMP)
else
	INSTALL = install -bC -S $(TIMESTAMP)
endif

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

.PHONY: all
all: $(PROGS)

jsonTester: jsonTester.o hsv2rgb.o json11.o Log.o illumiconeUtility.o illumiconePixelUtility.o ConfigReader.o WidgetId.o

logTest: logTest.o hsv2rgb.o Log.o illumiconeUtility.o

patternController: patternController.o AnnoyingFlashingPattern.o BatonWidget.o BellsWidget.o BoogieBoardWidget.o ConfigReader.o \
                   EyeWidget.o FillAndBurstPattern.o FlowerWidget.o FourPlay4xWidget.o HorizontalStripePattern.o \
                   IndicatorRegion.o IndicatorRegionsPattern.o LidarWidget.o Log.o MidiActivatedRegionsPattern.o \
                   MikeWidget.o ParticlesPattern.o Pattern.o PumpWidget.o QueuedWidgetChannel.o \
                   RainbowExplosionPattern.o RainstickWidget.o RattleWidget.o RgbStripePattern.o \
                   RgbVerticalPattern.o SchroedersPlaythingWidget.o SimpleBlockIndicator.o \
				   SparklePattern.o SpinnahWidget.o SpinnerPattern.o SpiralPattern.o StripePattern.o \
				   SwitchActivatedRegionsPattern.o Widget.o WidgetChannel.o WidgetId.o colorpalettes.o \
                   colorutils.o hsv2rgb.o illumiconePixelUtility.o illumiconeUtility.o \
                   indicatorRegionFactory.o json11.o lib8tion.o patternFactory.o widgetFactory.o

stringTester: stringTester.o hsv2rgb.o json11.o Log.o illumiconeUtility.o illumiconePixelUtility.o ConfigReader.o

unitTests: unitTests.o hsv2rgb.o json11.o Log.o illumiconeUtility.o illumiconePixelUtility.o ConfigReader.o

widgetRcvr: widgetRcvr.o ConfigReader.o hsv2rgb.o illumiconeUtility.o illumiconePixelUtility.o json11.o Log.o WidgetId.o

.PHONY: install
install:
	@$(INSTALLMKDIR) $(DESTDIR)$(PREFIX)/bin
	@for f in $(INSTALL_PROGS); do \
		$(INSTALL) $$f $(DESTDIR)$(PREFIX)/bin; \
	done

.PHONY: clean
clean:
	rm -f $(wildcard *.o) $(PROGS)

.PHONY: cleandep
cleandep:
	rm -rf $(DEPDIR)

$(DEPDIR)/%.d: ;
.PRECIOUS: $(DEPDIR)/%.d

include $(wildcard $(patsubst %,$(DEPDIR)/%.d,$(basename $(SRCS))))

