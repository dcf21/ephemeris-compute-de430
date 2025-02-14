cmake_minimum_required(VERSION 3.6)
project(ephemeris_compute)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -Wall -std=c99 -D SRCDIR='\"${CMAKE_SOURCE_DIR}/src/\"' -D DCFVERSION='\"6.0\"' -D DEBUG=0 -D MEMDEBUG1=0 -D MEMDEBUG2=0")

include_directories(src)
include_directories(src/argparse)
include_directories(src/coreUtils)
include_directories(src/ephemCalc)
include_directories(src/listTools)
include_directories(src/mathsTools)
include_directories(src/settings)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
        src/argparse/argparse.c
        src/argparse/argparse.h
        src/asteroids.c
        src/coreUtils/asciiDouble.c
        src/coreUtils/asciiDouble.h
        src/coreUtils/errorReport.c
        src/coreUtils/errorReport.h
        src/coreUtils/makeRasters.c
        src/coreUtils/makeRasters.h
        src/coreUtils/strConstants.h
        src/ephemCalc/constellations.c
        src/ephemCalc/constellations.h
        src/ephemCalc/jpl.c
        src/ephemCalc/jpl.h
        src/ephemCalc/magnitudeEstimate.c
        src/ephemCalc/magnitudeEstimate.h
        src/ephemCalc/meeus.c
        src/ephemCalc/meeus.h
        src/ephemCalc/orbitalElements.c
        src/ephemCalc/orbitalElements.h
        src/listTools/ltDict.c
        src/listTools/ltDict.h
        src/listTools/ltList.c
        src/listTools/ltList.h
        src/listTools/ltMemory.c
        src/listTools/ltMemory.h
        src/listTools/ltStringProc.c
        src/listTools/ltStringProc.h
        src/main.c
        src/mathsTools/julianDate.c
        src/mathsTools/julianDate.h
        src/mathsTools/precess_equinoxes.c
        src/mathsTools/precess_equinoxes.h
        src/mathsTools/sphericalAst.c
        src/mathsTools/sphericalAst.h
        src/settings/settings.c
        src/settings/settings.h)

add_executable(ephem ${SOURCE_FILES} src/main.c)
add_executable(asteroids ${SOURCE_FILES} src/asteroids.c)
