// constellations.c
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

// Code used to store information about the constellations, and specifically to work out which constellation any
// particular point on the night sky lies within. We do this by calculating the winding number of the boundary of each
// constellation around the point being tested. The winding number will be zero for all constellations except for the
// one the point lies within. For this constellation, the winding number will be +/i 2pi.

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <gsl/gsl_math.h>

#include "coreUtils/asciiDouble.h"
#include "coreUtils/errorReport.h"
#include "coreUtils/strConstants.h"

#include "listTools/ltMemory.h"
#include "listTools/ltDict.h"

#include "mathsTools/sphericalAst.h"

#include "constellations.h"

//! constel_point - A structure representing a point on the outline of a constellation.

typedef struct {
    double RA, Dec;
} constel_point;

//! constel_desc - A structure containing the number of a constellation, and a set of points defining its outline in
//! J2000 celestial coordinates.

typedef struct {
    constel_point point[1024];
    int ShortNameSet, LongNameSet, Npoints;
    char ShortName[6], LongName[64];
} constel_desc;

//! constel_desc constel_data - Storage for the 88 constellations which make up the night sky
static constel_desc constel_data[90];

//! Nconstel - A counter for the number of constellations we have loaded so far
static int Nconstel = 0;

//! dWind - Work out the change in azimuth winding number along the line segment (RA0, Dec0) to (RA1, Dec1), as seen
//! from (RA, Dec).
//! \param RA - The right ascension of the point whose constellation we are determining (radians)
//! \param Dec - The declination of the point whose constellation we are determining (radians)
//! \param RA0 - The right ascension of the start of the line segment for which we are calculating d[azimuth] (radians)
//! \param Dec0 - The declination of the start of the line segment for which we are calculating d[azimuth] (radians)
//! \param RA1 - The right ascension of the end of the line segment for which we are calculating d[azimuth] (radians)
//! \param Dec1 - The declination of the end of the line segment for which we are calculating d[azimuth] (radians)
//! \return Change in azimuth (or winding number) (radians)

static double dWind(double RA, double Dec, double RA0, double Dec0, double RA1, double Dec1) {
    double xA0 = sin(RA0) * cos(Dec0);
    double xA1 = sin(RA1) * cos(Dec1);
    double yA0 = cos(RA0) * cos(Dec0);
    double yA1 = cos(RA1) * cos(Dec1);
    double zA0 = sin(Dec0);
    double zA1 = sin(Dec1);

    double xB0 = xA0 * cos(-RA) + yA0 * sin(-RA);
    double xB1 = xA1 * cos(-RA) + yA1 * sin(-RA);
    double yB0 = xA0 * sin(RA) + yA0 * cos(-RA);
    double yB1 = xA1 * sin(RA) + yA1 * cos(-RA);
    double zB0 = zA0;
    double zB1 = zA1;

    double a = (M_PI / 2) - Dec;

    double xC0 = xB0;
    double xC1 = xB1;
    double yC0 = yB0 * cos(-a) + zB0 * sin(-a);
    double yC1 = yB1 * cos(-a) + zB1 * sin(-a);
//double zC0 = yB0*sin(  a) + zB0*cos(- a);      double zC1 = yB1*sin(  a) + zB1*cos(- a);

    double dW;

    dW = atan2(xC0, yC0) - atan2(xC1, yC1);
    while (dW < -M_PI) dW += 2 * M_PI;
    while (dW > M_PI) dW -= 2 * M_PI;
    return dW;
}

//! constellations_init - Initialise the constellations module. Load the constellation boundaries from disk.

