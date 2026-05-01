// socket_server.c
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
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include <gsl/gsl_errno.h>

#include "compute.h"

#include "argparse/argparse.h"
#include "coreUtils/errorReport.h"
#include "coreUtils/asciiDouble.h"
#include "ephemCalc/constellations.h"
#include "ephemCalc/jpl.h"
#include "listTools/ltMemory.h"
#include "settings/settings.h"

static const char *const usage[] = {
    "socker_server.bin [options] [[--] args]",
    "socker_server.bin [options]",
    NULL,
};

//! read_float - Read a floating-point input argument from an input query.
//! \param scan - Pointer to the input query string, which we advance as we read characters.
//! \param target - The output floating-point argument we read.
//! \param argument_name - The name of the argument, used to produce user-friendly error messages.
//! \param final - Boolean flag, indicating whether we expect argument to be terminated by '\0' or '|'.
//! \return - Boolean flag, 0 means success, 1 means failure.

int read_float(char **scan, double *target, const char *argument_name, const int final) {
    int char_count = 0;
    *target = get_float(*scan, &char_count);
    if (char_count < 1) {
        printf("Could not read <%s>.\n", argument_name);
        fflush(stdout);
        return 1;
    }
    if (final) {
        if ((*scan)[char_count] != '\0') {
            printf("Unexpected trailing matter after <%s>.\n", argument_name);
            fflush(stdout);
            return 1;
        }
    } else {
        if ((*scan)[char_count] != '|') {
            printf("No separator after <%s>.\n", argument_name);
            fflush(stdout);
            return 1;
        }
        char_count++; // forward over separator
    }
    (*scan) += char_count;
    return 0;
}

//! read_integer - Read an integer input argument from an input query.
//! \param scan - Pointer to the input query string, which we advance as we read characters.
//! \param target - The output integer argument we read.
//! \param argument_name - The name of the argument, used to produce user-friendly error messages.

int read_integer(char **scan, int *target, const char *argument_name, const int final) {
    int char_count = 0;
    *target = (int) get_float(*scan, &char_count);
    if (char_count < 1) {
        printf("Could not read <%s>.\n", argument_name);
        fflush(stdout);
        return 1;
    }
    if (final) {
        if ((*scan)[char_count] != '\0') {
            printf("Unexpected trailing matter after <%s>.\n", argument_name);
            fflush(stdout);
            return 1;
        }
    } else {
        if ((*scan)[char_count] != '|') {
            printf("No separator after <%s>.\n", argument_name);
            fflush(stdout);
            return 1;
        }
        char_count++; // forward over separator
    }
    (*scan) += char_count;
    return 0;
}

//! read_string - Read a string input argument from an input query.
//! \param scan - Pointer to the input query string, which we advance as we read characters.
//! \param target - The output integer argument we read.
//! \param nullable - If true, then we return a NULL pointer if an empty string was read.
//! \param argument_name - The name of the argument, used to produce user-friendly error messages.
//! \param final - Boolean flag, indicating whether we expect argument to be terminated by '\0' or '|'.
//! \return - Boolean flag, 0 means success, 1 means failure.

int read_string(char **scan, char **target, int nullable, const char *argument_name, const int final) {
    // Allocate a buffer to read the string into
    long buffer_size = BUFSIZ;
    char *buffer = malloc(buffer_size);
    if (buffer == NULL) {
        printf("Failed to allocate a string buffer of size <%ld>.\n", buffer_size);
        *target = NULL;
        return 1;
    }
    buffer[0] = '\0';

    int char_count = 0;
    while (((*scan)[char_count] != '\0') && ((*scan)[char_count] != '|') && (char_count < buffer_size - 1)) {
        if (char_count >= buffer_size - 2) {
            buffer_size += BUFSIZ;
            char *new_buffer = realloc(buffer, buffer_size);
            if (new_buffer == NULL) {
                printf("Failed to allocate a string buffer of size <%ld>.\n", buffer_size);
                free(buffer);
                *target = NULL;
                return 1;
            }
            buffer = new_buffer;
        }
        buffer[char_count] = (*scan)[char_count];
        char_count++;
    }
    buffer[char_count] = '\0';

    if ((char_count < 1) && nullable) {
        *target = NULL;
        free(buffer);
    } else {
        *target = buffer;
    }

    if (final) {
        if ((*scan)[char_count] != '\0') {
            printf("Unexpected trailing matter after <%s>.\n", argument_name);
            fflush(stdout);
            return 1;
        }
    } else {
        if ((*scan)[char_count] != '|') {
            printf("No separator after <%s>.\n", argument_name);
            fflush(stdout);
            return 1;
        }
        char_count++; // forward over separator
    }

    (*scan) += char_count;
    return 0;
}

//!
//! Main entry point for starting an ephemeris-compute socket query service

