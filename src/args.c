#include "args.h"

const char *argp_program_bug_address = PROGRAM_REPO;
static struct argp argp;

static struct argp_option rpcfetch_options[] = {
    {"config", 'c', "FILE", 0, "use this user configuration file"},
    {"debug", 'd', 0, 0, "print debugging info"},
    {"help", 'h', 0, 0, "print help message"},
    {"offline", 'o', 0, 0, "offline mode - don't download releases"},
    {"retry-on-error", 'r', 0, 0, "don't exit on discord callback errors"},
    {"system-rate", 's', "MILLIS", 0,
     "system resource usage update rate in milliseconds"},
    {"update", 'U', 0, 0, "force update relases and exit"},
    {"version", 'V', 0, 0, "print version"},
    {"window-rate", 'w', "MILLIS", 0,
     "focused window update rate in milliseconds"},
    {0}};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  struct rpcfetch_arguments *arguments = state->input;
  if (arguments->should_exit) {
    return 0;
  }

  switch (key) {
  case 'c':
    strncpy(arguments->config_file, arg, sizeof(arguments->config_file) - 1);
    arguments->config_file[sizeof(arguments->config_file) - 1] = '\0';
    break;
  case 'd':
    arguments->debug = true;
    break;
  case 'h':
    argp_state_help(state, state->err_stream, ARGP_HELP_STD_HELP);
    arguments->should_exit = true;
    break;
  case 'o':
    arguments->offline = true;
    break;
  case 'r':
    arguments->retry_on_error = true;
    break;
  case 's':
    arguments->system_rate = strtol(arg, NULL, 10);
    break;
  case 'w':
    arguments->window_rate = strtol(arg, NULL, 10);
    break;
  case 'U':
    arguments->force_update = true;
    break;
  case 'V':
    printf("%s %s\n", PROGRAM_NAME, PROGRAM_VERSION);
    arguments->should_exit = true;
    break;

  case ARGP_KEY_ARG:
    return 0;
  default:
    return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

static struct argp argp = {rpcfetch_options, parse_opt, 0, 0, 0, 0, 0};

int parse_args(int argc, char *argv[],
               struct rpcfetch_arguments *out_arguments) {
  memset(out_arguments, 0, sizeof(*out_arguments));
  return argp_parse(&argp, argc, argv, ARGP_NO_HELP, 0, out_arguments);
}
