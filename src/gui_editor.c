/**
 * Copyright (C) 2012 Richard Burnison
 *
 * This file is part of Clip.
 *
 * Clip is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Clip is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Clip.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "gui_editor.h"
#include "config.h"
#include "utils.h"

#include <gtk/gtk.h>

struct widgets {
    GtkDialog* dialog;
    GtkTextBuffer* buffer;
};

static void clip_gui_editor_create_dialog(struct widgets* widgets, char* text)
{
    // Create the textbox and buffer.
    GtkWidget* textbox = gtk_text_view_new();
    GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textbox));
    gtk_text_buffer_set_text(buffer, text == NULL ? "" : text, -1);

    // Set-up the dialog box.
    GtkWidget* dialog = gtk_dialog_new_with_buttons(PROGRAM, NULL, GTK_DIALOG_MODAL,
            GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
            GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
            NULL);
    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 600, 400);

    // Add the textbox to the dialog.
    GtkWidget* content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_box_pack_start(GTK_BOX(content), textbox, TRUE, TRUE, 0);

    // Populate.
    widgets->dialog = (GtkDialog*)dialog;
    widgets->buffer = buffer;
}


char* clip_gui_editor_edit_text(char* text)
{
    struct widgets ui = {0};
    clip_gui_editor_create_dialog(&ui, text);

    gtk_widget_show_all(GTK_WIDGET(ui.dialog));
    gint result = gtk_dialog_run(ui.dialog);

    char* edited = NULL;
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

void clip_gui_editor_free_text(char* text)
{
    g_free(text);
}
