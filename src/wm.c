#include "wm.h"

/**
 * @brief XGetWindowProperty function wrapper
 *
 * @param disp X display
 * @param win X window
 * @param xa_prop_type request property type
 * @param prop_name property name
 * @param ret output string
 * @param size output length
 * @return 1 if succeeded, 0 otherwise
 */
static int get_property(Display *disp, Window win, Atom xa_prop_type,
                        char *prop_name, char *ret, int size) {
  Atom xa_prop_name;
  Atom xa_ret_type;
  int ret_format;
  unsigned long ret_nitems;
  unsigned long ret_bytes_after;
  unsigned long tmp_size;
  unsigned char *ret_prop;

  ret_prop = NULL;

  xa_prop_name = XInternAtom(disp, prop_name, False);

  if (XGetWindowProperty(disp, win, xa_prop_name, 0, ~0L, False, xa_prop_type,
                         &xa_ret_type, &ret_format, &ret_nitems,
                         &ret_bytes_after, &ret_prop) != Success) {
    return 0;
  }

  if (ret_prop == NULL) {
    return 0;
  }

  if (xa_ret_type != xa_prop_type) {
    log_msg(WARN, "get_property", 0,
            "invalid return type received: %lu (expected %lu)", xa_ret_type,
            xa_prop_type);
    XFree(ret_prop);
    return 0;
  }

  tmp_size = (ret_format / (16 / sizeof(long))) * ret_nitems;
  tmp_size = size < tmp_size ? size : tmp_size;

  memcpy(ret, ret_prop, tmp_size - 1);

  ret[tmp_size - 1] = '\0';

  XFree(ret_prop);
  return 1;
}

int wm_info(Display *disp, char *name, int length) {
  Window sup_window[256];
  char wm_name[256];
  char *name_out;

  if (!get_property(disp, DefaultRootWindow(disp), XA_WINDOW,
                    "_NET_SUPPORTING_WM_CHECK", (char *)sup_window, 256) &&
      !get_property(disp, DefaultRootWindow(disp), XA_CARDINAL,
                    "_WIN_SUPPORTING_WM_CHECK", (char *)sup_window, 256)) {
    log_msg(ERROR, "wm_info", 0, "could not get window manager");
    return 0;
  }

  log_msg(ERROR, "wm_info", 0, "%d\n", *sup_window);

  /* WM_NAME */
  if (!get_property(disp, *sup_window, XInternAtom(disp, "UTF8_STRING", False),
                    "_NET_WM_NAME", wm_name, 256) &&
      !get_property(disp, *sup_window, XA_STRING, "_NET_WM_NAME", wm_name,
                    256)) {
    log_msg(ERROR, "wm_info", 0, "could not get window manager name");
    return 0;
  }

  strncpy(name, wm_name, length - 1);
  name[length - 1] = '\0';

  return 1;
}

int get_active_window_name(Display *disp, char *name, int length) {
  Window root = XDefaultRootWindow(disp);

  char prop[256];
  int prop_status =
      get_property(disp, root, XA_WINDOW, "_NET_ACTIVE_WINDOW", prop, 256);

  if (!prop_status || strlen(prop) == 0) {
    return 0;
  }

  XClassHint hint;
  int hint_status = XGetClassHint(disp, *((Window *)prop), &hint);

  if (!hint_status) {
    return 0;
  }

  strncpy(name, hint.res_class, length - 1);
  name[length - 1] = '\0';

  XFree(hint.res_name);
  XFree(hint.res_class);

  return 1;
}
