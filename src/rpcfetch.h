#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "args.h"
#include "config.h"
#include "discord/discord_game_sdk.h"
#include "logger.h"
#include "resource_manager.h"
#include "system_util.h"
#include "updater.h"
#include "version.h"
#include "wm.h"

#define DISCORD_REQUIRE(x) assert(x == DiscordResult_Ok)

#define HOME_VARIABLE "HOME"

struct DiscordApplication {
  struct IDiscordCore *core;
  struct IDiscordUserManager *users;
  struct IDiscordAchievementManager *achievements;
  struct IDiscordActivityManager *activities;
  struct IDiscordRelationshipManager *relationships;
  struct IDiscordApplicationManager *application;
  struct IDiscordLobbyManager *lobbies;
  DiscordUserId user_id;
};
