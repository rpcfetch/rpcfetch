#include "updater.h"

#ifndef OFFLINE

struct url_data {
  size_t size;
  char *data;
};

static size_t write_data(void *ptr, size_t size, size_t nmemb,
                         struct url_data *data) {
  size_t index = data->size;
  size_t n = (size * nmemb);
  char *tmp;

  data->size += (size * nmemb);

  tmp = realloc(data->data, data->size + 1); /* +1 for '\0' */

  if (tmp) {
    data->data = tmp;
  } else {
    if (data->data) {
      free(data->data);
    }
    log_msg(ERROR, "write_data", 0, "Failed to allocate memory.");
    return 0;
  }

  memcpy((data->data + index), ptr, n);
  data->data[data->size] = '\0';

  return size * nmemb;
}

static char *handle_url(char *url) {
  CURL *curl;

  struct url_data data;
  data.size = 0;
  data.data = malloc(4096); /* reasonable size initial buffer */
  if (NULL == data.data) {
    log_msg(ERROR, "handle_url", 0, "Failed to allocate memory.");
    return NULL;
  }

  data.data[0] = '\0';

  CURLcode res;

  curl = curl_easy_init();
  if (curl) {
    char user_agent[256];

    snprintf(user_agent, sizeof(user_agent), "%s/%s", PROGRAM_NAME,
             PROGRAM_VERSION);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, user_agent);

    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
      log_msg(ERROR, "handle_url", 0, "curl_easy_perform() failed: %s",
              curl_easy_strerror(res));
    }

    curl_easy_cleanup(curl);
  }
  return data.data;
}

static int compare_versions(char *current_version, char *new_version) {
  semver_t c = {};
  semver_t n = {};

  if (semver_parse(current_version, &c) || semver_parse(new_version, &n)) {
    log_msg(ERROR, "compare_versions", 0, "Invalid semver string (%s, %s)",
            current_version, new_version);
    return -1;
  }

  int result = semver_compare(n, c);
  semver_free(&c);
  semver_free(&n);

  return result;
}

static int get_current_version(char *file, char *ret_version, int length) {
  char *temp;
  long size;
  cJSON *json;
  FILE *fp = fopen(file, "r");
  ret_version[0] = '\0';

  if (fp == NULL) {
    log_msg(ERROR, "get_current_version", 0, "could not open file: %s", file);
    return 0;
  }

  fseek(fp, 0L, SEEK_END);
  size = ftell(fp);
  rewind(fp);

  temp = calloc(1, size + 1);
  if (!temp) {
    fclose(fp);
    log_msg(ERROR, "get_current_version", 0,
            "allocation memory for resource failed");
    return 0;
  }

  if (1 != fread(temp, size, 1, fp)) {
    fclose(fp);
    free(temp);
    log_msg(ERROR, "get_current_version", 0, "reading resource file failed");
    return 0;
  }

  json = cJSON_Parse(temp);

  if (json == NULL) {
    fclose(fp);
    free(temp);
    cJSON_free(json);
    log_msg(ERROR, "get_current_version", 0,
            "couldn't parse current version: invalid json content");

    return 0;
  }

  char *version = cJSON_GetStringValue(cJSON_GetObjectItem(json, "version"));

  if (version == NULL) {
    fclose(fp);
    free(temp);
    cJSON_free(json);
    log_msg(ERROR, "get_current_version", 0,
            "couldn't parse current version: version field not found");
    return 0;
  }

  strncpy(ret_version, version, length - 1);
  ret_version[length - 1] = '\0';

  fclose(fp);
  cJSON_free(json);
  free(temp);

  return 1;
}

static int read_version(cJSON *json, char **return_version) {
  *return_version = NULL;

  cJSON *version = cJSON_GetObjectItem(json, "name");

  if (version == NULL) {
    version = cJSON_GetObjectItem(json, "tag_name");
    if (version == NULL) {
      log_msg(ERROR, "read_version", 0, "release without valid version found!");
      return 0;
    }
  }

  char *version_str = cJSON_GetStringValue(version);

  if (!semver_is_valid(version_str) && version_str[0] != 'v') {
    version = cJSON_GetObjectItem(json, "tag_name");
    if (version == NULL) {
      log_msg(ERROR, "read_version", 0, "release without valid version found!");
      return 0;
    }
    version_str = cJSON_GetStringValue(version);
  }
  if (version_str[0] == 'v') {
    version_str += 1; // shift one byte to the left (effectively remove the
                      // first character)
  }
  *return_version = version_str;

  return 1;
}

static int download_update(char *download_url, char *filename) {
  int status = 0;
  char *data = handle_url(download_url);
  size_t data_len = strlen(data);
  log_msg(INFO, "download_update", 1, "Downloaded %lu bytes", data_len);

  if (data_len > 1) {
    FILE *f = fopen(filename, "w+");
    if (f == NULL) {
      log_msg(ERROR, "download_update", 0, "could not open file: %s", filename);
    } else {
      fputs(data, f);
      fclose(f);
      status = 1;
    }
  }

  free(data);
  return status;
}

static int parse_update_response(cJSON *json, char **name,
                                 char **download_url) {
  cJSON *assets = cJSON_GetObjectItem(json, "assets");
  if (assets == NULL || assets->child == NULL) {
    log_msg(ERROR, "parse_update_response", 0, "release without assets found");
    return 0;
  }

  cJSON *asset = assets->child;
  *name = cJSON_GetStringValue(cJSON_GetObjectItem(asset, "name"));
  *download_url =
      cJSON_GetStringValue(cJSON_GetObjectItem(asset, "browser_download_url"));

  return 1;
}

int check_for_updates(char *repo_url, char *file, char *ret_latest_version,
                      int length) {

  cJSON *json;
  int status;
  char *data;
  size_t data_len;
  char current_version[256];
  size_t current_version_length;

  status = 0;

  if (!get_current_version(file, current_version, sizeof(current_version))) {
    ret_latest_version[0] = '\0';
    return status;
  }

  strncpy(ret_latest_version, current_version, length - 1);
  ret_latest_version[length - 1] = '\0';

  log_msg(INFO, "check_for_updates", 1, "Fetching releases from %s", repo_url);
  data = handle_url(repo_url);
  data_len = strlen(data);

  if (data_len > 1) {
    char *new_version;
    size_t parsed_length;

    json = cJSON_Parse(data);

    if (read_version(json, &new_version)) {
      parsed_length = strlen(new_version);
      int valid_version = new_version != NULL && semver_is_valid(new_version);

      if (valid_version && compare_versions(current_version, new_version) > 0) {
        status = 1;

        strncpy(ret_latest_version, new_version, length - 1);
        ret_latest_version[length - 1] = '\0';
      }
    }
  }

  return status;
}

int update(char *repo_url, char *content_file) {
  cJSON *json;
  int status = 0;
  log_msg(INFO, "update", 1, "Fetching releases from %s", repo_url);
  char *data = handle_url(repo_url);
  size_t data_len = strlen(data);

  if (data_len > 1) {
    char *version;
    char *name;
    char *download_url;
    json = cJSON_Parse(data);

    if (read_version(json, &version) &&
        parse_update_response(json, &name, &download_url)) {
      log_msg(INFO, "update", 1, "Downloading %s from %s...", name,
              download_url);
      status = download_update(download_url, content_file);
    }
  }

  free(data);
  cJSON_Delete(json);
  return status;
}
#endif
