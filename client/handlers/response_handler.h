#ifndef RESPONSE_HANDLER_H
#define RESPONSE_HADLER_H
#include <gtk-4.0/gtk/gtk.h>

// Response types
typedef enum {
    RESPONSE_LOGIN,
    RESPONSE_REGISTER,
    RESPONSE_LOGOUT,
    RESPONSE_SEND_MESSAGE,
    RESPONSE_ADD_FRIEND,
    RESPONSE_CREATE_CHAT
} ResponseType;

// Generic response data structure
typedef struct {
    GMutex mutex;
    GCond cond;
    gboolean response_received;
    gboolean timeout_occurred;
    gboolean success;
    ResponseType type;
    gpointer custom_data;  // For type-specific data
} ResponseData;

// Callback function type for showing dialog
typedef void (*ResponseCallback)(ResponseData *data);

// Register response handling
void register_response_handler(ResponseType type, ResponseCallback callback);

// Initiation of request 
gboolean start_request(ResponseType type);

// Signal response
void signal_response(ResponseType type, gboolean success);

// Check if request is still being processed
gboolean is_request_pending(ResponseType type);

// Cancel pending request of certain type
void cancel_request(ResponseType type);

// Dialog functions
void show_login_dialog(ResponseData *data);
void show_account_creation_dialog(ResponseData *data);
void show_send_message_dialog(ResponseData *data);
void show_logout_dialog(ResponseData *data);
void show_friend_add_dialog(ResponseData *data);
void show_chat_create_dialog(ResponseData *data);

void init_response_handlers();

#endif
