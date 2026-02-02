#ifndef MAIN_H
#define MAIN_H
#include <gtk-4.0/gtk/gtk.h>

extern GtkApplication *global_app;
extern GtkWidget *global_window;

void switch_to_chat_window();
void switch_to_login_window();

#endif