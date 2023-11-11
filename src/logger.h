#pragma once

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

enum log_type { INFO, WARN, ERROR };
static const char *log_type_name[] = {"info", "warning", "error", 0};

#define LOG_INFO "info"
#define LOG_WARN "warning"
#define LOG_ERROR "error"

/**
 * @brief set debug mode to value
 *
 * @param debug value
 */
void set_debug(int debug);

/**
 * @brief log message
 *
 * @param type log type
 * @param source source function
 * @param debug debug mode
 * @param format printf-like log messageformat
 * @param ... printf-like arguments
 */
void log_msg(enum log_type type, const char *source, int debug,
             const char *format, ...);
