#include "config.h"

#include <glib/gstdio.h>

#define debug(args ...) if(DEBUG){ fprintf(stderr, "DEBUG: " args); }
#define trace(args ...) if(TRACE){ fprintf(stderr, "TRACE: " args); }
#define warn(args ...) if(WARN){ fprintf(stderr, "WARN: " args); }

