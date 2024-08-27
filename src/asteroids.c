// asteroids.c
// Dominic Ford
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

// This is a simple tool for automatically scanning for moments when asteroids are at opposition

// On the command line, you need to specify three numbers:
// * The starting JD
// * The ending JD
// * The magnitude limit (i.e. the faintest magnitude an asteroid may have at opposition to be listed)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#include <gsl/gsl_const_mksa.h>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_math.h>

#include "coreUtils/asciiDouble.h"
#include "coreUtils/strConstants.h"
#include "coreUtils/errorReport.h"

#include "ephemCalc/constellations.h"
#include "ephemCalc/jpl.h"
#include "ephemCalc/magnitudeEstimate.h"
#include "ephemCalc/orbitalElements.h"

#include "listTools/ltMemory.h"

#include "mathsTools/julianDate.h"

#include "settings/settings.h"

#define N_INPUTS 7

void file_event(int report, int i, const char *name, char *type, double JD, double mag, double earth_dist,
                double ra, double dec) {
    int year, month, day, hour, min, j;
    double sec;

    char name_no_spaces[1024];
    for (j = 0; name[j] != '\0'; j++) {
        name_no_spaces[j] = name[j];
        if (name[j] == ' ') name_no_spaces[j] = '@';
    }
    name_no_spaces[j] = '\0';

#pragma omp critical (file_asteroid_event)
    {
        inv_julian_day(JD, &year, &month, &day, &hour, &min, &sec, &j, temp_err_string);
        snprintf(temp_err_string, FNAME_LENGTH,
                 "%10.1f %04d %02d %02d %02d %02d %s   %6.1f %8.3f   %10.6f %10.6f %s   "
                 "%07d %s %.16e %.16e %.16e %.16e %.16e %.16e %.16e",
                 JD, year, month, day, hour, min, type, mag, earth_dist, ra, dec,
                 constellations_fetch(ra, dec), i, name_no_spaces,
                 asteroid_database[i].semiMajorAxis, asteroid_database[i].eccentricity,
                 asteroid_database[i].longAscNode, asteroid_database[i].inclination,
                 asteroid_database[i].argumentPerihelion, asteroid_database[i].meanAnomaly,
                 asteroid_database[i].epochOsculation);
        if (report) {
            fprintf(stdout, "%s\n", temp_err_string);
            fflush(stdout);
        }
        if (DEBUG) ephem_log(temp_err_string);
    }
}

void scan_for_oppositions(settings *s, double jd_min, double jd_max, double jd_step, double mag_limit, int report,
                          double *sun_ang_dist_1, double *sun_ang_dist_2, double *earth_dist_1, double *earth_dist_2,
                          double *mag1, double *mag2, const int *selected_in, int *selected_out) {
    int so_count = 0;
    int j, loop_iter;
    double jd;
    int max_iters;

    if (selected_in == NULL) {
        max_iters = asteroid_count - 1;
    } else {
        for (j = 0; 1; j++) {
            int i = selected_in[j];
            if (i < 0) {
                max_iters = j;
                break;
            }
        }
    }

    // Loop, day by day, over search period
    for (jd = jd_min, loop_iter = 0; jd <= jd_max; jd += jd_step, loop_iter++) {
        //if (DEBUG) {
        // snprintf(temp_err_string, FNAME_LENGTH, "Starting work on day %.1f",jd); ephem_log(temp_err_string); }
#pragma omp parallel for shared(jd, loop_iter, max_iters, so_count) private(j)
        for (j = 0; j < max_iters; j++) {
            int i;
            if (selected_in == NULL) { i = j + 1; }
            else { i = selected_in[j]; }

            if (asteroid_database[i].secureOrbit) {
                double ra = 0, dec = 0, x = 0, y = 0, z = 0;
                double mag = 0, phase = 0, ang_size = 0, phy_size = 0, albedo = 0, sun_dist = 0;
                double earth_dist = 0, sun_ang_dist = 0, theta_eso = 0;
                double ecliptic_longitude = 0, ecliptic_latitude = 0, ecliptic_distance = 0;

                orbitalElements_computeEphemeris(s, 10000000 + i, jd, &x, &y, &z, &ra, &dec, &mag, &phase, &ang_size,
                                                 &phy_size,
                                                 &albedo, &sun_dist, &earth_dist, &sun_ang_dist, &theta_eso,
                                                 &ecliptic_longitude, &ecliptic_latitude,
                                                 &ecliptic_distance);

                // Check if asteroid is both bright, also at opposition
                if ((mag < mag_limit) && (loop_iter > 2)) {
                    if (selected_out != NULL) {
                        int got = 0, c = 0;
#pragma omp critical (select_asteroid)
                        {
                            for (c = 0; c < so_count; c++)
                                if (selected_out[c] == i) {
                                    got = 1;
                                    break;
                                }
                            if (!got) selected_out[so_count++] = i;
                        }
                    }
                    if ((sun_ang_dist_1[i] > sun_ang_dist) && (sun_ang_dist_1[i] > sun_ang_dist_2[i]))
                        file_event(report, i, asteroid_database[i].name, "Opposition", jd - jd_step, mag, earth_dist,
                                   ra, dec);
                    if ((earth_dist_1[i] < earth_dist) && (earth_dist_1[i] < earth_dist_2[i]))
                        file_event(report, i, asteroid_database[i].name, "Apogee    ", jd - jd_step, mag, earth_dist,
                                   ra, dec);
                    if ((mag1[i] < mag) && (mag1[i] < mag2[i]))
                        file_event(report, i, asteroid_database[i].name, "PeakMag   ", jd - jd_step, mag, earth_dist,
                                   ra, dec);
                }

                sun_ang_dist_2[i] = sun_ang_dist_1[i];
                sun_ang_dist_1[i] = sun_ang_dist;
                earth_dist_2[i] = earth_dist_1[i];
                earth_dist_1[i] = earth_dist;
                mag2[i] = mag1[i];
                mag1[i] = mag;
            }
        }
    }
    if (selected_out != NULL) {
        selected_out[so_count] = -1;
        if (DEBUG) {
            snprintf(temp_err_string, FNAME_LENGTH, "Selected %d objects.", so_count);
            ephem_log(temp_err_string);
        }
    }
}

