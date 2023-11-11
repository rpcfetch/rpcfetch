#pragma once
#include <argp.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "version.h"

/*
 * The command line arguments system uses argp.h from the GNU C Library
 * This might need to be replaced for more complicated tasks, and cross-platform
 * compatibility.
 */

struct rpcfetch_arguments {
  char config_file[1024];
  bool debug;
  bool force_update;
  bool offline;
  bool retry_on_error;
  bool should_exit;
  long system_rate;
  long window_rate;
};

/**
 * @brief parse command line arguments
 *
 * @param argc arg count
 * @param argv arg array
 * @param out_arguments output arguments
 * @return argp_parse return value
 */
error_t parse_args(int argc, char *argv[],
                   struct rpcfetch_arguments *out_arguments);