void constellations_init() {
    FILE *file;
    int i;
    char line[FNAME_LENGTH], *scan, constellation[6] = "@@@";

    Nconstel = 0;

    // Scan through catalogue of bound
    file = fopen(SRCDIR "../constellations/bound_20.dat", "r");
    if (file == NULL) ephem_fatal(__FILE__, __LINE__, "Could not open constellation boundary data");

    while ((!feof(file)) && (!ferror(file))) {
        double ra, dec;
        file_readline(file, line);
        if ((line[0] == '#') || (strlen(line) < 28)) continue; // Comment line
        scan = line + 0;
        while ((scan[0] > '\0') && (scan[0] <= ' ')) scan++;
        ra = get_float(scan, NULL);
        scan = line + 12;
        while ((scan[0] > '\0') && (scan[0] <= ' ')) scan++;
        dec = get_float(scan, NULL);
        if (line[11] == '-') dec = -dec;

        if (strncmp(line + 23, constellation, 4) != 0) {
            Nconstel++;
            constel_data[Nconstel - 1].ShortNameSet = 1;
            constel_data[Nconstel - 1].LongNameSet = 0;
            constel_data[Nconstel - 1].Npoints = 0;
            strncpy(constel_data[Nconstel - 1].ShortName, line + 23, 4);
            constel_data[Nconstel - 1].ShortName[5] = '\0';
            strncpy(constellation, line + 23, 4);
            constellation[5] = '\0';
        }

        constel_data[Nconstel - 1].point[constel_data[Nconstel - 1].Npoints].RA = ra / 12. * M_PI;
        constel_data[Nconstel - 1].point[constel_data[Nconstel - 1].Npoints].Dec = dec / 180. * M_PI;
        constel_data[Nconstel - 1].Npoints++;
    }

    fclose(file);

    // Scan through list of full names
    file = fopen(SRCDIR "../constellations/constellation_names.dat", "r");
    if (file == NULL) ephem_fatal(__FILE__, __LINE__, "Could not open constellation name data");

    while ((!feof(file)) && (!ferror(file))) {
        char ShortName[6], LongName[64];
        int i, done;

        // Read short/long name pair
        file_readline(file, line);
        if ((line[0] == '#') || (strlen(line) < 4)) continue; // Comment line
        scan = line + 0;
        while ((scan[0] > '\0') && (scan[0] <= ' ')) scan++;
        for (i = 0; scan[0] > ' '; i++, scan++) ShortName[i] = *scan;
        while (i < 4) ShortName[i++] = ' ';
        ShortName[i] = '\0';
        while ((scan[0] > '\0') && (scan[0] <= ' ')) scan++;
        for (i = 0; scan[0] > ' '; i++, scan++) {
            if ((*scan < 'a') && (i > 0)) LongName[i++] = '@';
            LongName[i] = *scan;
        }
        LongName[i] = '\0';

        // Search for entry in constel_data
        for (done = i = 0; i < Nconstel; i++) {
            if (!constel_data[i].ShortNameSet) ephem_fatal(__FILE__, __LINE__, "ShortName of constellation not set");
            if (strcmp(ShortName, constel_data[i].ShortName) == 0) {
                strcpy(constel_data[i].LongName, LongName);
                constel_data[i].LongNameSet = 1;
                done = 1;
                break;
            }
        }
        if (!done) {
            sprintf(temp_err_string, "Could not find match for constellation short name '%s'", ShortName);
            ephem_fatal(__FILE__, __LINE__, temp_err_string);
        }
    }

    for (i = 0; i < Nconstel; i++)
        if (!constel_data[i].LongNameSet) {
            sprintf(temp_err_string, "Could not find long name for constellation'%s'", constel_data[i].ShortName);
            ephem_fatal(__FILE__, __LINE__, temp_err_string);
        }

    fclose(file);
}

//! constellations_fetch - Determine which constellation a point lies within
//! \param ra - The right ascension of the point whose constellation we are determining (radians)
//! \param dec - The declination of the point whose constellation we are determining (radians)
//! \return The full name of the constellation, in a static character buffer

char *constellations_fetch(double ra, double dec) {
    int i, outcome[Nconstel];

#pragma omp parallel for shared(outcome) private(i)
    for (i = 0; i < Nconstel; i++) {
        int j;
        double winding = 0.0;
        double ang_sep = angDist_RADec(ra, dec, constel_data[i].point[0].RA, constel_data[i].point[0].Dec);

        // Winding number calculation triggers for constellation containing point opposite to (RA,DEC) as well as
        // for desired point; filter for this now.
        if (ang_sep > M_PI / 2) {
            outcome[i] = 0;
            continue;
        }

        for (j = 0; j < constel_data[i].Npoints; j++) {
            int k = (j + 1) % constel_data[i].Npoints;
            winding += dWind(ra, dec, constel_data[i].point[j].RA, constel_data[i].point[j].Dec,
                             constel_data[i].point[k].RA, constel_data[i].point[k].Dec);
        }

        if (fabs(winding) > M_PI) {
            outcome[i] = 1;
        } else {
            outcome[i] = 0;
        }
    }

    // Work out which constellation produced a positive outcome
    for (i = 0; i < Nconstel; i++) {
        if (outcome[i]) return constel_data[i].LongName;
    }

    // No constellation produced a positive outcome
    return "Unknown";
}

//! constellations_close - Free up any memory used by the constellations module.

void constellations_close() {
}

