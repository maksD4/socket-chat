#include <stdio.h>
#include <gtk-4.0/gtk/gtk.h>
#include "client/gui/app.h"
#include "client/gui/login.h"

static void activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window;
    
    window = gtk_application_window_new(app);
    gtk_window_set_default_size(GTK_WINDOW(window), 300, 200);
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
    
    show_login_window(app, window);
    
    gtk_window_present(GTK_WINDOW(window));
}

int run_app(int argc, char **argv){
    GtkApplication *app;
    int status;
    
    app = gtk_application_new("com.example.login", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    
    return status;
}
