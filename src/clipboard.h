typedef struct clipboard Clipboard; 

Clipboard* clip_clipboard_new(void* provider);
void clip_clipboard_free(Clipboard* clipboard);

void* clip_clipboard_get_provider(Clipboard* clipboard);

char* clip_clipboard_get_text(Clipboard* clipboard);
void clip_clipboard_set_text(Clipboard* clipboard, char* text);
