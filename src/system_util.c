#include "system_util.h"

/**
 * @brief parse line of x-release file
 *
 * @param line line to parse
 * @param offset character offset
 * @param out output pointer
 * @return 1 if succeeded, 0 otherwise
 */
static int parse_release_line(char *line, int offset, char **out) {
  char *tmp;
  int out_length;
  int strip;
  int len = strlen(line);
  strip = 0;
  if (len <= offset) {
    return 0;
  }

  tmp = malloc(len - offset);
  out_length = 0;
  for (int i = offset; i < len; i++) {
    if (i == offset && line[i] == '"') {
      strip = 1;
      continue;
    }

    if (strip && line[i] == '"')
      break;

    if (line[i] == '\n' || line[i] == '\r')
      continue;

    tmp[out_length++] = line[i];
  }

  *out = malloc(out_length + 1);
  strncpy(*out, tmp, out_length);
  (*out)[out_length] = '\0';
  free(tmp);

  return 1;
}

int get_os_release_name(char **name) {
  FILE *fp;
  char *line;
  size_t len;
  int status;
  ;

  fp = fopen("/etc/os-release", "r");
  line = NULL;
  status = 0;

  if (fp == NULL) {
    log_msg(INFO, "get_os_release_name", 1, "Could not open file: %s",
            "/etc/os-release");
    return 0;
  }

  while (getline(&line, &len, fp) != -1) {
    if (strncmp(line, "NAME=", strlen("NAME=")) == 0) {
      status = parse_release_line(line, strlen("NAME="), name);
      break;
    }
  }

  free(line);
  fclose(fp);

  return status;
}

int get_lsb_release_id(char **name) {
  FILE *fp;
  char *line;
  size_t len;
  int status;

  fp = fopen("/etc/lsb-release", "r");
  line = NULL;
  status = 0;

  if (fp == NULL) {
    log_msg(INFO, "get_lsb_release_id", 1, "Could not open file: %s",
            "/etc/lsb-release");
    return 0;
  }

  while (getline(&line, &len, fp) != -1) {
    if (strncmp(line, "DISTRIB_ID=", strlen("DISTRIB_ID=")) == 0) {
      status = parse_release_line(line, strlen("DISTRIB_ID="), name);
      break;
    }
  }

  free(line);
  fclose(fp);

  return status;
}

int get_os_name(char **name) {
#ifdef __linux__
  if (get_os_release_name(name) == 0 && get_lsb_release_id(name) == 0) {
    return 0;
  }
#elif defined(__APPLE__)
  *name = APPLE_NAME;
#elif defined(_WIN32)
  *name = WINDOWS_NAME;
#else
  log_msg(ERROR, "get_os_name", 0, "Error: could not find OS macro");
  *name = NULL;
  return 0;
#endif

  return 1;
}

double get_cpu_usage(unsigned long long *last_total_user,
                     unsigned long long *last_total_user_low,
                     unsigned long long *last_total_sys,
                     unsigned long long *last_total_idle) {
  double cpu_usage;
  FILE *fp;
  unsigned long long total_user, total_user_low, total_sys, total_idle, total;

  fp = fopen("/proc/stat", "r");
  if (fp == NULL) {
    log_msg(ERROR, "get_cpu_usage", 0, "Could not open file: %s", "/proc/stat");
    return -1.0;
  }

  fscanf(fp, "cpu %llu %llu %llu %llu", &total_user, &total_user_low,
         &total_sys, &total_idle);
  fclose(fp);

  if (!*last_total_user || !*last_total_user_low || !*last_total_sys ||
      !*last_total_idle) {
    cpu_usage = 0.0;
  } else if (total_user < *last_total_user ||
             total_user_low < *last_total_user_low ||
             total_sys < *last_total_sys || total_idle < *last_total_idle) {
    // Overflow detection. Just skip this value.
    cpu_usage = -1.0;
  } else {
    total = (total_user - *last_total_user) +
            (total_user_low - *last_total_user_low) +
            (total_sys - *last_total_sys);
    cpu_usage = total;
    total += (total_idle - *last_total_idle);
    cpu_usage /= total;
    cpu_usage *= 100;
  }

  *last_total_user = total_user;
  *last_total_user_low = total_user_low;
  *last_total_sys = total_sys;
  *last_total_idle = total_idle;

  return cpu_usage;
}

double get_memory_usage() {
  FILE *fp;
  double mem_usage;
  char *line = NULL;
  size_t len;
  long mem_total = -1;
  long mem_available = -1;
  long mem_used;

  fp = fopen("/proc/meminfo", "r");

  if (fp == NULL) {
    log_msg(ERROR, "get_memory_usage", 0, "Could not open file: %s",
            "/proc/meminfo");
    return -1.0;
  }

  while (getline(&line, &len, fp) >= 0) {
    if (strncmp(line, "MemTotal:", strlen("MemTotal:")) == 0) {
      mem_total = strtol(&line[strlen("MemTotal:")], NULL, 10);
    } else if (strncmp(line, "MemAvailable:", strlen("MemAvailable:")) == 0) {
      mem_available = strtol(&line[strlen("MemAvailable:")], NULL, 10);
    }
    if (mem_total != -1 && mem_available != -1) {
      break;
    }
  }

  free(line);
  fclose(fp);

  mem_used = mem_total - mem_available;
  mem_usage = (double)mem_used / mem_total * 100;

  return mem_usage;
}

double get_uptime() {
  FILE *fp;
  double uptime = 0;

  fp = fopen("/proc/uptime", "r");

  if (fp == NULL) {
    log_msg(ERROR, "get_uptime", 0, "Could not open file: %s", "/proc/uptime");
    return -1.0;
  }

  fscanf(fp, "%lf", &uptime);
  fclose(fp);

  return uptime;
}

long long millis_since_epoch() {
  struct timeval tv;

  gettimeofday(&tv, NULL);
  return (((long long)tv.tv_sec) * 1000) + (tv.tv_usec / 1000);
}
