

#include "resource_manager.h"

struct Resource {
  char *pattern;
  bool regexp;
  bool icase;
  char *image;
};

/**
 * @brief parse cJSON to Resource
 *
 * Every string points to the json's memory, so it shouldn't be freed.
 *
 * @param resources json input
 * @param ret Resource ouput
 * @return int
 */
static int parse_resource(cJSON *resources, struct Resource *ret) {
  memset(ret, 0, sizeof(struct Resource));

  ret->pattern =
      cJSON_GetStringValue(cJSON_GetObjectItem(resources, "pattern"));
  ret->regexp = cJSON_IsTrue(cJSON_GetObjectItem(resources, "regex"));
  ret->icase = cJSON_IsTrue(cJSON_GetObjectItem(resources, "icase"));
  ret->image = cJSON_GetStringValue(cJSON_GetObjectItem(resources, "image"));

  return ret->pattern != NULL && ret->image != NULL;
}

static int precompile_regexes_iter(cJSON *array, char ***compiled_raw,
                                   pcre2_code ***compiled,
                                   int *compiled_count) {
  cJSON *elem;

  cJSON_ArrayForEach(elem, array) {
    struct Resource resource;

    parse_resource(elem, &resource);

    if (resource.regexp) {
      pcre2_code *reg;
      int errornumber;
      size_t erroroffset;

      reg = pcre2_compile((PCRE2_SPTR)resource.pattern, PCRE2_ZERO_TERMINATED,
                          0, &errornumber, &erroroffset, NULL);
      if (reg == NULL) {
        PCRE2_UCHAR buffer[256];
        pcre2_get_error_message(errornumber, buffer, 256);
        log_msg(ERROR, "precompile_regexes_iter", 0,
                "couldn't compile regex '%s'. PCRE2 error: %d offset: %lu - %s",
                resource.pattern, errornumber, erroroffset, buffer);

        continue;
      }

      ((*compiled)[*compiled_count]) = reg;
      int len = strlen(resource.pattern);

      ((*compiled_raw)[*compiled_count]) = malloc(len + 1);
      if (((*compiled_raw)[*compiled_count]) == NULL) {
        log_msg(ERROR, "precompile_regexes_iter", 0,
                "couldn't allocate %d bytes for %s", len + 1, resource.pattern);
        return 0;
      }

      memcpy(((*compiled_raw)[*compiled_count]), resource.pattern, len);
      ((*compiled_raw)[*compiled_count])[len] = '\0';
      *compiled_count += 1;
    }
  }

  return 1;
}

static int query_resource_iter_list(cJSON *list, char *name,
                                    char **compiled_raw, pcre2_code **compiled,
                                    int compiled_count, char **out_url) {
  int status = 0;
  cJSON *elem;
  cJSON_ArrayForEach(elem, list) {
    struct Resource resource;
    parse_resource(elem, &resource);

    if (resource.regexp) {
      for (int i = 0; i < compiled_count; i++) {
        if (strcmp(compiled_raw[i], resource.pattern) == 0) {
          pcre2_match_data *match_data;
          pcre2_code *compiled_regex = compiled[i];
          match_data =
              pcre2_match_data_create_from_pattern(compiled_regex, NULL);

          if (pcre2_match(compiled_regex, (PCRE2_SPTR)name, strlen(name), 0, 0,
                          match_data, NULL) >= 0) {
            *out_url = resource.image;
            status = 1;
          }
          pcre2_match_data_free(match_data);
        }
      }
    } else {
      if ((!resource.icase && strcmp(resource.pattern, name) == 0) ||
          (resource.icase && strcasecmp(resource.pattern, name) == 0)) {
        *out_url = resource.image;
        status = 1;
      }
    }
  }

  return status;
}

int load_resources(char *file, char **out_content, cJSON **out_json) {
  long size;
  FILE *fp = fopen(file, "rb");

  if (fp == NULL) {
    log_msg(ERROR, "load_resources", 0, "could not open file: %s\n", file);
    return 0;
  }

  fseek(fp, 0L, SEEK_END);
  size = ftell(fp);
  rewind(fp);

  *out_content = calloc(1, size + 1);
  if (!(*out_content)) {
    fclose(fp);
    log_msg(ERROR, "load_resources", 0,
            "allocation memory for reading resource file failed");
    return 0;
  }

  if (1 != fread(*out_content, size, 1, fp)) {
    fclose(fp);
    free(*out_content);
    log_msg(ERROR, "load_resources", 0, "reading resource file failed");
    return 0;
  }

  *out_json = cJSON_Parse(*out_content);

  fclose(fp);

  return 1;
}

