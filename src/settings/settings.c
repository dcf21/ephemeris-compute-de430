// settings.c
// 
// -------------------------------------------------
// Copyright 2015-2020 Dominic Ford
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
#include <math.h>
#include <unistd.h>
#include <coreUtils/errorReport.h>

#include "coreUtils/asciiDouble.h"
#include "ephemCalc/orbitalElements.h"

#include "settings.h"

// Default settings for producing an ephemeris
void settings_default(settings *i) {
    i->jd_min = 2451544.5;
    i->jd_max = 2451575.5;
    i->jd_step = 1.0;
    i->ra_dec_epoch = 2451545.0;  // By default, use J2000 coordinates
    i->output_format = 0;
    i->use_orbital_elements = 0;
    i->output_constellations = 0;
    i->output_binary = 0;
    i->objects_count = 0;
    i->objects_input_list = "jupiter";
}

// Process the contents of a settings structure before producing the ephemeris
void settings_process(settings *i) {
    int k, l;
    char name[FNAME_LENGTH];

    // Transfer the names of objects we are to compute ephemerides for from <i->objects_input_list> to <i->object_name>
    k = l = 0;
    while (i->objects_input_list[k] > '\0') {
        // Commas are used to separate object names on the command line
        if (i->objects_input_list[k] == ',') {
            if (l > 0) {
                i->object_name[i->objects_count][l] = '\0';
                i->objects_count++;
                l = 0;
            }
            k++; // next character
            continue;
        }

        // All characters other than commas are part of object names
        i->object_name[i->objects_count][l++] = i->objects_input_list[k++];
    }

    // Make sure that last object is added to list
    if (l > 0) {
        i->object_name[i->objects_count][l] = '\0';
        i->objects_count++;
    }

    // Loop over all the objects we are producing an ephemeris for
    for (k = 0; k < i->objects_count; k++) {
        i->body_id[k] = -1;

        // Convert the name of the requested objects into numeric object IDs
        strncpy(name, i->object_name[k], FNAME_LENGTH);
        name[FNAME_LENGTH - 1] = '\0';
        str_strip(name, name);
        str_lower(name, name);
        if ((strcmp(name, "mercury") == 0) || (strcmp(name, "pmercury") == 0) || (strcmp(name, "p1") == 0))
            i->body_id[k] = 0;
        else if ((strcmp(name, "venus") == 0) || (strcmp(name, "pvenus") == 0) || (strcmp(name, "p2") == 0))
            i->body_id[k] = 1;
        else if ((strcmp(name, "earth") == 0) || (strcmp(name, "pearth") == 0) || (strcmp(name, "p3") == 0))
            i->body_id[k] = 19;
        else if ((strcmp(name, "mars") == 0) || (strcmp(name, "pmars") == 0) || (strcmp(name, "p4") == 0))
            i->body_id[k] = 3;
        else if ((strcmp(name, "jupiter") == 0) || (strcmp(name, "pjupiter") == 0) || (strcmp(name, "p5") == 0))
            i->body_id[k] = 4;
        else if ((strcmp(name, "saturn") == 0) || (strcmp(name, "psaturn") == 0) || (strcmp(name, "p6") == 0))
            i->body_id[k] = 5;
        else if ((strcmp(name, "uranus") == 0) || (strcmp(name, "puranus") == 0) || (strcmp(name, "p7") == 0))
            i->body_id[k] = 6;
        else if ((strcmp(name, "neptune") == 0) || (strcmp(name, "pneptune") == 0) || (strcmp(name, "p8") == 0))
            i->body_id[k] = 7;
        else if ((strcmp(name, "pluto") == 0) || (strcmp(name, "ppluto") == 0) || (strcmp(name, "p9") == 0))
            i->body_id[k] = 8;
        else if ((strcmp(name, "moon") == 0) || (strcmp(name, "pmoon") == 0) || (strcmp(name, "p301") == 0))
            i->body_id[k] = 9;
        else if (strcmp(name, "sun") == 0)
            i->body_id[k] = 10;
        else if (((name[0] == 'a') || (name[0] == 'A')) && valid_float(name + 1, NULL)) {
            // Asteroid, e.g. A1
            i->body_id[k] = 1000000 + (int) get_float(name + 1, NULL);
        } else if (((name[0] == 'c') || (name[0] == 'C')) && valid_float(name + 1, NULL)) {
            // Comet, e.g. C1 (first in datafile)
            i->body_id[k] = 2000000 + (int) get_float(name + 1, NULL);
        } else {
            // Search for comets with matching names

            // Open comet database
            orbitalElements_comets_init();

            // Loop over comets seeing if names match
            int index;
            for (index = 0; index < comet_count; index++) {
                // Fetch comet information
                orbitalElements *item = orbitalElements_comets_fetch(index);

                if ((str_cmp_no_case(name, item->name) == 0) || (str_cmp_no_case(name, item->name2) == 0)) {
                    i->body_id[k] = 2000000 + index;
                    break;
                }
            }
        }

        if (i->body_id[k] < 0) {
            snprintf(temp_err_string, FNAME_LENGTH, "Unrecognised object name <%s>", name);
            ephem_fatal(__FILE__, __LINE__, temp_err_string);
            exit(1);
        }
    }
}

// Delete any memory allocated within a settings structure
void settings_close(settings *i) {
}

