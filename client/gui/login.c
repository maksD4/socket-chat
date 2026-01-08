#include <stdio.h>
#include <gtk-4.0/gtk/gtk.h>
#include "client/gui/login.h"
#include "client/gui/creation.h"

typedef struct {
    GtkApplication *app;
    GtkWidget *window;
} AppData;

typedef struct {
    GMutex mutex;
    GCond cond;
    gboolean response_received;
    gboolean timeout_occurred;
    gboolean success;
} SignalData;

static gboolean on_timeout(gpointer user_data) {
    SignalData *signal_data = (SignalData *) user_data;

    g_mutex_lock(&signal_data->mutex);

    if(!signal_data->response_received){
        signal_data->timeout_occurred = TRUE;
        g_print("Timeout occured - no response within 5 seconds!\n");

        // Send signal
        g_cond_signal(&signal_data->cond);
    }
    
    g_mutex_unlock(&signal_data->mutex);
    
    return G_SOURCE_REMOVE;
}

static void simulate_server_response(gpointer user_data) {
    SignalData *signal_data = (SignalData *)user_data;
    
    // Simulate a delay (you can adjust this to test timeout)
    g_usleep(2000000); // 2 second delay - change to 6000000 (6 seconds) to test timeout
    
    g_mutex_lock(&signal_data->mutex);

    if(!signal_data->timeout_occurred){
        signal_data->response_received = TRUE;
        signal_data->success = TRUE;
        g_print("Success signal!");

        // Send signal
        g_cond_signal(&signal_data->cond);
    }

    g_mutex_unlock(&signal_data->mutex);

    return;
}

static gboolean show_result_dialog(gpointer user_data) {
    SignalData *signal_data = (SignalData *)user_data;
    
    if (signal_data->timeout_occurred) {
        GtkWidget *dialog = gtk_message_dialog_new(NULL,
                                                   GTK_DIALOG_MODAL,
                                                   GTK_MESSAGE_ERROR,
                                                   GTK_BUTTONS_OK,
                                                   "Login timeout! Server did not respond.");
        gtk_window_present(GTK_WINDOW(dialog));
        g_signal_connect_swapped(dialog, "response", G_CALLBACK(gtk_window_destroy), dialog);
        
        // Exit after showing dialog
        g_timeout_add_seconds(2, (GSourceFunc)exit, GINT_TO_POINTER(EXIT_FAILURE));
    } else if (signal_data->response_received && signal_data->success) {
        GtkWidget *dialog = gtk_message_dialog_new(NULL,
                                                   GTK_DIALOG_MODAL,
                                                   GTK_MESSAGE_INFO,
                                                   GTK_BUTTONS_OK,
                                                   "Login Successful!");
        gtk_window_present(GTK_WINDOW(dialog));
        g_signal_connect_swapped(dialog, "response", G_CALLBACK(gtk_window_destroy), dialog);
    } else {
        GtkWidget *dialog = gtk_message_dialog_new(NULL,
                                                   GTK_DIALOG_MODAL,
                                                   GTK_MESSAGE_ERROR,
                                                   GTK_BUTTONS_OK,
                                                   "Invalid username or password!");
        gtk_window_present(GTK_WINDOW(dialog));
        g_signal_connect_swapped(dialog, "response", G_CALLBACK(gtk_window_destroy), dialog);
    }
    
    return G_SOURCE_REMOVE;
}

static void on_login_button(GtkButton *button, gpointer user_data) {
    GtkWidget **entries = (GtkWidget **)user_data;
    GtkWidget *username_entry = entries[0];
    GtkWidget *password_entry = entries[1];
    
    const char *username = gtk_editable_get_text(GTK_EDITABLE(username_entry));
    const char *password = gtk_editable_get_text(GTK_EDITABLE(password_entry));
    
    static TimeoutData timeout_data;
    timeout_data.response_received = FALSE;

    // Set 5 second timeout
    timeout_data.timeout_id = g_timeout_add_seconds(5, on_timeout, &timeout_data);

    // Simulate server response in a separate thread
    g_thread_new("auth_thread", (GThreadFunc)simulate_server_response, &timeout_data);


    // Wait for signal

    
    // Signal was sent    
    if (timeout_data.response_received) {
        // Check credentials after receiving response
        if (g_strcmp0(username, "admin") == 0 && g_strcmp0(password, "password") == 0) {
            g_print("Login successful!\n");
            GtkWidget *dialog = gtk_message_dialog_new(NULL,
                                                       GTK_DIALOG_MODAL,
                                                       GTK_MESSAGE_INFO,
                                                       GTK_BUTTONS_OK,
                                                       "Login Successful!");
            gtk_window_present(GTK_WINDOW(dialog));
            g_signal_connect(dialog, "response", G_CALLBACK(gtk_window_destroy), NULL);
        } else {
            g_print("Login failed!\n");
            GtkWidget *dialog = gtk_message_dialog_new(NULL,
                                                       GTK_DIALOG_MODAL,
                                                       GTK_MESSAGE_ERROR,
                                                       GTK_BUTTONS_OK,
                                                       "Invalid username or password!");
            gtk_window_present(GTK_WINDOW(dialog));
            g_signal_connect(dialog, "response", G_CALLBACK(gtk_window_destroy), NULL);
        }
    }
}

static void on_create_account_button(GtkButton *button, gpointer user_data) {
    AppData *data = (AppData *)user_data;
    show_create_account_window(data->app, data->window);
}

void show_login_window(GtkApplication *app, GtkWidget *window) {
    GtkWidget *grid;
    GtkWidget *username_label, *password_label;
    GtkWidget *username_entry, *password_entry;
    GtkWidget *login_button, *create_account_button;
    static GtkWidget *entries[2];
    static AppData app_data;
    
    GtkWidget *child = gtk_window_get_child(GTK_WINDOW(window));
    if (child) {
        gtk_window_set_child(GTK_WINDOW(window), NULL);
    }
    
    gtk_window_set_title(GTK_WINDOW(window), "Login");
    
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
    gtk_entry_set_placeholder_text(GTK_ENTRY(username_entry), "Enter username");
    gtk_widget_set_size_request(username_entry, 200, -1);
    gtk_grid_attach(GTK_GRID(grid), username_entry, 1, 0, 1, 1);
    
    password_label = gtk_label_new("Password:");
    gtk_widget_set_halign(password_label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), password_label, 0, 1, 1, 1);
    
    password_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(password_entry), "Enter password");
    gtk_entry_set_visibility(GTK_ENTRY(password_entry), FALSE);
    gtk_widget_set_size_request(password_entry, 200, -1);
    gtk_grid_attach(GTK_GRID(grid), password_entry, 1, 1, 1, 1);
    
    login_button = gtk_button_new_with_label("Login");
    gtk_widget_set_size_request(login_button, 100, -1);
    gtk_widget_set_halign(login_button, GTK_ALIGN_CENTER);
    gtk_grid_attach(GTK_GRID(grid), login_button, 0, 2, 2, 1);
    
    create_account_button = gtk_button_new_with_label("Create Account");
    gtk_widget_set_size_request(create_account_button, 100, -1);
    gtk_widget_set_halign(create_account_button, GTK_ALIGN_CENTER);
    gtk_grid_attach(GTK_GRID(grid), create_account_button, 0, 3, 2, 1);
    
    entries[0] = username_entry;
    entries[1] = password_entry;
    
    app_data.app = app;
    app_data.window = window;
    
    g_signal_connect(login_button, "clicked", G_CALLBACK(on_login_button), entries);
    g_signal_connect(create_account_button, "clicked", G_CALLBACK(on_create_account_button), &app_data);
}
