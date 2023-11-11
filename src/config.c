#include "config.h"

/**
 * @brief parse line of config file
 *
 * @param line line input
 * @param line_length input string length
 * @param key return key
 * @param value return value
 * @return 0 on error, 1 if parsed, 2 if empty
 */
static int parse_config_line(const char *line, size_t line_length, char *key,
                             char *value) {
  if (line == NULL || line_length == 0 || line[0] == '\0' || line[0] == '\n' ||
      line[0] == '\r' || line[0] == '#') {
    key[0] = '\0';
    value[0] = '\0';
    return 2;
  }
  int state = 0;
  int key_length = 0;
  int value_length = 0;
  int string_start = -1;
  int string_end = -1;
  for (size_t i = 0; i < line_length; i++) {

    char c = line[i];
    if (c == '\0') {
      if (value[0] == '\0')
        return 0;
      break;
    }
    // comment
    if (c == '#' && (string_start == -1 || string_end != -1))
      break;
    if (state == 0) {
      if (key_length >= CONFIG_STRING_LENGTH) {
        log_msg(ERROR, "parse_config_line", 0,
                "key length is larger than expected");
        key[CONFIG_STRING_LENGTH - 1] = '\0';
        value[0] = '\0';
        return 0;
      }
      if (c == ' ')
        continue;
      if (c == '=') {
        state = 1;
        continue;
      }
      key[key_length] = c;
      key_length++;
    } else if (state == 1) {
      if (value_length >= CONFIG_STRING_LENGTH) {
        log_msg(ERROR, "parse_config_line", 0,
                "value length is larger than expected");
        value[CONFIG_STRING_LENGTH - 1] = '\0';
        return 0;
      }
      if (c == '"' && string_start == -1) {
        string_start = value_length;
        continue;
      }
      if (c == '"') {
        string_end = value_length;
        break;
      }
      if (c == '\r' || c == '\n')
        break;
      value[value_length] = c;
      value_length++;
    }
  }

  key[key_length] = '\0';
  value[value_length] = '\0';

  if (string_start != -1 && string_end != -1) {
    for (int i = string_start; i < string_end; i++) {
      value[i - string_start] = value[i];
    }
    value[string_end - string_start] = '\0';
  }

  return (key[0] == '\0' || value[0] == '\0') ? 2 : 1;
}

void default_config(char *cache_dir, struct rpcfetch_config *out_config) {
  memset(out_config, 0, sizeof(*out_config));
  out_config->system_rate = 15000;
  out_config->window_rate = 1000;
  out_config->discord_client_id = 934099338374824007L;

  snprintf(out_config->resources_repo_url, CONFIG_STRING_LENGTH,
           RESOURCES_RELEASES);
  if (cache_dir != NULL) {
    snprintf(out_config->resources_save_dir, CONFIG_STRING_LENGTH, "%s",
             cache_dir);
  } else {
    snprintf(out_config->resources_save_dir, CONFIG_STRING_LENGTH, "/tmp/%s",
             PROGRAM_NAME);
  }
  snprintf(out_config->resources_save_filename, CONFIG_STRING_LENGTH,
           "resources.json");
  snprintf(out_config->resources_path, CONFIG_STRING_LENGTH, "");
  out_config->auto_download = true;
  out_config->check_for_updates = true;
  snprintf(out_config->styles_path, CONFIG_STRING_LENGTH, "");
}

int parse_config(char *config_location, struct rpcfetch_config *out_config) {
  FILE *fp;
  char *line = NULL;
  size_t n;

  fp = fopen(config_location, "r");
  if (fp == NULL) {
    log_msg(ERROR, "parse_config", 0, "couldn't open file: %s",
            config_location);
    return 0;
  }

  while (getline(&line, &n, fp) != -1) {
    char key[CONFIG_STRING_LENGTH];
    char value[CONFIG_STRING_LENGTH];

    int parse_status = parse_config_line(line, n, key, value);
    if (parse_status == 1) {
      if (strncasecmp(key, "system_rate", CONFIG_STRING_LENGTH) == 0) {
        out_config->system_rate = strtol(value, NULL, 10);
        continue;
      }
      if (strncasecmp(key, "window_rate", CONFIG_STRING_LENGTH) == 0) {
        out_config->window_rate = strtol(value, NULL, 10);
        continue;
      }
      if (strncasecmp(key, "discord_client_id", CONFIG_STRING_LENGTH) == 0) {
        out_config->discord_client_id = strtol(value, NULL, 10);
        continue;
      }
      if (strncasecmp(key, "resources_repo_url", CONFIG_STRING_LENGTH) == 0) {
        snprintf(out_config->resources_repo_url, CONFIG_STRING_LENGTH, "%s",
                 value);
        continue;
      }
      if (strncasecmp(key, "resources_save_dir", CONFIG_STRING_LENGTH) == 0) {
        snprintf(out_config->resources_save_dir, CONFIG_STRING_LENGTH, "%s",
                 value);
        continue;
      }
      if (strncasecmp(key, "resources_save_filename", CONFIG_STRING_LENGTH) ==
          0) {
        snprintf(out_config->resources_save_filename, CONFIG_STRING_LENGTH,
                 "%s", value);
        continue;
      }
      if (strncasecmp(key, "resources_path", CONFIG_STRING_LENGTH) == 0) {
        snprintf(out_config->resources_path, CONFIG_STRING_LENGTH, "%s", value);
        continue;
      }
      if (strncasecmp(key, "auto_download", CONFIG_STRING_LENGTH) == 0) {
        int parsed_true = strncasecmp(value, "true", CONFIG_STRING_LENGTH) == 0;
        if (parsed_true) {
          out_config->auto_download = true;
          continue;
        }
        int parsed_false =
            strncasecmp(value, "false", CONFIG_STRING_LENGTH) == 0;
        if (!parsed_false) {
          log_msg(WARN, "parse_config", 0, "invalid value for %s",
                  "auto_download");
        }
        out_config->auto_download = false;
        continue;
      }
      if (strncasecmp(key, "check_for_updates", CONFIG_STRING_LENGTH) == 0) {
        int parsed_true = strncasecmp(value, "true", CONFIG_STRING_LENGTH) == 0;
        if (parsed_true) {
          out_config->check_for_updates = true;
          continue;
        }
        int parsed_false =
            strncasecmp(value, "false", CONFIG_STRING_LENGTH) == 0;
        if (!parsed_false) {
          log_msg(WARN, "parse_config", 0, "invalid value for %s",
                  "check_for_updates");
        }
        out_config->check_for_updates = false;
        continue;
      }
      if (strncasecmp(key, "styles_path", CONFIG_STRING_LENGTH) == 0) {
        snprintf(out_config->styles_path, CONFIG_STRING_LENGTH, "%s", value);
        continue;
      }

      log_msg(WARN, "parse_config", 0, "invalid key %s", key);
    } else if (!parse_status) {
      // remove newline for printing out
      size_t line_len = strlen(line);
      if (line[line_len - 2] == '\r') {
        line[line_len - 2] = '\0';
      }
      line[line_len - 1] = '\0';

      log_msg(WARN, "parse_config", 0, "invalid line %s", line);
    }
  }

  free(line);
  fclose(fp);

  return 1;
}

int try_parse_config(char *config_location,
                     struct rpcfetch_config *out_config) {
  if (access(config_location, F_OK) != 0) {
    return 2;
  }

  log_msg(INFO, "try_parse_config", 1, "Parsing config file: %s",
          config_location);
  return parse_config(config_location, out_config);
}
