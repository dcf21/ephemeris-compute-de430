#!/bin/bash
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

# Do all of the tasks we need to get the ephemeris computation code up and running

cd "$(dirname "$0")" || exit 1
cwd=`pwd`

# Download all of the data we need from the internet
echo "[`date`] Downloading required data files"
cd ${cwd} || exit 1
./dataFetch.py "$@" || exit 1

# Delete old binary ephemeris files
echo "[`date`] Cleaning old binary files"
cd ${cwd} || exit 1
rm -f data/binary_*.bin

# Compile the ephemerisCompute code
echo "[`date`] Compiling code"
cd ${cwd} || exit 1
./prettymake clean || exit 1
./prettymake || exit 1

# Make some sample ephemerides, to ensure text input files are converted to binary
echo "[`date`] Generating sample ephemerides"
cd ${cwd} || exit 1
./runDemos.py "$@" || exit 1

# Finished
cd ${cwd} || exit 1
echo "[`date`] Finishing DoAll script"
