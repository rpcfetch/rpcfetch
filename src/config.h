#pragma once

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "logger.h"
#include "version.h"

/*
 * The config file is a simple key=value file type with comments (#)
 */

#define CONFIG_STRING_LENGTH 256

struct rpcfetch_config {
  /* system usages refresh rate (milliseconds) */
  long system_rate;
  /* focused window refresh rate (milliseconds) */
  long window_rate;

  /* useful for changing the 'Playing with x' status or providing custom
   * fallback assets */
  long discord_client_id;
  /* GitHub repo latest release API url
   * (https://api.github.com/repos/username/reponame/releases/latest)
   */
  char resources_repo_url[CONFIG_STRING_LENGTH];
  /* save and load resources from dir/filename
   *
   * this can be provided seperately because this file can be automatically
   * downloaded thus the folder might need to be created, so it's easier to do
   * this way.
   */
  char resources_save_dir[CONFIG_STRING_LENGTH];
  char resources_save_filename[CONFIG_STRING_LENGTH];
  /* alternatively use this path to the resources file (if this is provided,
   * save_dir and filename will be ignored)
   */
  char resources_path[CONFIG_STRING_LENGTH];
  /* automatically download resources if not found locally */
  bool auto_download;
  /* automatically check the repository for newer resource versions */
  bool check_for_updates;

  /* path to the existing styles file */
  char styles_path[CONFIG_STRING_LENGTH];
};

/**
 * @brief sets the values of out_config to the defaults
 *
 * @param cache_dir path to cache directory, if empty /tmp/PROGRAM_NAME will be
 * set
 * @param out_config output config struct
 */
void default_config(char *cache_dir, struct rpcfetch_config *out_config);

/**
 * @brief parse config from config_location into out_config
 *
 * Note: default_config should be called before this to ensure that every value
 * is correctly set
 *
 * @param config_location config file location
 * @param out_config output config struct
 * @return 0 if can't open file, 1 on success
 */
int parse_config(char *config_location, struct rpcfetch_config *out_config);

/**
 * @brief before calling parse_config(), check if file exists
 *
 * @param config_location config file location
 * @param out_config output config struct
 * @return parse_config return, 2 if can't access file
 */
int try_parse_config(char *config_location, struct rpcfetch_config *out_config);
