// asteroids.c
// Dominic Ford
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

// This is a simple tool for automatically scanning for moments when asteroids are at opposition

// Usage:
// asteroids.bin <YearMin> <MonthMin> <DayMin>  <YearMax> <MonthMax> <DayMax>  <LimitingMagnitude>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gsl/gsl_errno.h>

#include "coreUtils/asciiDouble.h"
#include "coreUtils/strConstants.h"
#include "coreUtils/errorReport.h"

#include "ephemCalc/constellations.h"
#include "ephemCalc/jpl.h"
#include "ephemCalc/orbitalElements.h"

#include "listTools/ltMemory.h"

#include "mathsTools/julianDate.h"

#include "settings/settings.h"

// Link to the catalogue of asteroids in <astorb.dat>, which is in <orbitalElements.c>
extern orbitalElementSet asteroid_database;

// The number of floats we expect to receive on the command-line
#define N_INPUTS 7

//! fetch_orbital_elements - Fetch the best set of orbital elements to use for an asteroid at the requested epoch
//! \param [in] object_index - The number of the asteroid to query.
//! \param [in] jd - The Julian date of the epoch for which orbital elements are required.
//! \param [out] output - Output <orbitalElements> structure.
//! \return - 1 on success; 0 on failure.

int fetch_orbital_elements(int object_index, double jd, orbitalElements *output) {
    orbitalElements orbital_elements_0 = orbitalElements_nullOrbitalElements();
    orbitalElements orbital_elements_1 = orbitalElements_nullOrbitalElements();
    double weight_0 = 0;
    double weight_1 = 0;

    // Fetch the best set of orbital elements to use for this object at the requested epoch
    const int item_count = orbitalElements_fetch(
        1, object_index, jd,
        &orbital_elements_0, &weight_0, &orbital_elements_1, &weight_1);
    if (item_count < 1) return 0;

    // Use the set of orbital elements with the greatest weight.
    *output = (weight_0 > 0.5) ? orbital_elements_0 : orbital_elements_1;
    return 1;
}

//! file_event - Produce a line of output text, describing an event we have found.
//! \param [in] report - Boolean flag indicating whether to write report text.
//! \param [in] object_index - The number of the asteroid the event relates to.
//! \param [in] event_type - String containing a one-word description of the type of event being recorded.
//! \param [in] jd - The Julian date of the event.
//! \param [in] mag - The magnitude of the asteroid at the epoch of the event.
//! \param [in] earth_dist - The distance of the asteroid from Earth (AU)
//! \param [in] ra - The RA of the asteroid (J2000.0; radians; relative to the geocentre)
//! \param [in] dec - The declination of the asteroid (J2000.0; radians; relative to the geocentre)

void file_event(const int report, const int object_index, const char *event_type,
                const double jd, const double mag, const double earth_dist,
                const double ra, const double dec) {
    int year, month, day, hour, min, j;
    double sec;

    // Fetch asteroid's orbital elements
    orbitalElements e;
    const int status = fetch_orbital_elements(object_index, jd, &e);
    if (status != 1) return;

    // Check that names have been loaded
    if (asteroid_database.object_names == NULL) {
        ephem_fatal(__FILE__, __LINE__, "Object names have not been loaded.");
        exit(1);
    }

    // Fetch asteroid's name
    const char *name = asteroid_database.object_names[object_index].name;

    // Create a copy of the asteroid's name, with the spaces replaced with @ signs
    char name_no_spaces[1024];
    for (j = 0; name[j] != '\0'; j++) {
        name_no_spaces[j] = name[j];
        if (name[j] == ' ') name_no_spaces[j] = '@';
    }
    name_no_spaces[j] = '\0';

    // Write the output within an <omp critical> block so that multiple threads don't write at the same time
#pragma omp critical (file_asteroid_event)
    {
        inv_julian_day(jd, &year, &month, &day, &hour, &min, &sec, &j, temp_err_string);
        snprintf(temp_err_string, FNAME_LENGTH,
                 "%10.1f %04d %02d %02d %02d %02d %s   %6.1f %8.3f   %10.6f %10.6f %s   "
                 "%07d %s %.16e %.16e %.16e %.16e %.16e %.16e %.16e",
                 jd, year, month, day, hour, min, event_type, mag, earth_dist, ra, dec,
                 constellations_fetch(ra, dec, 1), object_index, name_no_spaces,
                 e.semiMajorAxis, e.eccentricity, e.longAscNode, e.inclination,
                 e.argumentPerihelion, e.meanAnomaly, e.epochOsculation);
        if (report) {
            fprintf(stdout, "%s\n", temp_err_string);
            fflush(stdout);
        }
        if (DEBUG) ephem_log(temp_err_string);
    }
}

