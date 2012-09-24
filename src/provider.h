typedef struct provider ClipboardProvider;

ClipboardProvider* clip_provider_new(void);
void clip_provider_free(ClipboardProvider* provider);

char* clip_provider_get_current(ClipboardProvider* provider);
void clip_provider_free_current(char* current);

void clip_provider_set_current(ClipboardProvider* provider, char* text);
