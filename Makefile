# Makefile for EphemerisCompute
#
# -------------------------------------------------
# Copyright 2015-2026 Dominic Ford
#
# This file is part of EphemerisCompute.
#
# EphemerisCompute is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# EphemerisCompute is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with EphemerisCompute.  If not, see <http://www.gnu.org/licenses/>.
# -------------------------------------------------

CWD=$(shell pwd)

VERSION = 8.0
DATE    = 01/05/2026
PATHLINK= /

WARNINGS           = -Wall -Wno-format-truncation -Wno-unknown-pragmas
COMPILE            = $(CC) $(WARNINGS) -g -c -I $(CWD)/src
LIBS               = -lgsl -lgslcblas -lm
LINK               = $(CC) $(WARNINGS) -g -fopenmp
LINK_SINGLE_THREAD = $(CC) $(WARNINGS) -g

OPTIMISATION = -O3

DEBUG          = -D DEBUG=1 -D MEMDEBUG1=1 -D MEMDEBUG2=0 -fopenmp
NODEBUG        = -D DEBUG=0 -D MEMDEBUG1=0 -D MEMDEBUG2=0 -fopenmp
SINGLE_THREAD  = -D DEBUG=0 -D MEMDEBUG1=0 -D MEMDEBUG2=0

LOCAL_SRCDIR = src
LOCAL_OBJDIR = obj
LOCAL_BINDIR = bin

CORE_FILES = argparse/argparse.c compute.c coreUtils/asciiDouble.c coreUtils/errorReport.c coreUtils/makeRasters.c \
             ephemCalc/constellations.c ephemCalc/magnitudeEstimate.c ephemCalc/meeus.c ephemCalc/jpl.c \
             ephemCalc/orbitalElements.c listTools/ltDict.c listTools/ltList.c listTools/ltMemory.c \
             listTools/ltStringProc.c mathsTools/julianDate.c mathsTools/precess_equinoxes.c mathsTools/sphericalAst.c \
             settings/settings.c

CORE_HEADERS = argparse/argparse.h coreUtils/asciiDouble.h compute.h coreUtils/errorReport.h coreUtils/makeRasters.h \
               coreUtils/strConstants.h ephemCalc/constellations.h ephemCalc/magnitudeEstimate.h ephemCalc/meeus.h \
               ephemCalc/jpl.h ephemCalc/orbitalElements.h listTools/ltDict.h listTools/ltList.h listTools/ltMemory.h \
               listTools/ltStringProc.h mathsTools/julianDate.h mathsTools/precess_equinoxes.h \
               mathsTools/sphericalAst.h settings/settings.h

EPHEM_FILES = main.c
EPHEM_HEADERS =

SOCKET_SERVER_FILES = socket_server.c
SOCKET_SERVER_HEADERS =

SOCKET_CLIENT_FILES = socket_client.c
SOCKET_CLIENT_HEADERS =

ASTEROID_FILES = asteroids.c
ASTEROID_HEADERS =

CORE_SOURCES                         = $(CORE_FILES:%.c=$(LOCAL_SRCDIR)/%.c)
CORE_OBJECTS                         = $(CORE_FILES:%.c=$(LOCAL_OBJDIR)/%.o)
CORE_OBJECTS_DEBUG                   = $(CORE_OBJECTS:%.o=%.debug.o)
CORE_OBJECTS_SINGLE_THREAD           = $(CORE_OBJECTS:%.o=%.single_thread.o)
CORE_HFILES                          = $(CORE_HEADERS:%.h=$(LOCAL_SRCDIR)/%.h) Makefile

EPHEM_SOURCES                        = $(EPHEM_FILES:%.c=$(LOCAL_SRCDIR)/%.c)
EPHEM_OBJECTS                        = $(EPHEM_FILES:%.c=$(LOCAL_OBJDIR)/%.o)
EPHEM_OBJECTS_DEBUG                  = $(EPHEM_OBJECTS:%.o=%.debug.o)
EPHEM_OBJECTS_SINGLE_THREAD          = $(EPHEM_OBJECTS:%.o=%.single_thread.o)
EPHEM_HFILES                         = $(EPHEM_HEADERS:%.h=$(LOCAL_SRCDIR)/%.h) Makefile

