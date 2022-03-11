// settings.h
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

#ifndef SETTINGS_H
#define SETTINGS_H 1

#include "coreUtils/strConstants.h"

#define MAX_OBJECTS 48

typedef struct settings {
    double jd_min, jd_max, jd_step, ra_dec_epoch;  // All specified in TT
    int use_orbital_elements, output_binary, output_format, output_constellations;
    int body_id[MAX_OBJECTS];
    char object_name[MAX_OBJECTS][FNAME_LENGTH];
    const char *objects_input_list;
    int objects_count;
} settings;

void settings_default(settings *i);

void settings_process(settings *i);

void settings_close(settings *i);

#endif

