// sphericalAst.c
// 
// -------------------------------------------------
// Copyright 2015-2022 Dominic Ford
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

#include <gsl/gsl_math.h>

//! angDist_ABC - Calculate the angle between the lines BA and BC, measured at B
//! \param xa - Cartesian coordinates of point A
//! \param ya - Cartesian coordinates of point A
//! \param za - Cartesian coordinates of point A
//! \param xb - Cartesian coordinates of point B
//! \param yb - Cartesian coordinates of point B
//! \param zb - Cartesian coordinates of point B
//! \param xc - Cartesian coordinates of point C
//! \param yc - Cartesian coordinates of point C
//! \param zc - Cartesian coordinates of point C
//! \return Angle ABC (radians)

double angDist_ABC(double xa, double ya, double za, double xb, double yb, double zb, double xc, double yc, double zc) {
    double AB2 = gsl_pow_2(xa - xb) + gsl_pow_2(ya - yb) + gsl_pow_2(za - zb);
    double BC2 = gsl_pow_2(xb - xc) + gsl_pow_2(yb - yc) + gsl_pow_2(zb - zc);
    double CA2 = gsl_pow_2(xc - xa) + gsl_pow_2(yc - ya) + gsl_pow_2(zc - za);

    // Use the cosine rule
    double cosine = (AB2 + BC2 - CA2) / (2 * sqrt(AB2) * sqrt(BC2));
    if (cosine >= 1) return 0;

    return acos(cosine);
}

//! angDist_RADec - Calculate the angular distance between (RA0, Dec0) and (RA1, Dec1)
//! \param ra0 - Right ascension of the first point (radians)
//! \param dec0 - Declination of the first point (radians)
//! \param ra1 - Right ascension of the second point (radians)
//! \param dec1 - Declination of the second point (radians)
//! \return Angular distance between two points (radians)

double angDist_RADec(double ra0, double dec0, double ra1, double dec1) {
    double p0x = sin(ra0) * cos(dec0);
    double p0y = cos(ra0) * cos(dec0);
    double p0z = sin(dec0);

    double p1x = sin(ra1) * cos(dec1);
    double p1y = cos(ra1) * cos(dec1);
    double p1z = sin(dec1);

    double sep2 = gsl_pow_2(p0x - p1x) + gsl_pow_2(p0y - p1y) + gsl_pow_2(p0z - p1z);
    if (sep2 <= 0) return 0;

    double sep = sqrt(sep2);
    return 2 * asin(sep / 2);
}

//! ra_dec_from_j2000 - Convert celestial coordinates from J2000 into a new epoch.
//! See Green's Spherical Astronomy, pp 222-225
//! \param [in] ra0 - Right ascension, in hours, J2000
//! \param [in] dec0 - Declination, in degrees, J2000
//! \param [in] utc_new - Unix time of the epoch we are to transform celestial coordinates into
//! \param [out] ra_out - Output right ascension, in hours, epoch <utc_new>
//! \param [out] dec_out - Output declination, in degrees, epoch <utc_new>
void ra_dec_from_j2000(double ra0, double dec0, double utc_new, double *ra_out, double *dec_out) {

    // Convert inputs into radians
    ra0 *= M_PI / 12;
    dec0 *= M_PI / 180;

    const double u = utc_new;
    const double j = 40587.5 + u / 86400.0; // Julian date - 2400000
    const double t = (j - 51545.0) / 36525.0; // Julian century (no centuries since 2000.0)

    const double deg = M_PI / 180;
    const double m = (1.281232 * t + 0.000388 * t * t) * deg;
    const double n = (0.556753 * t + 0.000119 * t * t) * deg;

    const double ra_m = ra0 + 0.5 * (m + n * sin(ra0) * tan(dec0));
    const double dec_m = dec0 + 0.5 * n * cos(ra_m);

    const double ra_new = ra0 + m + n * sin(ra_m) * tan(dec_m);
    const double dec_new = dec0 + n * cos(ra_m);

    *ra_out = ra_new * 12 / M_PI;
    *dec_out = dec_new * 180 / M_PI;
}
