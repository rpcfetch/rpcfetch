#pragma once

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

#include "cjson/cJSON.h"
#include "logger.h"

// TODO: user-defined fallback strings (in styles)
#define FALLBACK_OS "tux"
#define FALLBACK_APP "file"
#define FALLBACK_WM "xorg"

enum query_type {
  OS,
  APP,
  WM,
};

/**
 * @brief load resources into out_json
 *
 * out_content and out_json should be freed after usage.
 * Warning: out_json points to out_content, meaning if you free out_content you
 * must free out_json too!
 *
 * @param file resources file path
 * @param out_content output file content
 * @param out_json output parsed JSON
 * @return 1 if succeeded, 0 otherwise
 */
int load_resources(char *file, char **out_content, cJSON **out_json);

/**
 * @brief precompile regexes from JSON into two arrays:
 * - compiled_raw (key)
 * - compiled (value)
 *
 * Both arrays should be freed after usage
 *
 * @param resources JSON input
 * @param compiled_raw output keys
 * @param compiled output values
 * @param compiled_count output count
 * @return 1 if succeeded, 0 otherwise
 */
int precompile_regexes(cJSON *resources, char ***compiled_raw,
                       pcre2_code ***compiled, int *compiled_count);

#define QUERY_ARGS                                                             \
  char *resource_name, char *return_url, int len, enum query_type type,        \
      cJSON *resources, char **compiled_raw, pcre2_code **compiled,            \
      int compiled_count

/**
 * @brief query resource image from name
 * This function will use regular expression patter matching if needed.
 *
 * @return 1 if querying resource succeeded, otherwise 0
 */
int query_resource(QUERY_ARGS);

/**
 * @brief acts like query_resource() however it does only return the fallback
 * values
 *
 * @return always returns 1
 */
int query_fallback(QUERY_ARGS);

/**
 * @brief log missing name to file
 *
 * @param file log file
 * @param name log content
 * @return 0 if fails, 1 if added successfully, 2 if already exists
 */
int append_missing_names(char *file, char *name);
