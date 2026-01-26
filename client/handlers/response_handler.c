#include "stdio.h"
#include <gtk-4.0/gtk/gtk.h>

#include "client/handlers/response_handler.h"
#include "lib/constants.h"

static ResponseCallback response_callbacks[6] = {NULL};

// Active requests - one per type
static GMutex active_requests_mutex;
static ResponseData *active_requests[6] = {NULL};

void response_handler_init() {
    g_mutex_init(&active_requests_mutex);
    for(int i = 0; i < 6; i++){
        active_requests[i] = NULL;
    }
}

void response_handler_cleanup(){
    g_mutex_lock(&active_requests_mutex);
    for(int i = 0; i < 6; i++){
        if(active_requests[i]){
            g_mutex_clear(&active_requests[i]->mutex);
            g_cond_clear(&active_requests[i]->cond);
            g_free(active_requests[i]);
            active_requests[i] = NULL;
        }
    }
    g_mutex_unlock(&active_requests_mutex);
    g_mutex_clear(&active_requests_mutex);
}

void register_response_handler(ResponseType type, ResponseCallback callback){
    if(type < 0 || type > 5){
        return;
    }

    response_callbacks[type] = callback;
}

static void cleanup_response_data(ResponseData *data) {
    if(!data){
        return;
    }
    
    ResponseType type = data->type;
    
    g_mutex_lock(&active_requests_mutex);
    
    // Clear from active requests array
    if(active_requests[type] == data){
        active_requests[type] = NULL;
    }
    g_mutex_unlock(&active_requests_mutex);
    
    g_mutex_clear(&data->mutex);
    g_cond_clear(&data->cond);
    g_free(data);
}

static gboolean show_response_dialog(gpointer user_data){
    ResponseData *response_data = (ResponseData *) user_data;

    if(response_callbacks[response_data->type] == NULL){
        cleanup_response_data(response_data);
        g_print("Invalid reponse type (response_callback is NULL)!\n");
    }

    response_callbacks[response_data->type](response_data);

    return G_SOURCE_REMOVE;
}

static gpointer response_wait_thread(gpointer user_data){
    ResponseData *response_data = (ResponseData *) user_data;
    gint64 end_time;
    
    g_mutex_lock(&response_data->mutex);

    end_time = g_get_monotonic_time() + (RESPONSE_TTL + 1) * G_TIME_SPAN_SECOND;

    // Wait for signal with timeout
    while(!response_data->response_received && !response_data->timeout_occurred) {
        if(!g_cond_wait_until(&response_data->cond, &response_data->mutex, end_time)) {
            // Timeout occurred
            if(!response_data->response_received) {
                response_data->timeout_occurred = TRUE;
                g_print("Requset timed out for type %d!\n", response_data->type);
            }
            break;
        }
    }

    g_mutex_unlock(&response_data->mutex);

    g_idle_add(show_response_dialog, response_data);

    return NULL;
}

gboolean start_request(ResponseType type){
    g_mutex_lock(&active_requests_mutex);

    if(active_requests[type] != NULL){
        g_mutex_unlock(&active_requests_mutex);
        g_print("Request already pending for type %d\n", type);
        return FALSE;
    }

    ResponseData *response_data = g_new0(ResponseData, 1);

    g_mutex_init(&response_data->mutex);
    g_cond_init(&response_data->cond);
    response_data->response_received = FALSE;
    response_data->timeout_occurred = FALSE;
    response_data->success = FALSE;
    response_data->type = type;

    active_requests[type] = response_data;
    g_mutex_unlock(&active_requests_mutex);

    g_print("Initializing request (type: %d)...\n", type);

    g_thread_new("wait_thread", response_wait_thread, response_data);

    return TRUE;
}

// Signal response from server
void signal_response(ResponseType type, gboolean success){
    g_mutex_lock(&active_requests_mutex);
    
    ResponseData *response_data = active_requests[type];
    if (!response_data) {
        g_mutex_unlock(&active_requests_mutex);
        g_print("Warning: No pending request found for type %d\n", type);
        return;
    }
    
    g_mutex_unlock(&active_requests_mutex);

    // Critical section
    g_mutex_lock(&response_data->mutex);
    response_data->response_received = TRUE;
    response_data->success = success;
    g_cond_signal(&response_data->cond);
    g_mutex_unlock(&response_data->mutex);

    g_print("Signaled response for type %d: %s\n", type, success ? "success" : "failure");
}

