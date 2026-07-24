// settings.c
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "coreUtils/asciiDouble.h"
#include "ephemCalc/orbitalElements.h"

#include "settings.h"

// Default settings for producing an ephemeris
void settings_default(settings *i) {
    i->jd_min = 2451544.5;
    i->jd_max = 2451575.5;
    i->jd_step = 1.0;
    i->latitude = 0;
    i->longitude = 0;
    i->enable_topocentric_correction = 0;
    i->ra_dec_epoch = 2451545.0; // By default, use J2000 coordinates
    i->output_format = 0;
    i->use_orbital_elements = 0;
    i->output_constellations = 0;
    i->output_binary = 0;
    i->objects_count = 0;
    i->time_standard = "tt";
    i->objects_input_list = "jupiter";
    i->jd_list = NULL;
}

// Display a textual representation of a set of configuration settings
void settings_display(const settings *i, FILE *output) {
    fprintf(output, "jd_min = %.18e\n", i->jd_min);
    fprintf(output, "jd_max = %.18e\n", i->jd_max);
    fprintf(output, "jd_step = %.18e\n", i->jd_step);
    fprintf(output, "time_standard = %s\n", i->time_standard);
    fprintf(output, "latitude = %.18e\n", i->latitude);
    fprintf(output, "longitude = %.18e\n", i->longitude);
    fprintf(output, "enable_topocentric_correction = %d\n", i->enable_topocentric_correction);
    fprintf(output, "ra_dec_epoch = %.18e\n", i->ra_dec_epoch);
    fprintf(output, "output_format = %d\n", i->output_format);
    fprintf(output, "use_orbital_elements = %d\n", i->use_orbital_elements);
    fprintf(output, "output_constellations = %d\n", i->output_constellations);
    fprintf(output, "output_binary = %d\n", i->output_binary);
    fprintf(output, "objects_count = %d\n", i->objects_count);
    fprintf(output, "objects_input_list = %s\n", i->objects_input_list);
    fprintf(output, "jd_list = %s\n", i->jd_list);
    fprintf(output, "\n");
}

// Process the contents of a settings structure before producing the ephemeris
void settings_validate(const settings *i, int *status, char *error_text) {
    // Clear error status
    *status = 0;

    // Debugging code to output the settings in use
    //    {
    //        FILE *o;
    //        o = fopen("/tmp/ec_config", "a");
    //        settings_display(i, o);
    //        fclose(o);
    //    }

    // Check that we're using a recognised time standard
    if ((str_cmp_no_case(i->time_standard, "tt") != 0) && (str_cmp_no_case(i->time_standard, "utc") != 0)) {
        *status = 1;
        snprintf(error_text, FNAME_LENGTH, "Unrecognised time standard <%s>.", i->time_standard);
        return;
    }
}

// Process the contents of a settings structure before producing the ephemeris
void settings_process(settings *i, int *status, char *error_text) {
    // Clear error status
    *status = 0;

    // Check that settings are valid
    settings_validate(i, status, error_text);
    if (*status) return;

    // Transfer the names of objects we are to compute ephemerides for from <i->objects_input_list> to <i->object_name>
    int k = 0, l = 0;
    while (i->objects_input_list[k] > '\0') {
        // Commas are used to separate object names on the command line
        if (i->objects_input_list[k] == ',') {
            if (l > 0) {
                i->object_name[i->objects_count][l] = '\0';
                i->objects_count++;
                l = 0;

                if (i->objects_count >= MAX_OBJECTS) {
                    *status = 1;
                    snprintf(error_text, FNAME_LENGTH, "Too many objects (limit of %d).", MAX_OBJECTS);
                    return;
                }
            }
            k++; // next character
            continue;
        }

        // All characters other than commas are part of object names
        i->object_name[i->objects_count][l++] = i->objects_input_list[k++];
    }

    // Make sure that the last object is added to the list
    if (l > 0) {
        i->object_name[i->objects_count][l] = '\0';
        i->objects_count++;
    }

    // Loop over all the objects we are producing an ephemeris for
    for (k = 0; k < i->objects_count; k++) {
        i->body_id[k] = orbitalElements_searchBodyIdByObjectName(i->object_name[k]);

        if (i->body_id[k] < 0) {
            *status = 1;
            snprintf(error_text, FNAME_LENGTH, "Unrecognised object name <%s>", i->object_name[k]);
            //return;
        }
    }
}

// Delete any memory allocated within a settings structure
void settings_close(settings *i) {
}
