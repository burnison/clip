/*
 * Copyright (c) 2016 Richard Burnison
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "gui_editor.h"
#include "config.h"
#include "utils.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

struct widgets {
    GtkDialog *dialog;
    GtkTextBuffer *buffer;
};

static void clip_gui_editor_create_dialog(struct widgets *widgets, char *text)
{
    // Create the textbox and buffer.
    GtkWidget *textbox = gtk_text_view_new();
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textbox));
    gtk_text_buffer_set_text(buffer, text == NULL ? "" : text, -1);

    // Set-up the dialog box.
    GtkWidget *dialog = gtk_dialog_new_with_buttons(PROGRAM, NULL, GTK_DIALOG_MODAL,
            _("OK"), GTK_RESPONSE_ACCEPT,
            _("CANCEL"), GTK_RESPONSE_REJECT,
            NULL);
    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 600, 400);

    // Add the textbox to the dialog.
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_box_pack_start(GTK_BOX(content), textbox, TRUE, TRUE, 0);

    // Populate.
    widgets->dialog = (GtkDialog*)dialog;
    widgets->buffer = buffer;
}


char* clip_gui_editor_edit_text(char *text)
{
    struct widgets ui = {0};
    clip_gui_editor_create_dialog(&ui, text);

    gtk_widget_show_all(GTK_WIDGET(ui.dialog));
    gint result = gtk_dialog_run(ui.dialog);

    char *edited = NULL;
    if(result == GTK_RESPONSE_ACCEPT){
        GtkTextIter start, end;
        gtk_text_buffer_get_bounds(ui.buffer, &start, &end);
        if(gtk_text_iter_get_offset(&end) > 0){
            edited = gtk_text_buffer_get_text(ui.buffer, &start, &end, FALSE);
        } else {
            debug("Edited text is empty. Ignoring edit request.\n");
        }
    }

    gtk_widget_destroy(GTK_WIDGET(ui.dialog));
    return edited;
}

void clip_gui_editor_free_text(char *text)
{
    g_free(text);
}
