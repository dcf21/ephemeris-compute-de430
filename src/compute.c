// compute.c
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
#include <string.h>
#include <math.h>

#include <gsl/gsl_math.h>

#include "compute.h"

#include "coreUtils/asciiDouble.h"
#include "coreUtils/strConstants.h"
#include "coreUtils/errorReport.h"

#include "ephemCalc/constellations.h"
#include "ephemCalc/jpl.h"
#include "ephemCalc/magnitudeEstimate.h"
#include "ephemCalc/meeus.h"
#include "ephemCalc/orbitalElements.h"
#include "mathsTools/deltaT.h"
#include "mathsTools/julianDate.h"
#include "mathsTools/precession.h"

#include "settings/settings.h"

// Buffer used for building a single row of output
#define N_PARAMETERS 19
static double buffer[N_PARAMETERS * MAX_OBJECTS];

//! compute_ephemeris_time_point - Compute the positions of all requested objects, at a single time point.
//! @param [in] s - The ephemeris settings.
//! @param [in] dt_calc - Delta-T computation instance.
//! @param [out] output - The output stream to which we write a single row of results.
//! @param [in] jd_tt - The Julian date of the time point we are calculating (TT).
//! @return - The number of bytes written to the output stream.

int compute_ephemeris_time_point(const settings *s, const DeltaTCalculator *dt_calc, FILE *output, const double jd_tt) {
    // Keep track of how much data we have written
    int bytes_written = 0;

    // When producing a text-based ephemeris, the first column in Julian day number (TT)
    // Binary ephemerides have no JD column to save space.
    if (!s->output_binary) {
        bytes_written += fprintf(output, "%.12f   ", jd_tt);
    }

    // Compute ephemeris
    int i;
#pragma omp parallel for shared(output) private(i)
    for (i = 0; i < s->objects_count; i++) {
        const int o = i * N_PARAMETERS;
        double ra = 0, dec = 0, x = 0, y = 0, z = 0;
        double mag = 0, phase = 0, ang_size = 0, phy_size = 0, albedo = 0;
        double sun_dist = 0, earth_dist = 0, sun_ang_dist = 0, theta_eso = 0;
        double ecliptic_longitude = 0, ecliptic_latitude = 0, ecliptic_distance = 0;

        if (s->body_id[i] < 0) {
            // Unrecognised object
            x = y = z = ra = dec = mag = phase = ang_size = phy_size = albedo = GSL_NAN;
            sun_dist = earth_dist = sun_ang_dist = theta_eso = GSL_NAN;
            ecliptic_longitude = ecliptic_latitude = ecliptic_distance = GSL_NAN;
        } else if (s->use_orbital_elements == 0) {
            // If the <use_orbital_elements> is 0, we use DE4xx
            jpl_computeEphemeris(s->body_id[i], jd_tt, &x, &y, &z, &ra, &dec, &mag, &phase, &ang_size, &phy_size,
                                 &albedo,
                                 &sun_dist, &earth_dist, &sun_ang_dist, &theta_eso, &ecliptic_longitude,
                                 &ecliptic_latitude, &ecliptic_distance, s->ra_dec_epoch,
                                 s->enable_topocentric_correction,
                                 s->latitude, s->longitude);
        } else if (s->use_orbital_elements == 2) {
            // If the <use_orbital_elements> is 2, we use Jean Meeus's algorithms (NOT IMPLEMENTED!!!)
            meeus_computeEphemeris(s->body_id[i], jd_tt, &x, &y, &z, &ra, &dec, &mag, &phase, &ang_size, &phy_size,
                                   &albedo,
                                   &sun_dist, &earth_dist, &sun_ang_dist, &theta_eso, &ecliptic_longitude,
                                   &ecliptic_latitude, &ecliptic_distance, s->ra_dec_epoch,
                                   s->enable_topocentric_correction,
                                   s->latitude, s->longitude);
        } else if (s->use_orbital_elements == 1) {
            // If the <use_orbital_elements> is 1, we use orbital elements
            orbitalElements_computeEphemeris(s->body_id[i], jd_tt, &x, &y, &z, &ra, &dec, &mag, &phase, &ang_size,
                                             &phy_size,
                                             &albedo, &sun_dist, &earth_dist, &sun_ang_dist, &theta_eso,
                                             &ecliptic_longitude, &ecliptic_latitude,
                                             &ecliptic_distance, s->ra_dec_epoch,
                                             s->enable_topocentric_correction,
                                             s->latitude, s->longitude);
        }

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
        double ecl_lat_epoch, ecl_lng_epoch;
        ecl_switch_epoch(2451545.0, jd_tt,
            ecliptic_longitude, ecliptic_latitude,
            &ecl_lng_epoch, &ecl_lat_epoch);

        buffer[o + 0] = x; // AU
        buffer[o + 1] = y; // AU
        buffer[o + 2] = z; // AU
        buffer[o + 3] = ra; // radians
        buffer[o + 4] = dec; // radians
        buffer[o + 5] = mag;
        buffer[o + 6] = phase; // 0-1
        buffer[o + 7] = ang_size; // diameter; arcseconds
        buffer[o + 8] = phy_size; // diameter; metres
        buffer[o + 9] = albedo; // 0-1
        buffer[o + 10] = sun_dist; // AU
        buffer[o + 11] = earth_dist; // AU
        buffer[o + 12] = sun_ang_dist; // radians
        buffer[o + 13] = theta_eso; // radians
        buffer[o + 14] = ecl_lng_epoch; // radians; ecliptic longitude in epoch of jd, not J2000.0
        buffer[o + 15] = ecliptic_distance; // radians
        buffer[o + 16] = ecl_lat_epoch; // radians; ecliptic latitude in epoch of jd, not J2000.0
        buffer[o + 17] = sidereal_time_jd(dt_calc, jd_tt); // radians
        buffer[o + 18] = delta_t(dt_calc, jd_tt); // seconds

        // fix ecliptic longitude for precession of the equinoxes
        if (buffer[o + 14] > M_PI) buffer[o + 14] -= 2 * M_PI;
        if (buffer[o + 14] < -M_PI) buffer[o + 14] += 2 * M_PI;
    }

    // Produce output to file -- loop over objects producing a set of columns for each
    for (i = 0; i < s->objects_count; i++) {
        const int o = i * N_PARAMETERS;

        if (!s->output_binary) {
            // Produce text-based output

            // Supported output data formats (text):

            //-1 - jd x y z   (ecliptic)                                      [ 4 columns]
            // 0 - jd x y z   (J2000)                                         [ 4 columns]
            // 1 - jd ra dec  (radians)                                       [ 3 columns]
            // 2 - jd x y z ra dec mag phase AngSize                          [ 9 columns]
            // 3 - jd x y z ra dec mag phase AngSize physical_size albedo ... [20 columns]

            // if (s->output_constellation) is set, one additional column is output (variable width)

            // Supported output data formats (binary):

            //-1 - x y z   (ecliptic)                                      [ 3 columns]
            // 0 - x y z   (J2000)                                         [ 3 columns]
            // 1 - ra dec  (radians)                                       [ 2 columns]
            // 2 - x y z ra dec mag phase AngSize                          [ 8 columns]
            // 3 - x y z ra dec mag phase AngSize physical_size albedo ... [19 columns]

            // if (s->output_constellation) is set, one additional column is output (8 bytes)

            // Write: XYZ coordinates (in all modes but 1)
            if (s->output_format != 1) {
                bytes_written += fprintf(output,
                                         "%12.16f %12.16f %12.16f   ",
                                         buffer[o + 0], buffer[o + 1], buffer[o + 2]);
            }

            // Write: RA and Dec in modes 1,2,3
            if (s->output_format >= 1) {
                bytes_written += fprintf(output,
                                         "%12.16f %12.16f   ",
                                         buffer[o + 3], buffer[o + 4]);
            }

            // Write: magnitude, phase and angular size in modes 2,3
            if (s->output_format >= 2) {
                bytes_written += fprintf(output,
                                         "%6.10f %7.10f %12.10f   ",
                                         buffer[o + 5], buffer[o + 6], buffer[o + 7]);
            }

            // Write: physical size, albedo, sun_dist, earth_dist, sun_ang_dist, theta_edo, eclLng, eclDist, eclLat,
            //        sidereal time, delta_T
            if (s->output_format >= 3) {
                bytes_written += fprintf(
                    output,
                    "%12.10e %8.10f %12.12f %12.12f %12.12f %12.12f %12.12f %12.12f %12.12f %12.12f %12.12f  ",
                    buffer[o + 8], buffer[o + 9],
                    buffer[o + 10], buffer[o + 11], buffer[o + 12], buffer[o + 13],
                    buffer[o + 14], buffer[o + 15], buffer[o + 16],
                    buffer[o + 17], buffer[o + 18]);
            }

            // Write the name of the constellation the object is in, in the final column
            if (s->output_constellations) {
                bytes_written += fprintf(output, "%s ", constellations_fetch(buffer[o + 3], buffer[o + 4], 1));
            }
        } else {
            // Produce binary output
            if (s->output_format != 1) {
                fwrite((void *) (buffer + o + 0), sizeof(double), 3, output);
                bytes_written += 3 * sizeof(double);
            }
            if (s->output_format >= 1) {
                fwrite((void *) (buffer + o + 3), sizeof(double), 2, output);
                bytes_written += 2 * sizeof(double);
            }
            if (s->output_format >= 2) {
                fwrite((void *) (buffer + o + 5), sizeof(double), 3, output);
                bytes_written += 3 * sizeof(double);
            }
            if (s->output_format >= 3) {
                fwrite((void *) (buffer + o + 8), sizeof(double), 11, output);
                bytes_written += 11 * sizeof(double);
            }
            if (s->output_constellations) {
                // Copy constellation ID into an 8-byte null-terminated buffer, to ensure it is padded with zeros
                const char *constellation_id = constellations_fetch(buffer[o + 3], buffer[o + 4], 0);
                char constellation_id_buffer[8];
                memset(constellation_id_buffer, 0, 8);
                snprintf(constellation_id_buffer, 7, "%s", constellation_id);
                fwrite((const void *) constellation_id_buffer, 1, 8, output);
                bytes_written += 8;
            }
        }
    }
    if (!s->output_binary) {
        bytes_written += fprintf(output, "\n");
    }

    // Write the number of bytes written
    return bytes_written;
}

