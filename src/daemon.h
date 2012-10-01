#include "clipboard.h"
#include "provider.h"

typedef struct daemon Daemon;

Daemon* clip_daemon_new(ClipboardProvider* provider, Clipboard* clipboard);
void clip_daemon_free(Daemon* daemon);

void clip_daemon_start(Daemon* daemon);
