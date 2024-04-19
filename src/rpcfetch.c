#include "rpcfetch.h"

void UpdateActivityCallback(void *data, enum EDiscordResult result) {
  DISCORD_REQUIRE(result);
}

void build_activity(struct DiscordActivity *activity, char *window,
                    char *window_url, char *wm, char *os_name, char *os_url,
                    long uptime_epoch) {
  memset(activity, 0, sizeof(struct DiscordActivity));
  snprintf(activity->state, sizeof(activity->state), "%s", wm);
  snprintf(activity->details, sizeof(activity->details), "%s", window);

  struct DiscordActivityAssets activity_assets;
  memset(&activity_assets, 0, sizeof(activity_assets));
  snprintf(activity_assets.large_image, sizeof(activity_assets.large_image),
           "%s", os_url);
  snprintf(activity_assets.large_text, sizeof(activity_assets.large_text),
           "%s | %s/%s", os_name, PROGRAM_NAME, PROGRAM_VERSION);
  snprintf(activity_assets.small_image, sizeof(activity_assets.small_image),
           "%s", window_url);
  snprintf(activity_assets.small_text, sizeof(activity_assets.small_text), "%s",
           window);

  struct DiscordActivityTimestamps activity_timestamps;
  memset(&activity_timestamps, 0, sizeof(activity_timestamps));
  activity_timestamps.start = uptime_epoch;

  activity->assets = activity_assets;
  activity->timestamps = activity_timestamps;
}

int handler(Display *d, XErrorEvent *e) {
  if (e->error_code != BadWindow) {
    log_msg(ERROR, "x_error_handler", 0, "X error: %d - request: %d",
            e->error_code, e->request_code);
  }
  return 0;
}

