#include "logger.h"

int debug = 0;

void set_debug(int val) { debug = val; }
void log_format(FILE *restrict stream, const char *tag, const char *source,
                const char *message, va_list args) {
  long now_seconds = time(NULL);
  char formatted_time[sizeof "0000-00-00 00:00:00"];
  strftime(formatted_time, sizeof(formatted_time), "%Y-%m-%d %H:%M:%S",
           localtime(&now_seconds));
  fprintf(stream, source ? "%s [%s] %s: " : "%s [%s] ", formatted_time, tag,
          source ? source : NULL);
  vfprintf(stream, message, args);
  printf("\n");
}

void log_msg(enum log_type type, const char *source, int d, const char *format,
             ...) {
  if (!d || debug) {
    va_list ap;
    va_start(ap, format);
    log_format(type == ERROR ? stderr : stdout, log_type_name[type], source,
               format, ap);
    va_end(ap);
  }
}