//! scan_for_oppositions - Scan over the requested search period for asteroids at opposition, above the magnitude limit.
//! \param s - Ephemeris settings.
//! \param jd_min - Start of search period (JD).
//! \param jd_max - End of search period (JD).
//! \param jd_step - Time resolution of search (days).
//! \param mag_limit - Only report opposition events that are brighter than this magnitude limit.
//! \param report - Boolean flag indicating whether to produce text output reporting events.
//! \param sun_ang_dist_1
//! \param sun_ang_dist_2
//! \param earth_dist_1
//! \param earth_dist_2
//! \param mag1
//! \param mag2
//! \param selected_in
//! \param selected_out
void scan_for_oppositions(settings *s, double jd_min, double jd_max, double jd_step, double mag_limit, int report,
                          double *sun_ang_dist_1, double *sun_ang_dist_2, double *earth_dist_1, double *earth_dist_2,
                          double *mag1, double *mag2, const int *selected_in, int *selected_out) {
    int so_count = 0;
    int object_count, loop_iter;
    double jd;
    int max_iters; // The highest object index we need to use

    // If we have been passed a <selected_in> array, then we only search for asteroids in that list.
    // If there is no <selected_in> array, then we search over all the numbered asteroids.
    if (selected_in == NULL) {
        // Search through all the numbered asteroids.
        max_iters = asteroid_database.object_count - 1;
    } else {
        // Search through the list of asteroids specified in the array <selected_in>.
        for (int k = 0; 1; k++) {
            int i = selected_in[k];
            if (i < 0) {
                max_iters = k;
                break;
            }
        }
    }

    // Construct an array of which objects have secure orbits
    int *secure_orbit_array = (int *) malloc(max_iters * sizeof(int));
    if (secure_orbit_array == NULL) {
        ephem_fatal(__FILE__, __LINE__, "Malloc fail.");
        exit(1);
    }

    for (object_count = 0; object_count < max_iters; object_count++) {
        // Work out what asteroid number we are working on
        int object_index;

        if (selected_in == NULL) {
            // If we're not passed a list of asteroids, work on all asteroids in turn
            object_index = object_count + 1;
        } else {
            // ... otherwise iterate through the provided list
            object_index = selected_in[object_count];
        }

        secure_orbit_array[object_count] = orbitalElements_binary_getSecureFlag(&asteroid_database, object_index);
    }

    // Loop over the search period, with specified stride
    for (jd = jd_min, loop_iter = 0; jd <= jd_max; jd += jd_step, loop_iter++) {
        //if (DEBUG) {
        // snprintf(temp_err_string, FNAME_LENGTH, "Starting work on jd=%.1f", jd);
        // ephem_log(temp_err_string);
        // }

        // Loop over asteroids
#pragma omp parallel for shared(jd, loop_iter, max_iters, so_count) private(object_count)
        for (object_count = 0; object_count < max_iters; object_count++) {
            // Work out what asteroid number we are working on
            int object_index;

            if (selected_in == NULL) {
                // If we're not passed a list of asteroids, work on all asteroids in turn
                object_index = object_count + 1;
            } else {
                // ... otherwise iterate through the provided list
                object_index = selected_in[object_count];
            }

            // Only work on asteroids with secure orbits
            const int secure_orbit = secure_orbit_array[object_count];
            if (secure_orbit) {
                // Fetch asteroid's current circumstances
                double ra = 0, dec = 0, x = 0, y = 0, z = 0;
                double mag = 0, phase = 0, ang_size = 0, phy_size = 0, albedo = 0, sun_dist = 0;
                double earth_dist = 0, sun_ang_dist = 0, theta_eso = 0;
                double ecliptic_longitude = 0, ecliptic_latitude = 0, ecliptic_distance = 0;

                orbitalElements_computeEphemeris(
                    ASTEROIDS_OFFSET + object_index, jd, &x, &y, &z, &ra, &dec, &mag, &phase,
                    &ang_size,
                    &phy_size,
                    &albedo, &sun_dist, &earth_dist, &sun_ang_dist, &theta_eso,
                    &ecliptic_longitude, &ecliptic_latitude,
                    &ecliptic_distance, s->ra_dec_epoch,
                    0, 0, 0);

                // Check if asteroid is bright enough to be of interest
                if ((mag < mag_limit) && (loop_iter > 2)) {
                    // Add this asteroid to the output catalogue of potentially interesting asteroids
                    if (selected_out != NULL) {
                        int got = 0;
#pragma omp critical (select_asteroid)
                        {
                            for (int c = 0; c < so_count; c++)
                                if (selected_out[c] == object_index) {
                                    got = 1;
                                    break;
                                }
                            if (!got) selected_out[so_count++] = object_index;
                        }
                    }

                    // Check whether this asteroid is at opposition
                    if ((sun_ang_dist_1[object_index] > sun_ang_dist) &&
                        (sun_ang_dist_1[object_index] > sun_ang_dist_2[object_index])) {
                        file_event(report, object_index, "Opposition",
                                   jd - jd_step, mag, earth_dist, ra, dec);
                    }

                    // Check whether this asteroid is at apogee
                    if ((earth_dist_1[object_index] < earth_dist) &&
                        (earth_dist_1[object_index] < earth_dist_2[object_index])) {
                        file_event(report, object_index, "Apogee    ",
                                   jd - jd_step, mag, earth_dist, ra, dec);
                    }

                    // Check whether this asteroid is at peak brightness
                    if ((mag1[object_index] < mag) && (mag1[object_index] < mag2[object_index])) {
                        file_event(report, object_index, "PeakMag   ",
                                   jd - jd_step, mag, earth_dist, ra, dec);
                    }
                }

                // Store the circumstances of the asteroid in a bunch of arrays, for comparison with the next
                // time sample
                sun_ang_dist_2[object_index] = sun_ang_dist_1[object_index];
                sun_ang_dist_1[object_index] = sun_ang_dist;
                earth_dist_2[object_index] = earth_dist_1[object_index];
                earth_dist_1[object_index] = earth_dist;
                mag2[object_index] = mag1[object_index];
                mag1[object_index] = mag;
            }
        }
    }

    // Free array of secure orbit flags
    if (secure_orbit_array != NULL) {
        free(secure_orbit_array);
    }

    // If we are making a catalogue of interesting asteroids, then terminate the catalogue with a negative value now
    if (selected_out != NULL) {
        selected_out[so_count] = -1;
        if (DEBUG) {
            snprintf(temp_err_string, FNAME_LENGTH, "Selected %d objects.", so_count);
            ephem_log(temp_err_string);
        }
    }
}

