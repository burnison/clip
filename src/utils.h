#include "config.h"

#include <stdio.h>

#define debug(args ...) if(DEBUG) fprintf(stderr, "DEBUG: " args)
#define trace(args ...) if(TRACE) fprintf(stderr, "TRACE: " args)

