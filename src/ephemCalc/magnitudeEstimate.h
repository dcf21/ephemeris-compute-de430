// magnitudeEstimate.h
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

#ifndef MAGNITUDEESTIMATE_H
#define MAGNITUDEESTIMATE_H 1

#include <settings/settings.h>

#ifndef MAGNITUDEESTIMATE_C
extern double *albedo_array;
extern double *phy_size_array;
#endif

void magnitudeEstimate_init();

void magnitudeEstimate(int body_id, double xo, double yo, double zo, double xe, double ye, double ze, double xs,
                       double ys, double zs, double *ra, double *dec, double *mag, double *phase, double *angSize,
                       double *phySize, double *albedoOut, double *sunDist, double *earthDist, double *sunAngDist,
                       double *theta_eso, double *eclipticLongitude, double *eclipticLatitude,
                       double *eclipticDistance, settings *i);

#endif

