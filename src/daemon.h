#include "clipboard.h"
#include "provider.h"

typedef struct subscriber DaemonSubscriber;
typedef void (*clipboard_changed_callback)(Clipboard* clipboard);

void clip_daemon_init(ClipboardProvider* provider, Clipboard* clipboard);
void clip_daemon_destroy(void);

void clip_daemon_start(void);