//! apply_time_standard - Convert the input Julian day number into Terrestrial Time
//! @param [in] s - The ephemeris settings.
//! @param [in] dt_calc - Delta-T computation instance.
//! @param [in] jd_in - The requested Julian day number.
//! @param [out] jd_tt - The output TT Julian day number.
//! @param [out] jd_utc - The output UTC Julian day number.

void apply_time_standard(const settings *s, const DeltaTCalculator *dt_calc,
                         const double jd_in, double *jd_tt, double *jd_utc) {
    // Check which time standard is in use
    const int use_tt = (str_cmp_no_case(s->time_standard, "tt") == 0);

    if (use_tt) {
        *jd_utc = utc_from_tt(dt_calc, jd_in, 0, 0);
        *jd_tt = jd_in;
    } else {
        *jd_utc = jd_in;
        *jd_tt = tt_from_utc(dt_calc, jd_in, 0, 0);
    }
}

//! compute_ephemeris - Main entry point to compute an ephemeris, with parameters described by a settings structure
//! @param [in] s - The ephemeris settings.
//! @param [in] dt_calc - Delta-T computation instance.
//! @param [in] output - The output stream where we write results.
//! @param [out] rows_computed - The number of rows we wrote to the output stream.
//! @param [out] status - 0 on success; 1 otherwise
//! @param [out] error_text - Error message in the event of failure.
//! @return The number of bytes written to the output stream.

