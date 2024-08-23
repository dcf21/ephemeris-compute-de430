// main.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#include <gsl/gsl_const_mksa.h>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_math.h>

#include "argparse/argparse.h"

#include "coreUtils/asciiDouble.h"
#include "coreUtils/strConstants.h"
#include "coreUtils/errorReport.h"

#include "ephemCalc/constellations.h"
#include "ephemCalc/jpl.h"
#include "ephemCalc/meeus.h"
#include "ephemCalc/orbitalElements.h"
#include "ephemCalc/magnitudeEstimate.h"
#include "mathsTools/precess_equinoxes.h"

#include "listTools/ltMemory.h"

#include "settings/settings.h"

#define N_PARAMETERS 17
static double buffer[N_PARAMETERS * MAX_OBJECTS];

static const char *const usage[] = {
        "ephem.bin [options] [[--] args]",
        "ephem.bin [options]",
        NULL,
};

// Main entry point to compute an ephemeris, with parameters described by a settings structure
void compute_ephemeris(settings *s) {
    FILE *output = stdout;

    // Initial processing of settings for this ephemeris
    settings_process(s);

    // Loop over all the time points in the ephemeris
    const int steps_total = (int)ceil((s->jd_max - s->jd_min) / s->jd_step);
    for (int step_count=0; step_count<steps_total; step_count++) {
        const double jd = s->jd_min + step_count * s->jd_step;  // TT

        // When producing a text-based ephemeris, the first column in Julian day number (TT)
        // Binary ephemerides have no JD column to save space.
        if (!s->output_binary) fprintf(output, "%.12f   ", jd);

        // Compute ephemeris
        int i;
#pragma omp parallel for shared(output) private(i)
        for (i = 0; i < s->objects_count; i++) {
            const int o = i * N_PARAMETERS;
            double ra = 0, dec = 0, x = 0, y = 0, z = 0;
            double mag = 0, phase = 0, ang_size = 0, phy_size = 0, albedo = 0;
            double sun_dist = 0, earth_dist = 0, sun_ang_dist = 0, theta_eso = 0;
            double ecliptic_longitude = 0, ecliptic_latitude = 0, ecliptic_distance = 0;

            // If the <use_orbital_elements> is 0, we use DE430
            if (s->use_orbital_elements == 0)
                jpl_computeEphemeris(s, s->body_id[i], jd, &x, &y, &z, &ra, &dec, &mag, &phase, &ang_size, &phy_size,
                                     &albedo,
                                     &sun_dist, &earth_dist, &sun_ang_dist, &theta_eso, &ecliptic_longitude,
                                     &ecliptic_latitude, &ecliptic_distance);

            // If the <use_orbital_elements> is 2, we use Jean Meeus's algorithms (NOT IMPLEMENTED!!!)
            else if (s->use_orbital_elements == 2)
                meeus_computeEphemeris(s, s->body_id[i], jd, &x, &y, &z, &ra, &dec, &mag, &phase, &ang_size, &phy_size,
                                       &albedo,
                                       &sun_dist, &earth_dist, &sun_ang_dist, &theta_eso, &ecliptic_longitude,
                                       &ecliptic_latitude, &ecliptic_distance);

            // If the <use_orbital_elements> is 1, we use orbital elements
            else if (s->use_orbital_elements == 1)
                orbitalElements_computeEphemeris(s, s->body_id[i], jd, &x, &y, &z, &ra, &dec, &mag, &phase, &ang_size,
                                                 &phy_size,
                                                 &albedo, &sun_dist, &earth_dist, &sun_ang_dist, &theta_eso,
                                                 &ecliptic_longitude, &ecliptic_latitude,
                                                 &ecliptic_distance);

            // Negative output formats use ecliptic coordinates, not RA and Declination
            if (s->output_format < 0) {
                double x2, y2, z2;
                double epsilon = (23. + 26. / 60. + 21.448 / 3600.) / 180. * M_PI; // Meeus (22.2)

                // negative x-axis points to the vernal equinox; (y,z) get tipped up by 23.5 degrees from (ra,dec)
                // to equatorial coordinates
                x2 = x;
                y2 = cos(epsilon) * y + sin(epsilon) * z;
                z2 = -sin(epsilon) * y + cos(epsilon) * z;
                x = x2;
                y = y2;
                z = z2;
            }

            // Convert ecliptic longitude we output to epoch of observation
            double eclTo_lat, eclTo_lng;
            precess(2451545.0, jd, ecliptic_longitude, ecliptic_latitude, &eclTo_lng, &eclTo_lat);


            buffer[o + 0] = x;
            buffer[o + 1] = y;
            buffer[o + 2] = z;
            buffer[o + 3] = ra;
            buffer[o + 4] = dec;
            buffer[o + 5] = mag;
            buffer[o + 6] = phase;
            buffer[o + 7] = ang_size;
            buffer[o + 8] = phy_size;
            buffer[o + 9] = albedo;
            buffer[o + 10] = sun_dist;
            buffer[o + 11] = earth_dist;
            buffer[o + 12] = sun_ang_dist;
            buffer[o + 13] = theta_eso;
            buffer[o + 14] = eclTo_lng; // ecliptic longitude in epoch of jd, not J2000.0
            buffer[o + 15] = ecliptic_distance;
            buffer[o + 16] = eclTo_lat;

            // fix ecliptic longitude for precession of the equinoxes
            if (buffer[o + 14] > M_PI)  buffer[o + 14] -= 2 * M_PI;
            if (buffer[o + 14] < -M_PI) buffer[o + 14] += 2 * M_PI;
        }

        // Produce output to file -- loop over objects producing a set of columns for each
        for (i = 0; i < s->objects_count; i++) {
            const int o = i * N_PARAMETERS;

            // Produce text-based output
            if (!s->output_binary) {
                //-1 - x y z   (ecliptic)
                // 0 - x y z   (J2000)
                // 1 - ra dec  (radians)
                // 2 - x y z ra dec mag phase AngSize
                // 3 - x y z ra dec mag phase AngSize physical_size albedo

                // Write XYZ coordinates (in all modes but 1)
                if (s->output_format != 1) {
                    fprintf(output, "%12.9f %12.9f %12.9f   ", buffer[o + 0], buffer[o + 1], buffer[o + 2]);
                }

                // Write RA and Dec in modes 1,2,3
                if (s->output_format >= 1) {
                    fprintf(output, "%12.9f %12.9f   ", buffer[o + 3], buffer[o + 4]);
                }

                // Write magnitude, phase and angular size in modes 2,3
                if (s->output_format >= 2) {
                    fprintf(output, "%6.3f %7.4f %12.9f   ", buffer[o + 5], buffer[o + 6], buffer[o + 7]);
                }

                // Write physical size, albedo, sun_dist, earth_dist, sun_ang_dist, theta_edo, eclLng, eclDist, eclLat
                if (s->output_format >= 3) {
                    fprintf(output, "%12.6e %8.5f %12.9f %12.9f %12.9f %12.9f %12.9f %12.9f %12.9f  ", buffer[o + 8],
                            buffer[o + 9], buffer[o + 10], buffer[o + 11], buffer[o + 12], buffer[o + 13],
                            buffer[o + 14], buffer[o + 15], buffer[o + 16]);
                }

                // Write the name of the constellation the object is in, in the final column
                if (s->output_constellations) {
                    fprintf(output, "%s ", constellations_fetch(buffer[o + 3], buffer[o + 4]));
                }
            }

            // Produce binary output
            else {
                if (s->output_format != 1) fwrite((void *) (buffer + o + 0), sizeof(double), 3, output);
                if (s->output_format >= 1) fwrite((void *) (buffer + o + 3), sizeof(double), 2, output);
                if (s->output_format >= 2) fwrite((void *) (buffer + o + 5), sizeof(double), 3, output);
                if (s->output_format >= 3) fwrite((void *) (buffer + o + 8), sizeof(double), 9, output);
                if (s->output_constellations) fprintf(output, "%s ", constellations_fetch(buffer[o + 3], buffer[o + 4]));
            }
        }
        if (!s->output_binary) fprintf(output, "\n");
    }

    if (DEBUG) {
        char line[FNAME_LENGTH];
        strcpy(line, "Finished computing ephemeris.");
        ephem_log(line);
    }
    fclose(output);
    settings_close(s);
}

