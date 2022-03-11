// precess_equinoxes.c
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

#include "precess_equinoxes.h"

// Precess equatorial celestial coordinates from one epoch to another

// For equivalent Javascript code, see:

// https://github.com/commenthol/astronomia/blob/master/src/base.js
// https://github.com/commenthol/astronomia/blob/master/src/precess.js

//! horner - Evaluate a polynomial with coefficients c, at point x. The constant term is c[0]; the linear term is
//! c[1]; etc.
//! \param x - Evaluate the polynomial at point x
//! \param c - The coefficients of the polynomial
//! \param length - The number of coefficients in the polynomial
//! \return - f(x)

double horner(double x, const double *c, int length) {
    int i = length - 1;
    double y = c[i];
    while (i > 0) {
        i--;
        y = y * x + c[i];
    }
    return y;
}

//! precess - Convert a celestial position, expressed into equatorial coordinates at one epoch, into equatorial
//! coordinates at a different epoch.
//! \param [in] epochFrom - The epoch of the input equatorial coordinates, expressed as a Julian day number.
//! \param [in] epochTo- The epoch of the output equatorial coordinates, expressed as a Julian day number.
//! \param [in] eclFrom_lng - Input equatorial longitude (radians)
//! \param [in] eclFrom_lat - Input equatorial latitude (radians)
//! \param [out] eclTo_lng - Output equatorial longitude (radians)
//! \param [out] eclTo_lat - Output equatorial latitude (radians)

void precess(double epochFrom, double epochTo, double eclFrom_lng, double eclFrom_lat,
             double *eclTo_lng, double *eclTo_lat) {

    // Convert epochs from JDs into Julian years
    epochFrom = (epochFrom - 2451544.5) / 365.2425 + 2000;
    epochTo = (epochTo - 2451544.5) / 365.2425 + 2000;

    double smallAngle = 10 * M_PI / 180 / 60; // about .003 radians
    // cosine of SmallAngle
    double cosSmallAngle = cos(smallAngle); // about .999996


    // coefficients from (21.5) p. 136
    const double d = M_PI / 180;
    const double s = d / 3600;
    const double eta_T[3] = {47.0029 * s, -0.06603 * s, 0.000598 * s};
    const double pi_T[3] = {174.876384 * d, 3289.4789 * s, 0.60622 * s};
    const double p_T[3] = {5029.0966 * s, 2.22226 * s, -0.000042 * s};

    // (21.5) p. 136
    double eta_coeff[3];
    double pi_coeff[3];
    double p_coeff[3];

    double T = (epochFrom - 2000) * 0.01; // Julian centuries
    eta_coeff[0] = horner(T, eta_T, 3);
    eta_coeff[1] = -0.03302 * s + 0.000598 * s * T;
    eta_coeff[2] = 0.000060 * s;

    pi_coeff[0] = horner(T, pi_T, 3);
    pi_coeff[1] = -869.8089 * s - 0.50491 * s * T;
    pi_coeff[2] = 0.03536 * s;

    p_coeff[0] = horner(T, p_T, 3);
    p_coeff[1] = 1.11113 * s - 0.000042 * s * T;
    p_coeff[2] = -0.000006 * s;

    double t = (epochTo - epochFrom) * 0.01;
    double pi = horner(t, pi_coeff, 3);
    double p = horner(t, p_coeff, 3) * t;
    double eta = horner(t, eta_coeff, 3) * t;
    double s_eta = sin(eta);
    double c_eta = cos(eta);

    // (21.7) p. 137
    double s_beta = sin(eclFrom_lat);
    double c_beta = cos(eclFrom_lat);
    double sd = sin(pi - eclFrom_lng);
    double cd = cos(pi - eclFrom_lng);

    double A = c_eta * c_beta * sd - s_eta * s_beta;
    double B = c_beta * cd;
    double C = c_eta * s_beta + s_eta * c_beta * sd;

    *eclTo_lng = p + pi - atan2(A, B);

    if (C < cosSmallAngle) {
        *eclTo_lat = asin(C);
    } else {
        *eclTo_lat = acos(hypot(A, B)); // near pole
    }
}

// Simple test case for diagnostics

//void main() {
//    double JD;
//    for (JD=2451544.5; JD<2469807.5; JD+=10) {
//        double lat, lng;
//        precess(2451544.5, JD, 0, 0, &lng, &lat);
//        double lng2 = (6.4691550386335095e-07 * (JD - 2451544.5));
//        printf("%10.2f %10.6f %10.6f\n", JD, lng, lng2);
//    }
//}
