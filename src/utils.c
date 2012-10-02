#include "utils.h"

static char* home;
static char* storage;

// Freed by destroy.
const char* clip_config_get_home_dir(void)
{
    if(home == NULL){
        home = g_build_path("/", g_get_home_dir(), CLIP_HOME, NULL);
    }
    return home;
}

// Freed by destroy.
const char* clip_config_get_storage_file(void)
{
    if(storage == NULL){
        storage = g_build_path("/", clip_config_get_home_dir(), HISTORY_FILE, NULL);
    }
    return storage;
}

void clip_config_destroy()
{
    g_free(home);
    home = NULL;

    g_free(storage);
    storage = NULL;
}
