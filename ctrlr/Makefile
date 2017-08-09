#############################################################################
#
# Makefile for librf24 examples on Linux
#
# License: GPL (General Public License)
# Author:  gnulnulf <arco@appeltaart.mine.nu>
# Date:    2013/02/07 (version 1.0)
#
# Description:
# ------------
# use make all and make install to install the examples
#

include Makefile.inc

PROGRAMS = main

SOURCES = $(PROGRAMS:=.cpp)

LIBS=-l$(LIB)
ifeq ($(DRIVER), LittleWire)
	LIBS+= -llittlewire-spi
endif

all: $(PROGRAMS)

$(PROGRAMS): $(SOURCES)
	$(CXX) $(CFLAGS) -I$(HEADER_DIR)/.. -I.. -L$(LIB_DIR) $@.cpp $(LIBS) -o $@

clean:
	@echo "[Cleaning]"
	rm -rf $(PROGRAMS)