gboolean is_request_pending(ResponseType type) {
    gboolean pending = FALSE;
    
    g_mutex_lock(&active_requests_mutex);
    if(active_requests[type] != NULL){
        pending = TRUE;
    }
    g_mutex_unlock(&active_requests_mutex);
    
    return pending;
}

void cancel_request(ResponseType type) {
    g_mutex_lock(&active_requests_mutex);
    
    ResponseData *response_data = active_requests[type];
    if(response_data){
        g_mutex_lock(&response_data->mutex);
        response_data->timeout_occurred = TRUE;  // Mark as timeouted
        g_cond_signal(&response_data->cond);
        g_mutex_unlock(&response_data->mutex);
    }
    
    g_mutex_unlock(&active_requests_mutex);
}

// Login dialog callback
void show_login_dialog(ResponseData *response_data){
    GtkWidget *dialog;

    if(response_data->timeout_occurred) {
        dialog = gtk_message_dialog_new(NULL,
                                                   GTK_DIALOG_MODAL,
                                                   GTK_MESSAGE_ERROR,
                                                   GTK_BUTTONS_OK,
                                                   "Login timeout! Server did not respond.");
    } 
    else if(response_data->response_received && response_data->success) {
        GtkWidget *dialog = gtk_message_dialog_new(NULL,
                                                   GTK_DIALOG_MODAL,
                                                   GTK_MESSAGE_INFO,
                                                   GTK_BUTTONS_OK,
                                                   "Login Successful!");
    } 
    else{
        GtkWidget *dialog = gtk_message_dialog_new(NULL,
                                                   GTK_DIALOG_MODAL,
                                                   GTK_MESSAGE_ERROR,
                                                   GTK_BUTTONS_OK,
                                                   "Invalid username or password!");
    }
    
    gtk_window_present(GTK_WINDOW(dialog));
    g_signal_connect_swapped(dialog, "response", G_CALLBACK(gtk_window_destroy), dialog);
    cleanup_response_data(response_data);
}

void show_account_creation_dialog(ResponseData *data){
    GtkWidget *dialog;

    if(data->timeout_occurred){
        dialog = gtk_message_dialog_new(NULL,
                                       GTK_DIALOG_MODAL,
                                       GTK_MESSAGE_ERROR,
                                       GTK_BUTTONS_OK,
                                       "Account creation timeout!");
    }
    else if(data->response_received && data->success){
        dialog = gtk_message_dialog_new(NULL,
                                       GTK_DIALOG_MODAL,
                                       GTK_MESSAGE_INFO,
                                       GTK_BUTTONS_OK,
                                       "Account created successfully!");
    }
    else{
        dialog = gtk_message_dialog_new(NULL,
                                       GTK_DIALOG_MODAL,
                                       GTK_MESSAGE_ERROR,
                                       GTK_BUTTONS_OK,
                                       "Account creation failed!");
    }
 
    gtk_window_present(GTK_WINDOW(dialog));
    g_signal_connect_swapped(dialog, "response", G_CALLBACK(gtk_window_destroy), dialog);
    cleanup_response_data(data);
}

void show_send_message_dialog(ResponseData *data){
    // Failed to send message
    if(data->timeout_occurred || !data->success){
        GtkWidget *dialog = gtk_message_dialog_new(NULL,
                                                   GTK_DIALOG_MODAL,
                                                   GTK_MESSAGE_ERROR,
                                                   GTK_BUTTONS_OK,
                                                   "Failed to send message!");
        gtk_window_present(GTK_WINDOW(dialog));
        g_signal_connect_swapped(dialog, "response", G_CALLBACK(gtk_window_destroy), dialog);
    }

    cleanup_response_data(data);
}