int main(int argc, const char **argv) {
    int service_port = 8091;
    int local_only = 1;
    int use_de_number = 440;

    // Initialise sub-modules
    if (DEBUG) ephem_log("Initialising ephemeris computer.");
    lt_memoryInit(&ephem_error, &ephem_log);
    constellations_init();

    // Turn off GSL's automatic error handler
    gsl_set_error_handler_off();

    // Scan command-line options for any switches
    struct argparse_option options[] = {
        OPT_HELP(),
        OPT_GROUP("Basic options"),
        OPT_INTEGER('p', "port", &service_port,
                    "Port number for remote computation service"),
        OPT_INTEGER('l', "local_only", &local_only,
                    "If true, then the service is only exposed for connections from localhost. If false, the "
                    "service is visible to external network traffic"),
        OPT_INTEGER('d', "use_de", &use_de_number,
                    "Select which NASA JPL DE4xx ephemeris to use (e.g. 440 for DE440; default)"),
        OPT_END(),
    };

    // Read command-line options
    struct argparse argparse;
    argparse_init(&argparse, options, usage, 0);
    argparse_describe(&argparse,
                      "\nStart service for computing ephemerides for solar system bodies",
                      "\n");
    argc = argparse_parse(&argparse, argc, argv);

    // Check that all command-line arguments were parsed
    if (argc != 0) {
        int i;
        for (i = 0; i < argc; i++) {
            printf("Error: unparsed argument <%s>\n", *(argv + i));
            fflush(stdout);
        }
        ephem_fatal(__FILE__, __LINE__, "Unparsed arguments");
    }

    // Select which NASA JPL DE4xx ephemeris to use
    jpl_setEphemerisNumber(use_de_number);

    // Creating socket file descriptor
    const int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the server port
    int option_value = 1;
    const int sockopt_1_status = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &option_value, sizeof(option_value));
    if (sockopt_1_status) {
        perror("setsockopt (SO_REUSEADDR) failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

#ifdef SOL_SOCKET
    const int sockopt_2_status = setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &option_value, sizeof(option_value));
    if (sockopt_2_status) {
        perror("setsockopt (SO_REUSEPORT) failed");
        exit(EXIT_FAILURE);
    }
#endif

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = (int) (500e3); // microseconds

    const int sockopt_3_status = setsockopt(server_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    if (sockopt_3_status) {
        perror("setsockopt (SO_RCVTIMEO) failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    const int sockopt_4_status = setsockopt(server_fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    if (sockopt_4_status) {
        perror("setsockopt (SO_SNDTIMEO) failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Set network interface for socket
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    address.sin_family = AF_INET;
    address.sin_port = htons(service_port);

    if (local_only) {
        // Convert IPv4 and IPv6 addresses from text to binary
        char *service_host = "127.0.0.1";
        const int addr_status = inet_pton(AF_INET, service_host, &address.sin_addr);
        if (addr_status <= 0) {
            printf("\nInvalid address/ Address not supported \n");
            fflush(stdout);
            return -1;
        }
    } else {
        address.sin_addr.s_addr = INADDR_ANY;
    }

    // Forcefully attaching socket to the service port
    const int bind_status = bind(server_fd, (struct sockaddr *) &address, addrlen);
    if (bind_status < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    const int queue_length = 512;
    const int listen_status = listen(server_fd, queue_length);
    if (listen_status < 0) {
        perror("listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Keep track of the number of connections
    time_t last_report = time(NULL);
    time_t reporting_cadence = 3600;

    long int connection_count = 0;
    long int connection_count_requiring_constellations = 0;
    double sum_compute_time = 0.;
    long int row_count = 0;
    long int byte_count = 0;

    // Accept connections indefinitely
    while (1) {
        // Periodically report usage statistics
        time_t now = time(NULL);
        if (now > last_report + reporting_cadence) {
            char timestamp_buffer[FNAME_LENGTH];
            printf(
                "[%s] Connection count %8ld. Row count %9ld. Connection rate %10.1f per hour. Output bandwidth %9.2f MB/hour. Mean compute time %10.4f ms. Fraction with constellations %6.2f%%.\n",
                str_strip(friendly_time_string(), timestamp_buffer),
                connection_count, row_count,
                connection_count * 3600. / reporting_cadence,
                byte_count * 3600. / reporting_cadence / 1024. / 1024.,
                sum_compute_time / (connection_count + 1e-8),
                connection_count_requiring_constellations / (connection_count + 1e-8) * 100.
            );
            fflush(stdout);
            connection_count = 0;
            connection_count_requiring_constellations = 0;
            sum_compute_time = 0.;
            row_count = 0;
            byte_count = 0;
            last_report = now;
        }


        // Accept new connection
        const int new_socket = accept(server_fd, (struct sockaddr *) &address, &addrlen);
        if (new_socket < 0) {
            if (errno == EAGAIN) {
                // accept timed out; retry
                continue;
            } else {
                perror("accept failed");
                close(server_fd);
                exit(EXIT_FAILURE);
            }
        }

        // Calculate processing time
        struct timeval tval_start;
        gettimeofday(&tval_start, NULL);

        // Set up default settings
        settings ephemeris_settings;
        int jd_list_malloced = 0, objects_list_malloced = 0;
        if (DEBUG) ephem_log("Setting up default ephemeris parameters.");
        settings_default(&ephemeris_settings);

        // Allocate a string buffer for the request from the socket
        long buffer_size = BUFSIZ;
        char *buffer = malloc(buffer_size);
        if (buffer == NULL) {
            printf("Failed to allocate a string buffer of size <%ld>.\n", buffer_size);
            close(new_socket);
            goto connection_cleanup;
        }
        buffer[0] = '\0';
        int position = 0;

        // Read the request from the socket
        while (1) {
            long available_bytes = buffer_size - position;
            if (available_bytes < BUFSIZ) {
                buffer_size += BUFSIZ;
                char *new_buffer = realloc(buffer, buffer_size);
                if (new_buffer == NULL) {
                    printf("Failed to allocate a string buffer of size <%ld>.\n", buffer_size);
                    close(new_socket);
                    goto connection_cleanup;
                }
                buffer = new_buffer;
                available_bytes = buffer_size - position;
            }
            ssize_t valread = read(new_socket, buffer + position, available_bytes - 1);
            if (valread == 0) break;
            position += (int) valread;
        }
        buffer[position] = '\0';
        // printf("%s\n", buffer);
        // fflush(stdout);

        // Read settings from supplied query string
        char *scan = buffer;
        if (read_float(&scan, &ephemeris_settings.jd_min, "jd_min", 0)) {
            close(new_socket);
            goto connection_cleanup;
        }
        if (read_float(&scan, &ephemeris_settings.jd_max, "jd_max", 0)) {
            close(new_socket);
            goto connection_cleanup;
        }
        if (read_float(&scan, &ephemeris_settings.jd_step, "jd_step", 0)) {
            close(new_socket);
            goto connection_cleanup;
        }
        jd_list_malloced = 1;
        if (read_string(&scan, &ephemeris_settings.jd_list, 1, "jd_step", 0)) {
            close(new_socket);
            goto connection_cleanup;
        }
        if (read_float(&scan, &ephemeris_settings.latitude, "latitude", 0)) {
            close(new_socket);
            goto connection_cleanup;
        }
        if (read_float(&scan, &ephemeris_settings.longitude, "longitude", 0)) {
            close(new_socket);
            goto connection_cleanup;
        }
        if (read_integer(&scan, &ephemeris_settings.enable_topocentric_correction, "enable_topocentric_correction",
                         0)) {
            close(new_socket);
            goto connection_cleanup;
        }
        if (read_float(&scan, &ephemeris_settings.ra_dec_epoch, "ra_dec_epoch", 0)) {
            close(new_socket);
            goto connection_cleanup;
        }
        if (read_integer(&scan, &ephemeris_settings.output_format, "output_format", 0)) {
            close(new_socket);
            goto connection_cleanup;
        }
        if (read_integer(&scan, &ephemeris_settings.use_orbital_elements, "use_orbital_elements", 0)) {
            close(new_socket);
            goto connection_cleanup;
        }
        if (read_integer(&scan, &ephemeris_settings.output_binary, "output_binary", 0)) {
            close(new_socket);
            goto connection_cleanup;
        }
        if (read_integer(&scan, &ephemeris_settings.output_constellations, "output_constellations", 0)) {
            close(new_socket);
            goto connection_cleanup;
        }
        objects_list_malloced = 1;
        if (read_string(&scan, &ephemeris_settings.objects_input_list, 0, "objects_input_list", 1)) {
            close(new_socket);
            goto connection_cleanup;
        }

        // Create ephemeris, and send it to the open socket connection
        {
            FILE *output = fdopen(new_socket, "w");
            int status = 0;
            char error_text[LSTR_LENGTH] = "\0";
            byte_count += compute_ephemeris(&ephemeris_settings, output, &row_count, &status, error_text);
            if (status) {
                char timestamp_buffer[FNAME_LENGTH];
                printf("ERROR: [%s] %s\n", str_strip(friendly_time_string(), timestamp_buffer), error_text);
                fflush(stdout);
            }
            fclose(output);
        }

        // Close the connected socket
        settings_close(&ephemeris_settings);
        close(new_socket);

        // Calculate processing time
        struct timeval tval_finish;
        gettimeofday(&tval_finish, NULL);

        // Update activity counter
        connection_count++;
        connection_count_requiring_constellations += ephemeris_settings.output_constellations ? 1 : 0;

        struct timeval tval_compute_time;
        timersub(&tval_finish, &tval_start, &tval_compute_time);
        const double compute_time = tval_compute_time.tv_sec + tval_compute_time.tv_usec * 1e-6;
        sum_compute_time += compute_time;

        // Clean-up this socket connection
    connection_cleanup:
        if (buffer != NULL) free(buffer);
        if (jd_list_malloced && (ephemeris_settings.jd_list != NULL)) {
            free(ephemeris_settings.jd_list);
        }
        if (objects_list_malloced && (ephemeris_settings.objects_input_list != NULL)) {
            free(ephemeris_settings.objects_input_list);
        }
    }

    // closing the listening socket
    close(server_fd);
    compute_ephemeris_shutdown();
    lt_freeAll(0);
    lt_memoryStop();
    if (DEBUG) ephem_log("Terminating normally.");
    return 0;
}
