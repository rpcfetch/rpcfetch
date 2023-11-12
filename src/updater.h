#pragma once

// uncomment the following line to build for only offline usage
// OR use `make offline`
// #define OFFLINE

#ifndef OFFLINE
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"
#include "logger.h"
#include "semver.h"
#include "version.h"

/**
 * @brief check for the updates and return the latest one
 *
 * @param repo_url GitHub repository latest release API endpoint e.g:
 * `https://api.github.com/repos/grialion/rpcpp/releases/latest`
 * @param file the JSON file to read the version from
 * @param ret_latest_version latest version string
 * @param length maximum version string length
 * @return 1 if update available, otherwise 0
 */
int check_for_updates(char *repo_url, char *file, char *ret_latest_version,
                      int length);

/**
 * @brief Download update from a GitHub repository
 *
 * @param repo_url GitHub repository latest release API endpoint e.g:
 * `https://api.github.com/repos/grialion/rpcpp/releases/latest`
 * @param content_file name of the file to download the update to
 * @param version_file name of the file to save the version to
 *
 * @return 1 if updated successfully, otherwise 0
 */
int update(char *repo_url, char *content_file);
#endif
