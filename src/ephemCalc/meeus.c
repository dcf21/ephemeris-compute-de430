// meeus.c
// 
// -------------------------------------------------
// Copyright 2015-2019 Dominic Ford
//
// This file is part of EphemerisCompute.
//
// EphemerisCompute is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// EphemerisCompute is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with EphemerisCompute.  If not, see <http://www.gnu.org/licenses/>.
// -------------------------------------------------

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "settings/settings.h"

#include "meeus.h"
#include "magnitudeEstimate.h"

//! meeus_computeEphemeris - Main entry point for estimating the position, brightness, etc of an object using the
//! algorithms in Jean Meeus's Astronomical Algorithms. Unfortunately not implemented yet.
//!
//! \param [in] i - Global settings used by ephemerisCompute
//! \param [in] bodyId - The object ID number we want to query. 0=Mercury. 2=Earth/Moon barycentre. 9=Pluto. 10=Sun, etc
//! \param [in] JD - The Julian Day number to query
//! \param [out] x - x,y,z position of body, in AU relative to solar system barycentre.
//! \param [out] y - negative x points to vernal equinox. z points to celestial north pole (i.e. J2000.0).
//! \param [out] z
//! \param [out] ra - Right ascension of the object
//! \param [out] dec - Declination of the object
//! \param [out] mag - Estimated V-band magnitude of the object
//! \param [out] phase - Phase of the object (0-1)
//! \param [out] angSize - Angular size of the object
//! \param [out] phySize - Physical size of the object
//! \param [out] albedo - Albedo of the object
//! \param [out] sunDist - Distance of the object from the Sun
//! \param [out] earthDist - Distance of the object from the Earth
//! \param [out] sunAngDist - Angular distance of the object from the Sun, as seen from the Earth
//! \param [out] theta_eso - Angular distance of the object from the Earth, as seen from the Sun
//! \param [out] eclipticLongitude - The ecliptic longitude of the object
//! \param [out] eclipticLatitude - The ecliptic latitude of the object
//! \param [out] eclipticDistance - The separation of the object from the Sun, in ecliptic longitude

void meeus_computeEphemeris(settings *i, int bodyId, double JD, double *x, double *y, double *z, double *ra,
                            double *dec, double *mag, double *phase, double *angSize, double *phySize, double *albedo,
                            double *sunDist, double *earthDist, double *sunAngDist, double *theta_eso,
                            double *eclipticLongitude, double *eclipticLatitude,
                            double *eclipticDistance) {

    // This is not implemented yet...

}

