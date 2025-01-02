#!/bin/bash
#
# -------------------------------------------------
# Copyright 2015-2025 Dominic Ford
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

cd "$(dirname "$0")" || exit
cwd=`pwd`

# Download all of the data we need from the internet
echo "[`date`] Downloading required data files"
cd ${cwd} || exit
./dataFetch.py

# Delete old binary ephemeris files
echo "[`date`] Cleaning old binary files"
cd ${cwd} || exit
rm -f data/dcfbinary*

# Compile the ephemerisCompute code
echo "[`date`] Compiling code"
cd ${cwd} || exit
./prettymake clean
./prettymake