void show_logout_dialog(ResponseData *data){
    if(data->timeout_occurred){
        GtkWidget *dialog = gtk_message_dialog_new(NULL,
                                                   GTK_DIALOG_MODAL,
                                                   GTK_MESSAGE_ERROR,
                                                   GTK_BUTTONS_OK,
                                                   "Server timeout!");
        gtk_window_present(GTK_WINDOW(dialog));
        //g_signal_connect_swapped(dialog, "response", G_CALLBACK(gtk_window_destroy), dialog);
    }
    else if(data->response_received && data->success){
        GtkWidget *dialog = gtk_message_dialog_new(NULL,
                                                   GTK_DIALOG_MODAL,
                                                   GTK_MESSAGE_INFO,
                                                   GTK_BUTTONS_OK,
                                                   "You've successfully logged in!");
        gtk_window_present(GTK_WINDOW(dialog));
        //g_signal_connect_swapped(dialog, "response", G_CALLBACK(gtk_window_destroy), dialog);

        g_print("Logged out successfully\n");

        // [Switch windows]
    }
    else{
        GtkWidget *dialog = gtk_message_dialog_new(NULL,
                                                   GTK_DIALOG_MODAL,
                                                   GTK_MESSAGE_ERROR,
                                                   GTK_BUTTONS_OK,
                                                   "You failed to log in!");
        gtk_window_present(GTK_WINDOW(dialog));
        //g_signal_connect_swapped(dialog, "response", G_CALLBACK(gtk_window_destroy), dialog);
    }

    cleanup_response_data(data);
}

void show_friend_add_dialog(ResponseData *data){
    GtkWidget *dialog;
    
    if(data->timeout_occurred) {
        dialog = gtk_message_dialog_new(NULL,
                                       GTK_DIALOG_MODAL,
                                       GTK_MESSAGE_ERROR,
                                       GTK_BUTTONS_OK,
                                       "Friend add request timeout!");
    } 
    else if(data->response_received && data->success) {
        dialog = gtk_message_dialog_new(NULL,
                                       GTK_DIALOG_MODAL,
                                       GTK_MESSAGE_INFO,
                                       GTK_BUTTONS_OK,
                                       "Friend request sent!");
    } 
    else{
        dialog = gtk_message_dialog_new(NULL,
                                       GTK_DIALOG_MODAL,
                                       GTK_MESSAGE_ERROR,
                                       GTK_BUTTONS_OK,
                                       "Failed to add friend!");
    }
    
    gtk_window_present(GTK_WINDOW(dialog));
    g_signal_connect_swapped(dialog, "response", G_CALLBACK(gtk_window_destroy), dialog);
    cleanup_response_data(data);
}

void show_chat_create_dialog(ResponseData *data){
    GtkWidget *dialog;
    
    if(data->timeout_occurred) {
        dialog = gtk_message_dialog_new(NULL,
                                       GTK_DIALOG_MODAL,
                                       GTK_MESSAGE_ERROR,
                                       GTK_BUTTONS_OK,
                                       "Chat creation request timeout!");
    } 
    else if(data->response_received && data->success) {
        dialog = gtk_message_dialog_new(NULL,
                                       GTK_DIALOG_MODAL,
                                       GTK_MESSAGE_INFO,
                                       GTK_BUTTONS_OK,
                                       "Chat has been created!");
    } 
    else{
        dialog = gtk_message_dialog_new(NULL,
                                       GTK_DIALOG_MODAL,
                                       GTK_MESSAGE_ERROR,
                                       GTK_BUTTONS_OK,
                                       "Failed to create chat!");
    }
    
    gtk_window_present(GTK_WINDOW(dialog));
    g_signal_connect_swapped(dialog, "response", G_CALLBACK(gtk_window_destroy), dialog);
    cleanup_response_data(data);
}

void init_response_handlers(){
    response_handler_init();

    register_response_handler(RESPONSE_LOGIN, show_login_dialog);
    register_response_handler(RESPONSE_REGISTER, show_account_creation_dialog);
    register_response_handler(RESPONSE_SEND_MESSAGE, show_send_message_dialog);
    register_response_handler(RESPONSE_LOGOUT, show_logout_dialog);
    register_response_handler(RESPONSE_ADD_FRIEND, show_friend_add_dialog);
    register_response_handler(RESPONSE_CREATE_CHAT, show_chat_create_dialog);
}