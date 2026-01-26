#include <stdio.h>
#include <gtk-4.0/gtk/gtk.h>
#include "client/gui/creation.h"
#include "client/gui/login.h"

#include "lib/constants.h"
#include "client/handlers/response_handler.h"
#include "client/modules/client.h"

typedef struct {
    GtkApplication *app;
    GtkWidget *window;
} AppData;

static void on_create_account_button(GtkButton *button, gpointer user_data) {
    GtkWidget **entries = (GtkWidget **)user_data;
    GtkWidget *username_entry = entries[0];
    GtkWidget *password_entry = entries[1];
    
    const char *username = gtk_editable_get_text(GTK_EDITABLE(username_entry));
    const char *password = gtk_editable_get_text(GTK_EDITABLE(password_entry));

    int username_len = strlen(username);
    int password_len = strlen(password);

    if(username_len < NAME_MIN_SIZE || username_len > NAME_MAX_SIZE || password_len < PASSWORD_MIN_SIZE || password_len > PASSWORD_MAX_SIZE){
        GtkWidget *dialog = gtk_message_dialog_new(NULL,
                                                   GTK_DIALOG_MODAL,
                                                   GTK_MESSAGE_ERROR,
                                                   GTK_BUTTONS_OK,
                                                   "Username or password length is invalid!");
        gtk_window_present(GTK_WINDOW(dialog));
        g_signal_connect_swapped(dialog, "response", G_CALLBACK(gtk_window_destroy), dialog);
        return;
    }

    g_print("Starting account creation sequence...\n");
    if(start_request(RESPONSE_REGISTER)){
        // Log in sequence
        if(create_account(username, password) == -1){
            signal_response(RESPONSE_REGISTER, FALSE);
        }        
        else{
            g_print("Account creation request started\n");
        }        
    } 
    else{
        g_print("Account creation request already in progress\n");
    }
}

static void on_back_to_login_button(GtkButton *button, gpointer user_data) {
    AppData *data = (AppData *)user_data;
    show_login_window(data->app, data->window);
}

void show_create_account_window(GtkApplication *app, GtkWidget *window) {
    GtkWidget *grid;
    GtkWidget *username_label, *password_label;
    GtkWidget *username_entry, *password_entry;
    GtkWidget *create_button, *back_button;
    static GtkWidget *entries[2];
    static AppData app_data;
    
    GtkWidget *child = gtk_window_get_child(GTK_WINDOW(window));
    if (child) {
        gtk_window_set_child(GTK_WINDOW(window), NULL);
    }
    
    gtk_window_set_title(GTK_WINDOW(window), "Create Account");
    
    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_widget_set_margin_start(grid, 20);
    gtk_widget_set_margin_end(grid, 20);
    gtk_widget_set_margin_top(grid, 20);
    gtk_widget_set_margin_bottom(grid, 20);
    gtk_widget_set_halign(grid, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(grid, GTK_ALIGN_CENTER);
    gtk_window_set_child(GTK_WINDOW(window), grid);
    
    username_label = gtk_label_new("Username:");
    gtk_widget_set_halign(username_label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), username_label, 0, 0, 1, 1);
    
    username_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(username_entry), "Choose username");
    gtk_widget_set_size_request(username_entry, 200, -1);
    gtk_grid_attach(GTK_GRID(grid), username_entry, 1, 0, 1, 1);
    
    password_label = gtk_label_new("Password:");
    gtk_widget_set_halign(password_label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), password_label, 0, 1, 1, 1);
    
    password_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(password_entry), "Choose password");
    gtk_entry_set_visibility(GTK_ENTRY(password_entry), FALSE);
    gtk_widget_set_size_request(password_entry, 200, -1);
    gtk_grid_attach(GTK_GRID(grid), password_entry, 1, 1, 1, 1);
    
    create_button = gtk_button_new_with_label("Create");
    gtk_widget_set_size_request(create_button, 100, -1);
    gtk_widget_set_halign(create_button, GTK_ALIGN_CENTER);
    gtk_grid_attach(GTK_GRID(grid), create_button, 0, 2, 2, 1);
    
    back_button = gtk_button_new_with_label("Back to Login");
    gtk_widget_set_size_request(back_button, 100, -1);
    gtk_widget_set_halign(back_button, GTK_ALIGN_CENTER);
    gtk_grid_attach(GTK_GRID(grid), back_button, 0, 3, 2, 1);
    
    entries[0] = username_entry;
    entries[1] = password_entry;
    
    app_data.app = app;
    app_data.window = window;
    
    g_signal_connect(create_button, "clicked", G_CALLBACK(on_create_account_button), entries);
    g_signal_connect(back_button, "clicked", G_CALLBACK(on_back_to_login_button), &app_data);
}