// TODO: replace three output arguments with a single linked list
int precompile_regexes(cJSON *resources, char ***compiled_raw,
                       pcre2_code ***compiled, int *compiled_count) {
  cJSON *elem;
  int size = 0;
  int str_size = 0;

  cJSON_ArrayForEach(elem, cJSON_GetObjectItem(resources, "wm")) {
    struct Resource resource;

    parse_resource(elem, &resource);

    if (resource.regexp) {
      size += 1;
      str_size += strlen(resource.pattern);
    }
  }

  cJSON_ArrayForEach(elem, cJSON_GetObjectItem(resources, "app")) {
    struct Resource resource;

    parse_resource(elem, &resource);

    if (resource.regexp) {
      size += 1;
      str_size += strlen(resource.pattern);
    }
  }

  // this is ok
  *compiled_raw = malloc(size * sizeof(char *));
  *compiled = malloc(size * sizeof(pcre2_code *));
  *compiled_count = 0;

  if (compiled_raw == NULL) {
    log_msg(ERROR, "precompile_regexes", 0,
            "couldn't allocate %lu bytes for compiled_raw array",
            size * sizeof(char *));
    return 0;
  }
  if (compiled == NULL) {
    log_msg(ERROR, "precompile_regexes", 0,
            "couldn't allocate %lu bytes for compiled array",
            size * sizeof(pcre2_code *));
    return 0;
  }

  precompile_regexes_iter(cJSON_GetObjectItem(resources, "wm"), compiled_raw,
                          compiled, compiled_count);
  precompile_regexes_iter(cJSON_GetObjectItem(resources, "app"), compiled_raw,
                          compiled, compiled_count);
  precompile_regexes_iter(cJSON_GetObjectItem(resources, "os"), compiled_raw,
                          compiled, compiled_count);

  return size == 0 && str_size == 0;
}

int query_resource(char *name, char *return_url, int len, enum query_type type,
                   cJSON *resources, char **compiled_raw, pcre2_code **compiled,
                   int compiled_count) {
  int status = 0;

  if (resources == NULL) {
    log_msg(WARN, "query_resource", 0, "null resource pointer passed");
    return status;
  }

  switch (type) {
  case APP: {
    char *url = FALLBACK_APP;
    cJSON *app_obj = cJSON_GetObjectItem(resources, "app");

    status = query_resource_iter_list(app_obj, name, compiled_raw, compiled,
                                      compiled_count, &url);

    strncpy(return_url, url, len - 1);
    return_url[len - 1] = '\0';
    break;
  }
  case OS: {
    char *url = FALLBACK_OS;
    cJSON *os_obj = cJSON_GetObjectItem(resources, "os");

    status = query_resource_iter_list(os_obj, name, compiled_raw, compiled,
                                      compiled_count, &url);

    strncpy(return_url, url, len - 1);
    return_url[len - 1] = '\0';
    break;
  }
  case WM: {
    char *url = FALLBACK_WM;
    cJSON *wm_obj = cJSON_GetObjectItem(resources, "wm");

    status = query_resource_iter_list(wm_obj, name, compiled_raw, compiled,
                                      compiled_count, &url);

    strncpy(return_url, url, len - 1);
    return_url[len - 1] = '\0';
    break;
  }
  }

  return status;
}

int query_fallback(char *name, char *return_url, int len, enum query_type type,
                   cJSON *empty, char **compiled_raw, pcre2_code **compiled,
                   int compiled_count) {
  if (empty != NULL) {
    log_msg(WARN, "query_fallback", 0,
            "Called fallback action on non-empty resource.");
  }

  char *from;

  switch (type) {
  case OS: {
    from = FALLBACK_OS;
    break;
  }
  case WM: {
    from = FALLBACK_WM;
    break;
  }
  case APP: {
    from = FALLBACK_APP;
    break;
  }
  default:
    return 0;
  }

  strncpy(return_url, from, len - 1);
  return_url[len - 1] = '\0';

  return 1;
}

int append_missing_names(char *file, char *name) {
  FILE *fp;
  char *line;
  size_t len;
  char line_to_append[strlen(name) + 1];

  line = NULL; // setting this to null will require freeing it after getline()

  sprintf(line_to_append, "%s\n", name);

  if (!file || !(name)) {
    return 0;
  }

  fp = fopen(file, "a+");

  if (fp == NULL) {
    log_msg(ERROR, "append_missing_names", 0, "Couldn't open file: %s", file);
    return 0;
  }

  while (getline(&line, &len, fp) >= 0) {
    if (strcmp(line, line_to_append) == 0) {
      log_msg(INFO, "append_missing_names", 1, "already found %s, skipping",
              name);
      free(line);
      fclose(fp);
      return 2;
    }
  }

  fputs(line_to_append, fp);
  free(line);
  fclose(fp);

  return 1;
}
