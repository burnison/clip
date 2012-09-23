typedef struct clipboard Clipboard; 

Clipboard* clip_clipboard_new(void);
void clip_clipboard_free(Clipboard* clipboard);

char* clip_clipboard_get_head(Clipboard* clipboard);
void clip_clipboard_set_head(Clipboard* clipboard, char* text);