SOCKET_SERVER_SOURCES                = $(SOCKET_SERVER_FILES:%.c=$(LOCAL_SRCDIR)/%.c)
SOCKET_SERVER_OBJECTS                = $(SOCKET_SERVER_FILES:%.c=$(LOCAL_OBJDIR)/%.o)
SOCKET_SERVER_OBJECTS_DEBUG          = $(SOCKET_SERVER_OBJECTS:%.o=%.debug.o)
SOCKET_SERVER_OBJECTS_SINGLE_THREAD  = $(SOCKET_SERVER_OBJECTS:%.o=%.single_thread.o)
SOCKET_SERVER_HFILES                 = $(SOCKET_SERVER_HEADERS:%.h=$(LOCAL_SRCDIR)/%.h) Makefile

SOCKET_CLIENT_SOURCES                = $(SOCKET_CLIENT_FILES:%.c=$(LOCAL_SRCDIR)/%.c)
SOCKET_CLIENT_OBJECTS                = $(SOCKET_CLIENT_FILES:%.c=$(LOCAL_OBJDIR)/%.o)
SOCKET_CLIENT_OBJECTS_DEBUG          = $(SOCKET_CLIENT_OBJECTS:%.o=%.debug.o)
SOCKET_CLIENT_OBJECTS_SINGLE_THREAD  = $(SOCKET_CLIENT_OBJECTS:%.o=%.single_thread.o)
SOCKET_CLIENT_HFILES                 = $(SOCKET_CLIENT_HEADERS:%.h=$(LOCAL_SRCDIR)/%.h) Makefile

ASTEROID_SOURCES                     = $(ASTEROID_FILES:%.c=$(LOCAL_SRCDIR)/%.c)
ASTEROID_OBJECTS                     = $(ASTEROID_FILES:%.c=$(LOCAL_OBJDIR)/%.o)
ASTEROID_OBJECTS_DEBUG               = $(ASTEROID_OBJECTS:%.o=%.debug.o)
ASTEROID_OBJECTS_SINGLE_THREAD       = $(ASTEROID_OBJECTS:%.o=%.single_thread.o)
ASTEROID_HFILES                      = $(ASTEROID_HEADERS:%.h=$(LOCAL_SRCDIR)/%.h) Makefile

ALL_HFILES = $(CORE_HFILES) $(EPHEM_HFILES) $(ASTEROID_HFILES)

SWITCHES = -D DCFVERSION=\"$(VERSION)\"  -D DATE=\"$(DATE)\"  -D PATHLINK=\"$(PATHLINK)\"  -D SRCDIR=\"$(CWD)/$(LOCAL_SRCDIR)/\"

all: $(LOCAL_BINDIR)/ephem.bin $(LOCAL_BINDIR)/debug/ephem.bin $(LOCAL_BINDIR)/single_thread/ephem.bin \
     $(LOCAL_BINDIR)/socket_server.bin $(LOCAL_BINDIR)/debug/socket_server.bin $(LOCAL_BINDIR)/single_thread/socket_server.bin \
     $(LOCAL_BINDIR)/socket_client.bin $(LOCAL_BINDIR)/debug/socket_client.bin $(LOCAL_BINDIR)/single_thread/socket_client.bin \
     $(LOCAL_BINDIR)/asteroids.bin $(LOCAL_BINDIR)/debug/asteroids.bin $(LOCAL_BINDIR)/single_thread/asteroids.bin

#
# General macros for the compile steps
#

$(LOCAL_OBJDIR)/%.o:                $(LOCAL_SRCDIR)/%.c $(ALL_HFILES)
	mkdir -p $(LOCAL_OBJDIR) $(LOCAL_OBJDIR)/argparse $(LOCAL_OBJDIR)/coreUtils $(LOCAL_OBJDIR)/ephemCalc $(LOCAL_OBJDIR)/listTools $(LOCAL_OBJDIR)/mathsTools $(LOCAL_OBJDIR)/settings
	$(COMPILE) $(OPTIMISATION) $(NODEBUG) $(SWITCHES) $< -o $@

