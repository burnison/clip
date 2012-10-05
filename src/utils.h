#include "config.h"

#include <glib/gstdio.h>

#define LOG_PREFIX ""
#define LOG_SUFFIX __FILE__ " - "

#define trace(args ...) if(LOG_TRACE){ fprintf(stderr, LOG_PREFIX "TRACE - " LOG_SUFFIX args); }
#define debug(args ...) if(LOG_DEBUG){ fprintf(stderr, LOG_PREFIX "DEBUG - " LOG_SUFFIX args); }
#define warn(args ...) if(LOG_WARN){ fprintf(stderr, LOG_PREFIX "WARN  - " LOG_SUFFIX args); }
#define error(args ...) fprintf(stderr, LOG_PREFIX "ERROR - " LOG_SUFFIX args);

const char* clip_config_get_home_dir(void);
const char* clip_config_get_storage_file(void);

void clip_config_destroy();
