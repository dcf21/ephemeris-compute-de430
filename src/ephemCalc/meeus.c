// meeus.c
// 
// -------------------------------------------------
// Copyright 2015-2024 Dominic Ford
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
//! \param [in] bodyId - The object ID number we want to query. 0=Mercury. 2=Earth/Moon barycentre. 9=Pluto. 10=Sun, etc
//! \param [in] jd - The Julian date to query; TT
//! \param [out] x - x,y,z position of body, in ICRF v2, in AU, relative to solar system barycentre.
//! \param [out] y - x points to RA=0. y points to RA=6h.
//! \param [out] z - z points to celestial north pole (i.e. J2000.0).
//! \param [out] ra - Right ascension of the object (J2000.0, radians, relative to geocentre)
//! \param [out] dec - Declination of the object (J2000.0, radians, relative to geocentre)
//! \param [out] mag - Estimated V-band magnitude of the object
//! \param [out] phase - Phase of the object (0-1)
//! \param [out] angSize - Angular size of the object (arcseconds)
//! \param [out] phySize - Physical size of the object (diameter, metres)
//! \param [out] albedo - Albedo of the object (0-1)
//! \param [out] sunDist - Distance of the object from the Sun (AU)
//! \param [out] earthDist - Distance of the object from the Earth (AU)
//! \param [out] sunAngDist - Angular distance of the object from the Sun, as seen from the Earth (radians)
//! \param [out] theta_ESO - Angular distance of the object from the Earth, as seen from the Sun (radians)
//! \param [out] eclipticLongitude - The ecliptic longitude of the object (J2000.0 radians)
//! \param [out] eclipticLatitude - The ecliptic latitude of the object (J2000.0 radians)
//! \param [out] eclipticDistance - The separation of the object from the Sun, in ecliptic longitude (radians)
//! \param [in] ra_dec_epoch - The epoch of the RA/Dec coordinates to output. Supply 2451545.0 for J2000.0.
//! \param [in] do_topocentric_correction - Boolean indicating whether to apply topocentric correction to (ra, dec)
//! \param [in] topocentric_latitude - Latitude (deg) of observer on Earth, if topocentric correction is applied.
//! \param [in] topocentric_longitude - Longitude (deg) of observer on Earth, if topocentric correction is applied.

void meeus_computeEphemeris(int bodyId, double jd, double *x, double *y, double *z, double *ra,
                            double *dec, double *mag, double *phase, double *angSize, double *phySize, double *albedo,
                            double *sunDist, double *earthDist, double *sunAngDist, double *theta_eso,
                            double *eclipticLongitude, double *eclipticLatitude,
                            double *eclipticDistance, double ra_dec_epoch,
                            int do_topocentric_correction, double topocentric_latitude, double topocentric_longitude) {

    // This is not implemented yet...

}