$(LOCAL_OBJDIR)/%.debug.o:          $(LOCAL_SRCDIR)/%.c $(ALL_HFILES)
	mkdir -p $(LOCAL_OBJDIR) $(LOCAL_OBJDIR)/argparse $(LOCAL_OBJDIR)/coreUtils $(LOCAL_OBJDIR)/ephemCalc $(LOCAL_OBJDIR)/listTools $(LOCAL_OBJDIR)/mathsTools $(LOCAL_OBJDIR)/settings
	$(COMPILE) $(OPTIMISATION) $(DEBUG)   $(SWITCHES) $< -o $@

$(LOCAL_OBJDIR)/%.single_thread.o:  $(LOCAL_SRCDIR)/%.c $(ALL_HFILES)
	mkdir -p $(LOCAL_OBJDIR) $(LOCAL_OBJDIR)/argparse $(LOCAL_OBJDIR)/coreUtils $(LOCAL_OBJDIR)/ephemCalc $(LOCAL_OBJDIR)/listTools $(LOCAL_OBJDIR)/mathsTools $(LOCAL_OBJDIR)/settings
	$(COMPILE) $(OPTIMISATION) $(SINGLE_THREAD)   $(SWITCHES) $< -o $@

#
# Make the ephemeris computer binaries
#

$(LOCAL_BINDIR)/ephem.bin: $(CORE_OBJECTS) $(EPHEM_OBJECTS)
	mkdir -p $(LOCAL_BINDIR)
	$(LINK) $(OPTIMISATION) $(CORE_OBJECTS) $(EPHEM_OBJECTS) $(LIBS) -o $(LOCAL_BINDIR)/ephem.bin

$(LOCAL_BINDIR)/debug/ephem.bin: $(CORE_OBJECTS_DEBUG) $(EPHEM_OBJECTS_DEBUG)
	mkdir -p $(LOCAL_BINDIR)/debug
	echo "The files in this directory are binaries with debugging options enabled: they produce activity logs called 'ephem.log'. It should be noted that these binaries can up to ten times slower than non-debugging versions." > $(LOCAL_BINDIR)/debug/README
	$(LINK) $(OPTIMISATION) $(CORE_OBJECTS_DEBUG) $(EPHEM_OBJECTS_DEBUG) $(LIBS) -o $(LOCAL_BINDIR)/debug/ephem.bin

$(LOCAL_BINDIR)/single_thread/ephem.bin: $(CORE_OBJECTS_SINGLE_THREAD) $(EPHEM_OBJECTS_SINGLE_THREAD)
	mkdir -p $(LOCAL_BINDIR)/single_thread
	$(LINK_SINGLE_THREAD) $(OPTIMISATION) $(CORE_OBJECTS_SINGLE_THREAD) $(EPHEM_OBJECTS_SINGLE_THREAD) $(LIBS) -o $(LOCAL_BINDIR)/single_thread/ephem.bin

#
# Make the socket server binaries
#

$(LOCAL_BINDIR)/socket_server.bin: $(CORE_OBJECTS) $(SOCKET_SERVER_OBJECTS)
	mkdir -p $(LOCAL_BINDIR)
	$(LINK) $(OPTIMISATION) $(CORE_OBJECTS) $(SOCKET_SERVER_OBJECTS) $(LIBS) -o $(LOCAL_BINDIR)/socket_server.bin

$(LOCAL_BINDIR)/debug/socket_server.bin: $(CORE_OBJECTS_DEBUG) $(SOCKET_SERVER_OBJECTS_DEBUG)
	mkdir -p $(LOCAL_BINDIR)/debug
	$(LINK) $(OPTIMISATION) $(CORE_OBJECTS_DEBUG) $(SOCKET_SERVER_OBJECTS_DEBUG) $(LIBS) -o $(LOCAL_BINDIR)/debug/socket_server.bin