//! Main entry point for searching for asteroid oppositions within a specified time interval.

int main(int argc, char **argv) {
    char help_string[LSTR_LENGTH], version_string[FNAME_LENGTH], version_string_underline[FNAME_LENGTH];
    int inputs_read = 0;
    settings s_model;
    double input[N_INPUTS];
    const int use_de_number = 440;

    // Pass 1: Step through the search period at 4-day resolution, compiling a list of asteroids that exceed the magnitude limit
    const double jd_step_pass_1 = 4;

    // Pass 2: Step through the search period at 30-sec resolution, only looking at bright asteroids
    const double jd_step_pass_2 = 1. / 24. / 3600. * 30.;

    // Initialise sub-modules
    if (DEBUG) ephem_log("Initialising asteroid opposition search.");
    lt_memoryInit(&ephem_error, &ephem_log);
    jpl_setEphemerisNumber(use_de_number);
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

    // Read command line arguments and extract search settings
    for (int i = 1; i < argc; i++) {
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

    // Check that we have been provided with the right number of floats on the command line
    if (inputs_read != N_INPUTS) {
        snprintf(temp_err_string, FNAME_LENGTH,
                 "asteroids.bin should be provided %d numeric values on the command line. Only %d were received. "
                 "Type 'ephem.bin -help' for a list of available command-line options.",
                 N_INPUTS, inputs_read);
        ephem_error(temp_err_string);
        return 1;
    }

    // Set up default settings
    if (DEBUG) {
        ephem_log("Setting up default ephemeris parameters.");
    }
    {
        int status = 0;
        char error_text[LSTR_LENGTH] = "\0";
        settings_default(&s_model);
        settings_process(&s_model, &status, error_text);
        if (status) {
            ephem_error(error_text);
        }
    }

    // Work out Julian day limits for search
    int status = 0;
    const double jd_min = julian_day((int) input[0], (int) input[1], (int) input[2], 12, 0, 0, &status,
                                     temp_err_string);
    const double jd_max = julian_day((int) input[3], (int) input[4], (int) input[5], 12, 0, 0, &status,
                                     temp_err_string);
    const double mag_limit = input[6];

    // Open asteroid database
    asteroid_database.cache_in_memory = 1; // This is memory intensive, but results in a 10x speed-up
    orbitalElements_asteroids_init(1);

    if (DEBUG) {
        snprintf(temp_err_string, FNAME_LENGTH, "Read asteroid database; got %d members.",
                 asteroid_database.object_count);
        ephem_log(temp_err_string);
    }

    // Malloc arrays for keeping track of the circumstances of each asteroid
    double *sun_ang_dist_1 = (double *) malloc(asteroid_database.object_count * sizeof(double));
    double *sun_ang_dist_2 = (double *) malloc(asteroid_database.object_count * sizeof(double));
    double *earth_dist_1 = (double *) malloc(asteroid_database.object_count * sizeof(double));
    double *earth_dist_2 = (double *) malloc(asteroid_database.object_count * sizeof(double));
    double *mag1 = (double *) malloc(asteroid_database.object_count * sizeof(double));
    double *mag2 = (double *) malloc(asteroid_database.object_count * sizeof(double));
    int *selected = (int *) malloc(asteroid_database.object_count * sizeof(int));
    if ((sun_ang_dist_1 == NULL) || (sun_ang_dist_2 == NULL) || (earth_dist_1 == NULL) || (earth_dist_2 == NULL) ||
        (mag1 == NULL) || (mag2 == NULL) || (selected == NULL)) {
        ephem_fatal(__FILE__, __LINE__, "Malloc fail");
        exit(1);
    }

    // Seed these arrays with some dummy initial values
    for (int i = 0; i < asteroid_database.object_count; i++) sun_ang_dist_1[i] = 800.;
    for (int i = 0; i < asteroid_database.object_count; i++) sun_ang_dist_2[i] = 900.;
    for (int i = 0; i < asteroid_database.object_count; i++) earth_dist_1[i] = 900.;
    for (int i = 0; i < asteroid_database.object_count; i++) earth_dist_2[i] = 800.;
    for (int i = 0; i < asteroid_database.object_count; i++) mag1[i] = 900.;
    for (int i = 0; i < asteroid_database.object_count; i++) mag2[i] = 800.;

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

    // Free the memory buffers we assigned
    if (sun_ang_dist_1 != NULL) free(sun_ang_dist_1);
    if (sun_ang_dist_2 != NULL) free(sun_ang_dist_2);
    if (earth_dist_1 != NULL) free(earth_dist_1);
    if (earth_dist_2 != NULL) free(earth_dist_2);
    if (mag1 != NULL) free(mag1);
    if (mag2 != NULL) free(mag2);
    if (selected != NULL) free(selected);

    if (DEBUG) ephem_log("Terminating normally.");
    return 0;
}
