#!/usr/bin/python3
# -*- coding: utf-8 -*-
# dataFetch.py
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

"""
Automatically download all of the required data files from the internet.
"""

import os
import sys
import logging


def fetch_file(web_address, destination, force_refresh=False):
    """
    Download a file that we need, using wget.

    :param web_address:
        The URL that we should use to fetch the file
    :type web_address:
        str
    :param destination:
        The path we should download the file to
    :type destination:
        str
    :param force_refresh:
        Boolean flag indicating whether we should download a new copy if the file already exists.
    :type force_refresh:
        bool
    :return:
        Boolean flag indicating whether the file was downloaded. Raises IOError if the download fails.
    """
    logging.info("Fetching file <{}>".format(destination))

    # Check if the file already exists
    if os.path.exists(destination):
        if not force_refresh:
            logging.info("File already exists. Not downloading fresh copy.")
            return False
        else:
            logging.info("File already exists, but downloading fresh copy.")
            os.unlink(destination)

    # Fetch the file with wget
    os.system("wget -q '{}' -O {}".format(web_address, destination))

    # Check that the file now exists
    if not os.path.exists(destination):
        raise IOError("Could not download file <{}>".format(web_address))

    return True


def fetch_required_files():
    # List of the files we require
    required_files = [
        {
            'url': 'ftp://ftp.lowell.edu/pub/elgb/astorb.dat.gz',
            'destination': 'data/astorb.dat.gz',
            'force_refresh': True
        },
        {
            'url': 'https://www.minorplanetcenter.net/iau/MPCORB/CometEls.txt',
            'destination': 'data/Soft00Cmt.txt',
            'force_refresh': True
        },
        {
            'url': 'ftp://ssd.jpl.nasa.gov/pub/eph/planets/ascii/de430/header.430_572',
            'destination': 'data/header.430',
            'force_refresh': False
        },
        {
            'url': 'http://cdsarc.u-strasbg.fr/ftp/VI/49/bound_20.dat.gz',
            'destination': 'constellations/bound_20.dat.gz',
            'force_refresh': False
        },
        {
            'url': 'http://cdsarc.u-strasbg.fr/ftp/VI/49/ReadMe',
            'destination': 'constellations/ReadMe',
            'force_refresh': False
        }
    ]

    # Fetch the JPL DE430 ephemeris
    for year in range(1550, 2551, 100):
        required_files.append({
            'url': 'ftp://ssd.jpl.nasa.gov/pub/eph/planets/ascii/de430/ascp{:04d}.430'.format(year),
            'destination': 'data/ascp{:04d}.430'.format(year),
            'force_refresh': False
        })

    # Fetch all the files
    for required_file in required_files:
        fetch_file(web_address=required_file['url'],
                   destination=required_file['destination'],
                   force_refresh=required_file['force_refresh']
                   )

    # Unzip the Lowell database of asteroid orbital elements
    os.system("gunzip -f data/astorb.dat.gz")

    # ... and the constellation boundaries
    os.system("gunzip -f constellations/bound_20.dat.gz")


if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO,
                        stream=sys.stdout,
                        format='[%(asctime)s] %(levelname)s:%(filename)s:%(message)s',
                        datefmt='%d/%m/%Y %H:%M:%S')
    logger = logging.getLogger(__name__)
    logger.info(__doc__.strip())

    fetch_required_files()
