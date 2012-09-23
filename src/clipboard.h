typedef struct clipboard Clipboard; 

Clipboard* clip_new_clipboard(void* provider);
void clip_free_clipboard(Clipboard* clipboard);


void* clip_get_provider(Clipboard* clipboard);


char* clip_get_text(Clipboard* clipboard);
void clip_set_text(Clipboard* clipboard, char* text);