int compute_ephemeris(settings *s, const DeltaTCalculator *dt_calc,
                      FILE *output, long *rows_computed, int *status, char *error_text) {
    // Keep track of how much data we have written
    int bytes_written = 0;

    // Initial processing of settings for this ephemeris
    settings_process(s, status, error_text);

    if (s->jd_list == NULL) {
        // Loop over all the time points in the ephemeris
        const int steps_total = (int) ceil((s->jd_max - s->jd_min) / s->jd_step);
        for (int step_count = 0; step_count < steps_total; step_count++) {
            double jd_tt, jd_utc;
            const double jd_in = s->jd_min + step_count * s->jd_step;
            apply_time_standard(s, dt_calc, jd_in, &jd_tt, &jd_utc);
            bytes_written += compute_ephemeris_time_point(s, dt_calc, output, jd_tt);
            (*rows_computed)++;
        }
    } else {
        // Loop over explicit list of time points in the ephemeris
        const char *scan = s->jd_list;
        while (*scan != '\0') {
            char jd_string[FNAME_LENGTH];
            str_comma_separated_list_scan(&scan, jd_string);
            double jd_tt, jd_utc;
            const double jd_in = get_float(jd_string, NULL);
            apply_time_standard(s, dt_calc, jd_in, &jd_tt, &jd_utc);
            bytes_written += compute_ephemeris_time_point(s, dt_calc, output, jd_tt);
            (*rows_computed)++;
        }
    }

    if (DEBUG) {
        char line[FNAME_LENGTH];
        strcpy(line, "Finished computing ephemeris.");
        ephem_log(line);
    }
    settings_close(s);

    // Write the number of bytes written
    return bytes_written;
}

//! compute_ephemeris_shutdown - Free all memory used in calculating ephemerides.

void compute_ephemeris_shutdown() {
    jpl_shutdown();
    meeus_shutdown();
    orbitalElements_shutdown();
    magnitudeEstimate_shutdown();
    constellations_close();
}