int main(int argc, const char **argv) {
    settings ephemeris_settings;

    // Initialise sub-modules
    if (DEBUG) ephem_log("Initialising ephemeris computer.");
    lt_memoryInit(&ephem_error, &ephem_log);
    constellations_init();

    // Turn off GSL's automatic error handler
    gsl_set_error_handler_off();

    // Set up default settings
    if (DEBUG) ephem_log("Setting up default ephemeris parameters.");
    settings_default(&ephemeris_settings);

    // Scan commandline options for any switches
    struct argparse_option options[] = {
            OPT_HELP(),
            OPT_GROUP("Basic options"),
            OPT_FLOAT('a', "jd_min", &ephemeris_settings.jd_min,
                    "The Julian day number at which the ephemeris should begin; TT"),
            OPT_FLOAT('b', "jd_max", &ephemeris_settings.jd_max,
                    "The Julian day number at which the ephemeris should end; TT"),
            OPT_FLOAT('s', "jd_step", &ephemeris_settings.jd_step,
                    "The interval between the lines in the ephemeris, in days"),
            OPT_FLOAT('e', "epoch", &ephemeris_settings.ra_dec_epoch,
                      "The epoch of the RA/Dec coordinate system, e.g. 2451545.0 for J2000"),
            OPT_INTEGER('r', "output_format", &ephemeris_settings.output_format,
                    "The output format for the ephemeris. See README.md."),
            OPT_INTEGER('o', "use_orbital_elements", &ephemeris_settings.use_orbital_elements,
                    "Set the either 0 (use DE430) or 1 (use orbital elements)"),
            OPT_INTEGER('b', "output_binary", &ephemeris_settings.output_binary,
                    "Set to either 0 (text output) or 1 (binary output)"),
            OPT_INTEGER('c', "output_constellations", &ephemeris_settings.output_constellations,
                    "Set to either 0 (no column for constellation names) or 1"),
            OPT_STRING('o', "objects", &ephemeris_settings.objects_input_list,
                    "The list of objects to produce ephemerides for. See README.md."),
            OPT_END(),
    };

    struct argparse argparse;
    argparse_init(&argparse, options, usage, 0);
    argparse_describe(&argparse,
                      "\nCompute an ephemeris for a solar system body",
                      "\n");
    argc = argparse_parse(&argparse, argc, argv);

    if (argc != 0) {
        int i;
        for (i = 0; i < argc; i++) {
            printf("Error: unparsed argument <%s>\n", *(argv + i));
        }
        ephem_fatal(__FILE__, __LINE__, "Unparsed arguments");
    }

    // Create ephemeris
    compute_ephemeris(&ephemeris_settings);

    lt_freeAll(0);
    lt_memoryStop();
    if (DEBUG) ephem_log("Terminating normally.");
    return 0;
}