$(LOCAL_BINDIR)/single_thread/socket_server.bin: $(CORE_OBJECTS_SINGLE_THREAD) $(SOCKET_SERVER_OBJECTS_SINGLE_THREAD)
	mkdir -p $(LOCAL_BINDIR)/single_thread
	$(LINK_SINGLE_THREAD) $(OPTIMISATION) $(CORE_OBJECTS_SINGLE_THREAD) $(SOCKET_SERVER_OBJECTS_SINGLE_THREAD) $(LIBS) -o $(LOCAL_BINDIR)/single_thread/socket_server.bin

#
# Make the socket client binaries
#

$(LOCAL_BINDIR)/socket_client.bin: $(CORE_OBJECTS) $(SOCKET_CLIENT_OBJECTS)
	mkdir -p $(LOCAL_BINDIR)
	$(LINK) $(OPTIMISATION) $(CORE_OBJECTS) $(SOCKET_CLIENT_OBJECTS) $(LIBS) -o $(LOCAL_BINDIR)/socket_client.bin

$(LOCAL_BINDIR)/debug/socket_client.bin: $(CORE_OBJECTS_DEBUG) $(SOCKET_CLIENT_OBJECTS_DEBUG)
	mkdir -p $(LOCAL_BINDIR)/debug
	$(LINK) $(OPTIMISATION) $(CORE_OBJECTS_DEBUG) $(SOCKET_CLIENT_OBJECTS_DEBUG) $(LIBS) -o $(LOCAL_BINDIR)/debug/socket_client.bin

$(LOCAL_BINDIR)/single_thread/socket_client.bin: $(CORE_OBJECTS_SINGLE_THREAD) $(SOCKET_CLIENT_OBJECTS_SINGLE_THREAD)
	mkdir -p $(LOCAL_BINDIR)/single_thread
	$(LINK_SINGLE_THREAD) $(OPTIMISATION) $(CORE_OBJECTS_SINGLE_THREAD) $(SOCKET_CLIENT_OBJECTS_SINGLE_THREAD) $(LIBS) -o $(LOCAL_BINDIR)/single_thread/socket_client.bin

#
# Make binaries for producing list of bright asteroids at opposition
#

$(LOCAL_BINDIR)/asteroids.bin: $(CORE_OBJECTS) $(ASTEROID_OBJECTS)
	mkdir -p $(LOCAL_BINDIR)
	$(LINK) $(OPTIMISATION) $(CORE_OBJECTS) $(ASTEROID_OBJECTS) $(LIBS) -o $(LOCAL_BINDIR)/asteroids.bin

$(LOCAL_BINDIR)/debug/asteroids.bin: $(CORE_OBJECTS_DEBUG) $(ASTEROID_OBJECTS_DEBUG)
	mkdir -p $(LOCAL_BINDIR)/debug
	echo "The files in this directory are binaries with debugging options enabled: they produce activity logs called 'ephem.log'. It should be noted that these binaries can up to ten times slower than non-debugging versions." > $(LOCAL_BINDIR)/debug/README
	$(LINK) $(OPTIMISATION) $(CORE_OBJECTS_DEBUG) $(ASTEROID_OBJECTS_DEBUG) $(LIBS) -o $(LOCAL_BINDIR)/debug/asteroids.bin

$(LOCAL_BINDIR)/single_thread/asteroids.bin: $(CORE_OBJECTS_SINGLE_THREAD) $(ASTEROID_OBJECTS_SINGLE_THREAD)
	mkdir -p $(LOCAL_BINDIR)/single_thread
	$(LINK_SINGLE_THREAD) $(OPTIMISATION) $(CORE_OBJECTS_SINGLE_THREAD) $(ASTEROID_OBJECTS_SINGLE_THREAD) $(LIBS) -o $(LOCAL_BINDIR)/single_thread/asteroids.bin

#
# Clean macros
#

clean:
	rm -vfR $(LOCAL_OBJDIR) $(LOCAL_BINDIR)

afresh: clean all
