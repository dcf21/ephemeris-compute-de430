// socket_client.c
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

#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include <sys/socket.h>

#include "argparse/argparse.h"
#include "coreUtils/errorReport.h"
#include "settings/settings.h"

static const char *const usage[] = {
        "socket_client.bin [options] [[--] args]",
        "socket_client.bin [options]",
        NULL,
};

//! Maximum length of sleep for <connect_retry>, in ms
const int max_sleep = 320;

//! connect_retry - Connect to a socket with retries after a short delay.
//! \param socket_fd - Socket file descriptor
//! \param addr - Address of the socket to connect to
//! \param alen - Length of addr
//! \return - Boolean indicating whether a successful connection was made
int connect_retry(int socket_fd, const struct sockaddr *addr, socklen_t alen) {
    // Try to connect to the socket_server with an exponential back off
    for (int pause_ms = 10; pause_ms <= max_sleep; pause_ms <<= 1) {
        if (connect(socket_fd, addr, alen) == 0) {
            // Successful connection
            return 0;
        }

        // Pause and retry
        struct timespec pause;
        pause.tv_sec = 0;
        pause.tv_nsec = (int) (pause_ms * 1e6);  // nanoseconds
        nanosleep(&pause, NULL);
    }

    // Fail
    return -1;
}

//! Main entry point for minimal client which connects to the ephemeris-compute socket service

int main(int argc, const char **argv) {
    settings ephemeris_settings;
    int service_port = 8091;
    char *service_host = "127.0.0.1";

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
            OPT_INTEGER('r', "use_orbital_elements", &ephemeris_settings.use_orbital_elements,
                        "Set the either 0 (use DE4xx) or 1 (use orbital elements)"),
            OPT_INTEGER('z', "output_binary", &ephemeris_settings.output_binary,
                        "Set to either 0 (text output) or 1 (binary output)"),
            OPT_INTEGER('c', "output_constellations", &ephemeris_settings.output_constellations,
                        "Set to either 0 (no column for constellation names) or 1"),
            OPT_STRING('o', "objects", &ephemeris_settings.objects_input_list,
                       "The list of objects to produce ephemerides for. See README.md."),

            OPT_INTEGER('p', "port", &service_port,
                        "Port number for remote computation service"),
            OPT_STRING('h', "host", &service_host,
                       "Hostname for remote computation service"),
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

    // Open socket to computation server
    const int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    // Set address and port
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(service_port);

    // Convert IPv4 and IPv6 addresses from text to binary
    const int addr_status = inet_pton(AF_INET, service_host, &serv_addr.sin_addr);
    if (addr_status <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    // Connect to socket
    const int status = connect_retry(client_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
    if (status < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    // Write query to buffer
    char query_buffer[LSTR_LENGTH];
    snprintf(query_buffer, LSTR_LENGTH,
             "%.18e|%.18e|%.18e|%s|%.18e|%.18e|%d|%.18e|%d|%d|%d|%d|%s",
             ephemeris_settings.jd_min, ephemeris_settings.jd_max, ephemeris_settings.jd_step,
             ephemeris_settings.jd_list == NULL ? "" : ephemeris_settings.jd_list,
             ephemeris_settings.latitude, ephemeris_settings.longitude,
             ephemeris_settings.enable_topocentric_correction,
             ephemeris_settings.ra_dec_epoch, ephemeris_settings.output_format, ephemeris_settings.use_orbital_elements,
             ephemeris_settings.output_binary, ephemeris_settings.output_constellations,
             ephemeris_settings.objects_input_list == NULL ? "" : ephemeris_settings.objects_input_list
    );
    send(client_fd, query_buffer, strlen(query_buffer), 0);

    // Close socket for writing
    shutdown(client_fd, SHUT_WR);

    // Read back response, and send to stdout
    int fd_to = fileno(stdout);
    while (1) {
        char buf[LSTR_LENGTH];
        ssize_t nread = read(client_fd, buf, sizeof buf);
        if (nread == 0) break;

        char *out_ptr = buf;
        ssize_t nwritten;

        do {
            nwritten = write(fd_to, out_ptr, nread);

            if (nwritten >= 0) {
                nread -= nwritten;
                out_ptr += nwritten;
            } else if (errno != EINTR) {
                perror("Could not write output to stdout");
                exit(EXIT_FAILURE);
            }
        } while (nread > 0);
    }

    // Close the connected socket
    close(client_fd);

    // Finished
    settings_close(&ephemeris_settings);
    if (DEBUG) ephem_log("Terminating normally.");
    return 0;
}

