#pragma once

#include <stdio.h>
#include <string.h>

// X11 libs
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "logger.h"

/**
 * @brief get window manager name
 *
 * @param disp current display
 * @param name return window manager name
 * @param length max name length
 * @return 1 if succeeded, 0 otherwise
 */
int wm_info(Display *disp, char *name, int length);

/**
 * @brief get active window name
 *
 * @param disp current display
 * @param name return window name
 * @param length max name length
 * @return 1 if succeeded, 0 otherwise
 */
int get_active_window_name(Display *disp, char *name, int length);
