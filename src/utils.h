#include "config.h"

#include <glib/gstdio.h>

#define debug(args ...) if(LOG_DEBUG){ fprintf(stderr, "DEBUG: " args); }
#define trace(args ...) if(LOG_TRACE){ fprintf(stderr, "TRACE: " args); }
#define warn(args ...) if(LOG_WARN){ fprintf(stderr, "WARN:  " args); }
#define error(args ...) fprintf(stderr, "ERROR: " args);

const char* clip_config_get_home_dir(void);
const char* clip_config_get_storage_file(void);

void clip_config_destroy();
