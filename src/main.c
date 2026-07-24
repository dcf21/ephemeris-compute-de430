// main.c
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
#include <stdlib.h>
#include <string.h>

#include <gsl/gsl_errno.h>

#include "compute.h"

#include "argparse/argparse.h"
#include "coreUtils/errorReport.h"
#include "ephemCalc/constellations.h"
#include "ephemCalc/jpl.h"
#include "listTools/ltMemory.h"
#include "settings/settings.h"

static const char *const usage[] = {
    "ephem.bin [options] [[--] args]",
    "ephem.bin [options]",
    NULL,
};

//! Main entry point for stand-alone ephemeris compute tool. This version of the tool computes the ephemeris and then
//! exits, unlike the socket client/server tools, which achieve more efficiency by having a background server which
//! keeps ephemeris data in memory.

int main(int argc, const char **argv) {
    settings ephemeris_settings;
    int use_de_number = 440;

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
        OPT_STRING('j', "jd_list", &ephemeris_settings.jd_list,
                   "The list of Julian day numbers to calculate (optional). If specified, this overrides <jd_min>, <jd_max> and <jd_step>."),
        OPT_STRING('i', "time_standard", &ephemeris_settings.time_standard,
                   "The time standard to use. Either 'TT' (default) or 'UTC'."),
        OPT_FLOAT('l', "latitude", &ephemeris_settings.latitude,
                  "The latitude of the observation site (deg); only used if topocentric correction enabled"),
        OPT_FLOAT('m', "longitude", &ephemeris_settings.longitude,
                  "The longitude of the observation site (deg); only used if topocentric correction enabled"),
        OPT_INTEGER('t', "enable_topocentric_correction", &ephemeris_settings.enable_topocentric_correction,
                    "Set to either 0 (return geocentric coordinates) or 1 (return topocentric coordinates)"),
        OPT_FLOAT('e', "epoch", &ephemeris_settings.ra_dec_epoch,
                  "The epoch of the RA/Dec coordinate system, e.g. 2451545.0 for J2000"),
        OPT_INTEGER('f', "output_format", &ephemeris_settings.output_format,
                    "The output format for the ephemeris. See README.md."),
        OPT_INTEGER('d', "use_de", &use_de_number,
                    "Select which NASA JPL DE4xx ephemeris to use (e.g. 440 for DE440; default)"),
        OPT_INTEGER('r', "use_orbital_elements", &ephemeris_settings.use_orbital_elements,
                    "Set the either 0 (use DE4xx ephemeris) or 1 (use orbital elements)"),
        OPT_INTEGER('z', "output_binary", &ephemeris_settings.output_binary,
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

    // Select which NASA JPL DE4xx ephemeris to use
    jpl_setEphemerisNumber(use_de_number);

    // Instantiate Delta-T calculator
    DeltaTCalculator dt_calc;
    if (!delta_t_init(&dt_calc)) {
        ephem_fatal(__FILE__, __LINE__, "Failed to initialise <DeltaTCalculator>.");
    }

    // Create ephemeris
    {
        int status = 0;
        char error_text[LSTR_LENGTH] = "\0";
        long row_count = 0;
        compute_ephemeris(&ephemeris_settings, &dt_calc, stdout, &row_count, &status, error_text);
        if (status) {
            ephem_fatal(__FILE__, __LINE__, error_text);
            exit(1);
        }
    }

    // Free Delta-T calculator
    delta_t_free(&dt_calc);

    compute_ephemeris_shutdown();
    lt_freeAll(0);
    lt_memoryStop();
    if (DEBUG) ephem_log("Terminating normally.");
    return 0;
}
