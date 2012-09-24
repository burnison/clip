#include "clipboard.h"
#include "provider.h"

void clip_daemon_init(ClipboardProvider* provider, Clipboard* clipboard);
void clip_daemon_destroy(void);

void clip_daemon_start(void);
