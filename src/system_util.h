#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "logger.h"

#define APPLE_NAME "MacOS"
#define WINDOWS_NAME "Windows"

int get_os_release_name(char **name);

int get_lsb_release_id(char **name);

/**
 * @brief Allocate and get OS or distro (if Linux) name into name.
 * Name should be freed after usage.
 *
 * @param name OS or distro name
 * @return 0 if failed 1 succeeded
 */
int get_os_name(char **name);

/**
 * @brief get cpu usage percentage
 * If the arguments are zero, it will return 0 and will initialize the arguments
 * for the second call.
 * Starting from the second call this will return valid percentages (some time
 * has to pass for accurate results).
 *
 * @param last_total_user last total user time (initially should be a pointer to
 * a zero value)
 * @param last_total_user_low last total user low time (initially should be a
 * pointer to a zero value)
 * @param last_total_sys last total system time (initially should be a pointer
 * to a zero value)
 * @param last_total_idle last total idle time (initially should be a pointer to
 * a zero value)
 * @return cpu usage percentage (0.0 - 100.0)
 */
double get_cpu_usage(unsigned long long *last_total_user,
                     unsigned long long *last_total_user_low,
                     unsigned long long *last_total_sys,
                     unsigned long long *last_total_idle);

/**
 * @brief get memory usage percentage
 *
 * @return memory usage percentage
 */
double get_memory_usage();

/**
 * @brief get system uptime in seconds
 *
 * @return uptime in seconds
 */
double get_uptime();

/**
 * @brief get milliseconds since epoch
 *
 * @return millis since epoch
 */
long long millis_since_epoch();