int main(int argc, char **argv) {
    char help_string[LSTR_LENGTH], version_string[FNAME_LENGTH], version_string_underline[FNAME_LENGTH];
    int i, inputs_read = 0;
    settings s_model;
    double input[N_INPUTS];
    double jd_min, jd_max, mag_limit;
    double *sun_ang_dist_1, *sun_ang_dist_2;
    double *earth_dist_1, *earth_dist_2;
    double *mag1, *mag2;
    int *selected;

    // Step through 4 days at a time looking for oppositions
    const double jd_step_pass_1 = 4;

    // Step through 30 sec at a time looking for oppositions
    const double jd_step_pass_2 = 1. / 24. / 3600. * 30.;

    // Initialise sub-modules
    if (DEBUG) ephem_log("Initialising asteroid opposition search.");
    lt_memoryInit(&ephem_error, &ephem_log);
    constellations_init();

    // Turn off GSL's automatic error handler
    gsl_set_error_handler_off();

    // Make help and version strings
    snprintf(version_string, FNAME_LENGTH, "Asteroid Opposition Search %s", DCFVERSION);

    snprintf(help_string, FNAME_LENGTH,
             "Asteroid Opposition Search %s\n"
             "%s\n\n"
             "Usage: asteroids.bin <YearMin> <MonthMin> <DayMin>  <YearMax> <MonthMax> <DayMax>  <LimitingMagnitude>\n"
             "-h, --help:       Display this help.\n"
             "-v, --version:    Display version number.",
             DCFVERSION, str_underline(version_string, version_string_underline));

    // Scan command line options for any switches
    for (i = 1; i < argc; i++) {
        if (strlen(argv[i]) == 0) continue;
        if (argv[i][0] != '-') {
            if (inputs_read >= N_INPUTS) {
                snprintf(temp_err_string, FNAME_LENGTH,
                         "Received too many command line inputs.\n"
                         "Type 'asteroids.bin -help' for a list of available command-line options.");
                ephem_error(temp_err_string);
                return 1;
            }
            if (!valid_float(argv[i], NULL)) {
                snprintf(temp_err_string, FNAME_LENGTH,
                         "Received command line option '%s' which should have been a numeric value.\n"
                         "Type 'asteroids.bin -help' for a list of available command-line options.",
                         argv[i]);
                ephem_error(temp_err_string);
                return 1;
            }
            input[inputs_read] = get_float(argv[i], NULL);
            inputs_read++; //filename = argv[i];
            continue;
        }
        if ((strcmp(argv[i], "-v") == 0) || (strcmp(argv[i], "-version") == 0) || (strcmp(argv[i], "--version") == 0)) {
            ephem_report(version_string);
            return 0;
        } else if ((strcmp(argv[i], "-h") == 0) || (strcmp(argv[i], "-help") == 0) ||
                   (strcmp(argv[i], "--help") == 0)) {
            ephem_report(help_string);
            return 0;
        } else {
            snprintf(temp_err_string, FNAME_LENGTH,
                     "Received switch '%s' which was not recognised.\n"
                     "Type 'ephem.bin -help' for a list of available command-line options.",
                     argv[i]);
            ephem_error(temp_err_string);
            return 1;
        }
    }

    // Check that we have been provided with exactly one filename on the command line
    if (inputs_read != N_INPUTS) {
        snprintf(temp_err_string, FNAME_LENGTH,
                 "asteroids.bin should be provided %d numeric values on the command line. Only %d were received. "
                 "Type 'ephem.bin -help' for a list of available command-line options.",
                 N_INPUTS, inputs_read);
        ephem_error(temp_err_string);
        return 1;
    }

    // Set up default settings
    if (DEBUG) ephem_log("Setting up default ephemeris parameters.");
    settings_default(&s_model);
    settings_process(&s_model);

    // Work out Julian day limits for search
    jd_min = julian_day((int) input[0], (int) input[1], (int) input[2], 12, 0, 0, &i, temp_err_string);
    jd_max = julian_day((int) input[3], (int) input[4], (int) input[5], 12, 0, 0, &i, temp_err_string);
    mag_limit = input[6];

    // Open asteroid database
    orbitalElements_asteroids_init();

    if (DEBUG) {
        snprintf(temp_err_string, FNAME_LENGTH, "Read asteroid database; got %d members.", asteroid_count);
        ephem_log(temp_err_string);
    }

    // Read contents of the asteroid database
    fseek(asteroid_database_file, asteroid_database_offset, SEEK_SET);
    dcf_fread((void *) asteroid_database, sizeof(orbitalElements), asteroid_count, asteroid_database_file,
              asteroid_database_filename, __FILE__, __LINE__);
    memset(asteroid_database_items_loaded, 1, asteroid_count);

    // Malloc arrays for keeping track of solar distance of asteroids
    sun_ang_dist_1 = (double *) lt_malloc(asteroid_count * sizeof(double));
    sun_ang_dist_2 = (double *) lt_malloc(asteroid_count * sizeof(double));
    earth_dist_1 = (double *) lt_malloc(asteroid_count * sizeof(double));
    earth_dist_2 = (double *) lt_malloc(asteroid_count * sizeof(double));
    mag1 = (double *) lt_malloc(asteroid_count * sizeof(double));
    mag2 = (double *) lt_malloc(asteroid_count * sizeof(double));
    selected = (int *) lt_malloc(asteroid_count * sizeof(int));
    if ((sun_ang_dist_1 == NULL) || (sun_ang_dist_2 == NULL) || (earth_dist_1 == NULL) || (earth_dist_2 == NULL) ||
        (mag1 == NULL) || (mag2 == NULL) || (selected == NULL)) {
        ephem_fatal(__FILE__, __LINE__, "Malloc fail");
        exit(1);
    }

    // Set up some dummy initial values
    for (i = 0; i < asteroid_count; i++) sun_ang_dist_1[i] = 800.;
    for (i = 0; i < asteroid_count; i++) sun_ang_dist_2[i] = 900.;
    for (i = 0; i < asteroid_count; i++) earth_dist_1[i] = 900.;
    for (i = 0; i < asteroid_count; i++) earth_dist_2[i] = 800.;
    for (i = 0; i < asteroid_count; i++) mag1[i] = 900.;
    for (i = 0; i < asteroid_count; i++) mag2[i] = 800.;

    if (DEBUG) {
        snprintf(temp_err_string, FNAME_LENGTH, "Starting pass 1.");
        ephem_log(temp_err_string);
    }
    scan_for_oppositions(&s_model, jd_min, jd_max, jd_step_pass_1, mag_limit, 0, sun_ang_dist_1, sun_ang_dist_2,
                         earth_dist_1,
                         earth_dist_2, mag1, mag2, NULL, selected);
    if (DEBUG) {
        snprintf(temp_err_string, FNAME_LENGTH, "Starting pass 2.");
        ephem_log(temp_err_string);
    }
    scan_for_oppositions(&s_model, jd_min, jd_max, jd_step_pass_2, mag_limit, 1, sun_ang_dist_1, sun_ang_dist_2,
                         earth_dist_1,
                         earth_dist_2, mag1, mag2, selected, NULL);

    // Finish off
    lt_freeAll(0);
    lt_memoryStop();
    if (DEBUG) ephem_log("Terminating normally.");
    return 0;
}