int main(int argc, char **argv) {
  struct rpcfetch_arguments args;

  parse_args(argc, argv, &args);

  if (args.should_exit)
    return 0;

  set_debug(args.debug);

  // refer to resource_manager.h
  int (*query)(QUERY_ARGS) = &query_fallback;

  // TODO: add styles

  // the following values should be freed, however they live long enough to not
  // have to be cared about
  cJSON *resource;        /* free with cJSON_free() */
  char *resource_content; /* free with libc free() */

  // times for CPU usage calcuations
  unsigned long long cpu_total_user = 0, cpu_total_user_low = 0,
                     cpu_total_sys = 0, cpu_total_idle = 0;
  double cpu_usage = get_cpu_usage(&cpu_total_user, &cpu_total_user_low,
                                   &cpu_total_sys, &cpu_total_idle);
  long uptime_epoch = time(NULL) - get_uptime();

  char *home = getenv(HOME_VARIABLE);
  int valid_home = home != NULL && home[0] != '\0';
  char cache_path[1024];
  char resources_json_file[1024];
  char missing_names_file[1024];

  pcre2_code **compiled_regex;
  char **compiled_raw;
  int compiled_count = 0;

  struct rpcfetch_config config;
  char public_config_path[1024];
  snprintf(public_config_path, sizeof(public_config_path), "/etc/%s/%s.cfg",
           PROGRAM_NAME, PROGRAM_NAME);

  // *nix only
  if (!valid_home) {
    log_msg(WARN, NULL, 0, "couldn't get HOME environment variable!");
    query = &query_fallback;
    default_config(NULL, &config);
    if (access(public_config_path, F_OK) == 0) {
      log_msg(INFO, NULL, 1, "Using config file: %s", public_config_path);
      parse_config(public_config_path, &config);
    }
  } else {
    char user_config_path[1024];
    int resources_json_exists = 0;
    int config_status = 0;

    snprintf(cache_path, sizeof(cache_path), "%s/.cache/%s", home,
             PROGRAM_NAME);
    snprintf(missing_names_file, sizeof(missing_names_file), "%s/missing.log",
             cache_path);
    snprintf(user_config_path, sizeof(user_config_path), "%s/.config/%s/%s.cfg",
             home, PROGRAM_NAME, PROGRAM_NAME);

    log_msg(INFO, NULL, 1, "Using cache directory: %s", cache_path);
    log_msg(INFO, NULL, 1, "Using missing names file: %s", missing_names_file);

    default_config(cache_path, &config);
    if (args.config_file[0] != '\0') {
      if ((config_status = try_parse_config(args.config_file, &config)) == 2) {
        log_msg(ERROR, NULL, 0, "Couldn't find config file: %s",
                args.config_file);
        return 1;
      } else if (!config_status) {
        log_msg(ERROR, NULL, 0, "Couldn't parse config file: %s",
                args.config_file);
        return 1;
      }
    } else if ((config_status = try_parse_config(user_config_path, &config)) !=
               2) {
      if (!config_status) {
        log_msg(ERROR, NULL, 0, "Couldn't parse config file: %s",
                user_config_path);
        return 1;
      }
    } else if ((config_status =
                    try_parse_config(public_config_path, &config)) != 2) {
      if (!config_status) {
        log_msg(ERROR, NULL, 0, "Couldn't parse config file: %n",
                public_config_path);
        return 1;
      }
    }

    /* TODO: This logic is needed to create the parent directory for the
     * resources file if does not exist. This should be simplified.
     */
    if (config.resources_path[0] == '\0') {
      snprintf(resources_json_file, sizeof(resources_json_file), "%s/%s",
               config.resources_save_dir, config.resources_save_filename);
    } else {
      snprintf(resources_json_file, sizeof(resources_json_file), "%s",
               config.resources_path);
    }

    struct stat sb;
    if (stat(cache_path, &sb) == -1) {
      log_msg(INFO, NULL, 1, "mkdir %s", cache_path);
      mkdir(cache_path, 0777);
    } else if (access(resources_json_file, F_OK) == 0) {
      resources_json_exists = 1;
    }

#ifndef OFFLINE
    if (args.force_update ||
        (!resources_json_exists && config.auto_download && !args.offline)) {
      log_msg(INFO, NULL, 0, "Updating...");
      int status = update(config.resources_repo_url, resources_json_file);

      if (status) {
        log_msg(INFO, NULL, 0, "Successfully updated.");
      } else {
        log_msg(INFO, NULL, 0, "Couldn't update.");
      }

      return status;
    } else if (!resources_json_exists) {
      log_msg(WARN, NULL, 0,
              "Could not find resources file.\nYou might want to enable auto "
              "downloading or download it manually to %s",
              resources_json_file);
      query = &query_fallback;
    }

    if (resources_json_exists && config.check_for_updates && !args.offline) {
      char new_version[256];
      int update_available;

      update_available =
          check_for_updates(config.resources_repo_url, resources_json_file,
                            new_version, sizeof(new_version));

      if (update_available) {
        log_msg(WARN, NULL, 0, "(!) Resource update available: %s",
                new_version);
        log_msg(WARN, NULL, 0, "(!) Use `%s -U` to download the update!",
                argv[0]);
      }
    }
#endif
    if (resources_json_exists) {
      log_msg(INFO, NULL, 1, "Parsing resources file: %s", resources_json_file);

      int parsed =
          load_resources(resources_json_file, &resource_content, &resource);
      if (!parsed || !resource_content || !resource) {
        query = &query_fallback;
        log_msg(ERROR, NULL, 0,
                "(!!) Error: couldn't parse json data from `%s`, falling back "
                "to %s (OS) and %s (APP)",
                resources_json_file, FALLBACK_OS, FALLBACK_APP);
      } else {
        precompile_regexes(resource, &compiled_raw, &compiled_regex,
                           &compiled_count);
        query = &query_resource;
      }
    }
  }

  if (args.system_rate) {
    config.system_rate = args.system_rate;
  }
  if (args.window_rate) {
    config.window_rate = args.window_rate;
  }

  Display *d = XOpenDisplay(NULL);
  if (d == NULL) {
    fputs("Could not open display\n", stderr);
    return 1;
  }
  XSetErrorHandler(handler);
  char *os_name = NULL;
  char os_url[256];

  get_os_name(&os_name);
  log_msg(INFO, NULL, 1, "OS name: %s", os_name);

  query(os_name, os_url, sizeof(os_url), OS, resource, compiled_raw,
        compiled_regex, compiled_count);
  log_msg(INFO, NULL, 1, "OS image: %s", os_url);

  char oldname[256];
  oldname[0] = '\0';
  char wmname[256];
  char windowname[256];
  char window_url[256];
  windowname[0] = '\0';
  get_active_window_name(d, windowname, 256);

  char usage_string[256];

  double last_app_update = millis_since_epoch() - config.window_rate;
  double last_usage_update = millis_since_epoch() - config.system_rate;

  do {
    struct DiscordApplication app;
    memset(&app, 0, sizeof(app));

    struct DiscordCreateParams params;
    DiscordCreateParamsSetDefault(&params);
    params.client_id = config.discord_client_id;
    params.flags = DiscordCreateFlags_Default;
    params.event_data = &app;
    DISCORD_REQUIRE(DiscordCreate(DISCORD_VERSION, &params, &app.core));

    log_msg(INFO, NULL, 0, "Connected to Discord");

    app.users = app.core->get_user_manager(app.core);
    app.achievements = app.core->get_achievement_manager(app.core);
    app.activities = app.core->get_activity_manager(app.core);
    app.application = app.core->get_application_manager(app.core);
    app.lobbies = app.core->get_lobby_manager(app.core);
    app.relationships = app.core->get_relationship_manager(app.core);

    struct DiscordActivity activity;
    memset(&activity, 0, sizeof(activity));
    struct DiscordActivityAssets activity_assets;
    memset(&activity_assets, 0, sizeof(activity_assets));

    for (;;) {
      int discord_result = app.core->run_callbacks(app.core);
      if (discord_result != DiscordResult_Ok) {
        log_msg(WARN, NULL, 0, "Discord returned with result %d",
                discord_result);
        break;
      }

      long now = millis_since_epoch();

      int should_update_app = last_app_update + config.window_rate <= now;
      int should_update, should_update_usage = should_update =
                             last_usage_update + config.system_rate <= now;

      if (should_update_app || should_update_usage) {
        if (should_update_app) {
          get_active_window_name(d, windowname, 256);

          if (strlen(oldname) == 0 || strncmp(oldname, windowname, 255) != 0) {
            should_update = 1;
            last_app_update = now;
            int window_url_status =
                query(windowname, window_url, 256, APP, resource, compiled_raw,
                      compiled_regex, compiled_count);

            if (!window_url_status && valid_home) {
              int append_status =
                  append_missing_names(missing_names_file, windowname);
              if (append_status == 1) {
                log_msg(WARN, NULL, 0,
                        "(!) Couldn't locate '%s' in resources. A fallback "
                        "icon to be used for the application. Application name "
                        "logged to %s",
                        windowname, missing_names_file);
              }
            }
          }
        }
        if (should_update_usage) {
          cpu_usage = get_cpu_usage(&cpu_total_user, &cpu_total_user_low,
                                    &cpu_total_sys, &cpu_total_idle);
          int cpu_usage_rounded = round(cpu_usage);
          int mem_usage_rounded = round(get_memory_usage());

          sprintf(usage_string, "CPU: %d%% | RAM: %d%%", cpu_usage_rounded,
                  mem_usage_rounded);
          last_usage_update = now;
        }
        if (should_update) {
          build_activity(&activity, windowname, window_url, usage_string,
                         os_name,
                         os_url,
                         uptime_epoch);
          app.activities->update_activity(app.activities, &activity, &app,
                                          UpdateActivityCallback);

          log_msg(INFO, NULL, 1, "Updated activity to %s", windowname);

          strncpy(oldname, windowname, sizeof(oldname) - 1);
          oldname[sizeof(oldname) - 1] = '\0';
        }
      }

#ifdef _WIN32
      Sleep(16);
#else
      usleep(1000 * 16);
#endif
    }
  } while (args.retry_on_error);

  return 0;
}
