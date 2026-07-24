// sphericalAst.c
//
// -------------------------------------------------
// Copyright 2015-2026 Dominic Ford
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

#include <stdio.h>
#include <math.h>
#include <gsl/gsl_math.h>

#include "mathsTools/deltaT.h"
#include "mathsTools/julianDate.h"

//! angDist_ABC - Calculate the angle between the lines BA and BC, measured at B
//! \param [in] xa - Cartesian coordinates of point A
//! \param [in] ya - Cartesian coordinates of point A
//! \param [in] za - Cartesian coordinates of point A
//! \param [in] xb - Cartesian coordinates of point B
//! \param [in] yb - Cartesian coordinates of point B
//! \param [in] zb - Cartesian coordinates of point B
//! \param [in] xc - Cartesian coordinates of point C
//! \param [in] yc - Cartesian coordinates of point C
//! \param [in] zc - Cartesian coordinates of point C
//! \return Angle ABC (radians)

double angDist_ABC(const double xa, const double ya, const double za,
    const double xb, const double yb, const double zb,
    const double xc, const double yc, const double zc) {
    const double AB2 = gsl_pow_2(xa - xb) + gsl_pow_2(ya - yb) + gsl_pow_2(za - zb);
    const double BC2 = gsl_pow_2(xb - xc) + gsl_pow_2(yb - yc) + gsl_pow_2(zb - zc);
    const double CA2 = gsl_pow_2(xc - xa) + gsl_pow_2(yc - ya) + gsl_pow_2(zc - za);

    // Use the cosine rule
    const double cosine = (AB2 + BC2 - CA2) / (2 * sqrt(AB2) * sqrt(BC2));
    if (cosine >= 1) return 0;

    return acos(cosine);
}

//! angDist_RADec - Calculate the angular distance between (RA0, Dec0) and (RA1, Dec1)
//! \param [in] ra0 - Right ascension of the first point (radians)
//! \param [in] dec0 - Declination of the first point (radians)
//! \param [in] ra1 - Right ascension of the second point (radians)
//! \param [in] dec1 - Declination of the second point (radians)
//! \return Angular distance between two points (radians)

double angDist_RADec(const double ra0, const double dec0, const double ra1, const double dec1) {
    const double p0x = sin(ra0) * cos(dec0);
    const double p0y = cos(ra0) * cos(dec0);
    const double p0z = sin(dec0);

    const double p1x = sin(ra1) * cos(dec1);
    const double p1y = cos(ra1) * cos(dec1);
    const double p1z = sin(dec1);

    const double sep2 = gsl_pow_2(p0x - p1x) + gsl_pow_2(p0y - p1y) + gsl_pow_2(p0z - p1z);
    if (sep2 <= 0) return 0;

    const double sep = sqrt(sep2);
    return 2 * asin(sep / 2);
}

//! getAltitude - Calculate the altitude of an astronomical object (degrees)
//! @param [in] ra - Right ascension of object, at epoch, radians.
//! @param [in] dec - Declination of object, at epoch, radians.
//! @param [in] JD - Julian day number of epoch (TT).
//! @param [in] latitude - Latitude of the observer, radians.
//! @return Altitude, degrees
double getAltitude(const DeltaTCalculator *dt_calc,
    const double ra, const double dec, const double JD, const double latitude) {
    const double deg = M_PI / 180.0;

    const double Xbody = sin(ra) * cos(dec);
    const double Ybody = cos(ra) * cos(dec);
    const double Zbody = sin(dec);

    // See pages 87-88 Astronomical Algorithms, by Jean Meeus
    const double zen_RA = sidereal_time_jd(dt_calc, JD);
    const double Xzen = sin(zen_RA) * cos(latitude);
    const double Yzen = cos(zen_RA) * cos(latitude);
    const double Zzen = sin(latitude);

    const double zen_angle = acos((2 - gsl_pow_2(Xzen - Xbody)
        - gsl_pow_2(Yzen - Ybody)
        - gsl_pow_2(Zzen - Zbody)) / 2)
                       / deg;

    return 90.0 - zen_angle; // Output in degrees
}

//! write_object_separation_string - Compile an HTML string describing the distance between the objects in appropriate units.
//! @param [in] separation_radians - Separation of objects, in radians
//! @param [out] separation_string - String describing the separation, in appropriate units
void write_object_separation_string(const double separation_radians, char *separation_string) {
    const double sep_deg = separation_radians * 180 / M_PI;
    if (sep_deg >= 1) {
        sprintf(separation_string, "%d&deg;%02d&#39;",
                ((int) floor(sep_deg)), ((int) floor(sep_deg * 60)) % 60);
    } else if (sep_deg >= 1. / 6) {
        sprintf(separation_string, "%d&#39;",
                ((int) floor(sep_deg * 60)) % 60);
    } else if (sep_deg >= 1. / 60) {
        sprintf(separation_string, "%d&#39;%02d&#34;",
                ((int) floor(sep_deg * 60)) % 60, ((int) floor(sep_deg * 3600)) % 60);
    } else {
        sprintf(separation_string, "%d&#34;",
                ((int) floor(sep_deg * 3600)) % 60);
    }
}

//! properMotion - Calculate the magnitude of the proper motion of an object from two positions.
//! \param ra0 - Right ascension of the first point (radians)
//! \param dec0 - Declination of the first point (radians)
//! \param ra1 - Right ascension of the second point (radians)
//! \param dec1 - Declination of the second point (radians)
//! \param utcStep - Time interval between the two calculated positions (seconds)
//! \return Proper motion in mas/yr
double properMotion(double ra0, double dec0, double ra1, double dec1, double utcStep) {
    const double distance = angDist_RADec(ra0, dec0, ra1, dec1); // radians
    const double radians_per_second = distance / utcStep;
    const double mas_per_second = radians_per_second * (180 / M_PI * 3600 * 1000);
    const double mas_per_year = mas_per_second * 86400 * 365.25;
    return mas_per_year;
}

//! positionAngle - Return the position angle of the great circle path from (RA1, Dec1) to (RA2, Dec2), as seen at the
//! former point.
//! \param ra0 - Right ascension of the first point (radians)
//! \param dec0 - Declination of the first point (radians)
//! \param ra1 - Right ascension of the second point (radians)
//! \param dec1 - Declination of the second point (radians)
//! \return Position angle - degrees east of north
double positionAngle(const double ra0, const double dec0, const double ra1, const double dec1) {
    const double x1 = cos(ra1) * cos(dec1);
    const double y1 = sin(ra1) * cos(dec1);
    const double z1 = sin(dec1);

    // Rotate by -ra0 around (x,y)
    const double theta = -ra0;
    const double x2 = x1 * cos(theta) + y1 * -sin(theta);
    const double y2 = x1 * sin(theta) + y1 * cos(theta);
    const double z2 = z1;

    // Rotate by (pi / 2 - dec1) around xz
    const double phi = M_PI / 2 - dec0;
    const double x3 = x2 * cos(phi) + z2 * -sin(phi);
    const double y3 = y2;
    // const double z3 = x2 * sin(phi) + z2 * cos(phi);

    // Calculate azimuth
    const double azimuth = atan2(y3, -x3);
    return azimuth * 180 / M_PI;
}
