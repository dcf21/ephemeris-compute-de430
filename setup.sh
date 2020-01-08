#!/bin/bash
#
# -------------------------------------------------
# Copyright 2015-2020 Dominic Ford
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

cd "$(dirname "$0")"
cwd=`pwd`

# Download all of the data we need from the internet
echo "Downloading required data files"
cd ${cwd}
./dataFetch.py

# Compile the ephemerisCompute code
echo "Compiling code"
cd ${cwd}
./prettymake
