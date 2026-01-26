#ifndef APP_H
#define APP_H
#include <gtk-4.0/gtk/gtk.h>

void show_chat_window(GtkApplication *app, GtkWidget *window);
void refresh_current_chat();

#endif