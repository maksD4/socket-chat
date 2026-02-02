#include <stdio.h>
#include <gtk-4.0/gtk/gtk.h>
#include <time.h>

#include "lib/constants.h"
#include "client/gui/app.h"
#include "client/gui/login.h"
#include "client/modules/app/user_app.h"
#include "client/handlers/response_handler.h"
#include "client/modules/client.h"
#include "client/modules/packets.h"
#include "client/main.h"

// Global widgets for chat area updates
static GtkWidget *chat_container = NULL;
static GtkWidget *chat_name_label = NULL;
static GtkWidget *chat_scroll = NULL;
static GtkWidget *chat_messages_box = NULL;
static GtkWidget *message_entry = NULL;
static GtkWidget *friends_list_box = NULL;
static GtkWidget *chats_list_box = NULL;
static int current_room_index = -1;

typedef struct {
    GtkWidget *window;
    int room_index;
} ChatData;

// Function to display chat with a specific room
static void show_chat_with_room(int room_index) {
    if (room_index < 0 || room_index >= user_data.room_amount) {
        g_print("Invalid room index\n");
        return;
    }
    
    current_room_index = room_index;
    
    // Build chat name (all users except current user)
    char chat_name[512] = "";
    int first = 1;
    
    for (int j = 0; j < user_data.rooms[room_index].user_amount; j++) {
        if (strcmp(user_data.rooms[room_index].users[j], user_data.username) != 0) {
            if (!first) {
                strcat(chat_name, ", ");
            }
            strcat(chat_name, user_data.rooms[room_index].users[j]);
            first = 0;
        }
    }
    
    g_print("Opening chat: %s\n", chat_name);
    
    // Update chat header with chat name
    gtk_label_set_text(GTK_LABEL(chat_name_label), chat_name);
    
    // Clear existing messages
    GtkWidget *child = gtk_widget_get_first_child(chat_messages_box);
    while (child != NULL) {
        GtkWidget *next = gtk_widget_get_next_sibling(child);
        gtk_box_remove(GTK_BOX(chat_messages_box), child);
        child = next;
    }
    
    // Display messages
    if (user_data.rooms[room_index].msg_amount > 0) {
        for (int i = 0; i < user_data.rooms[room_index].msg_amount; i++) {
            msg_data_t *msg = &user_data.rooms[room_index].messages[i];
            
            GtkWidget *msg_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
            gtk_widget_set_margin_start(msg_box, 10);
            gtk_widget_set_margin_end(msg_box, 10);
            gtk_widget_set_margin_top(msg_box, 5);
            gtk_widget_set_margin_bottom(msg_box, 5);
            
            // Check if message is from current user
            gboolean is_own = (strcmp(msg->sent_by, user_data.username) == 0);
            
            if (is_own) {
                gtk_widget_set_halign(msg_box, GTK_ALIGN_END);
            } else {
                gtk_widget_set_halign(msg_box, GTK_ALIGN_START);
            }
            
            GtkWidget *msg_frame = gtk_frame_new(NULL);
            GtkWidget *msg_content_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
            gtk_widget_set_margin_start(msg_content_box, 8);
            gtk_widget_set_margin_end(msg_content_box, 8);
            gtk_widget_set_margin_top(msg_content_box, 5);
            gtk_widget_set_margin_bottom(msg_content_box, 5);
            
            GtkWidget *sender_label = gtk_label_new(msg->sent_by);
            gtk_widget_add_css_class(sender_label, "dim-label");
            gtk_widget_set_halign(sender_label, GTK_ALIGN_START);
            
            GtkWidget *text_label = gtk_label_new(msg->message);
            gtk_label_set_wrap(GTK_LABEL(text_label), TRUE);
            gtk_label_set_xalign(GTK_LABEL(text_label), 0.0);
            gtk_widget_set_halign(text_label, GTK_ALIGN_START);
            
            gtk_box_append(GTK_BOX(msg_content_box), sender_label);
            gtk_box_append(GTK_BOX(msg_content_box), text_label);
            gtk_frame_set_child(GTK_FRAME(msg_frame), msg_content_box);
            gtk_box_append(GTK_BOX(msg_box), msg_frame);
            gtk_box_append(GTK_BOX(chat_messages_box), msg_box);
        }
    } else {
        GtkWidget *empty_label = gtk_label_new("No messages yet. Start the conversation!");
        gtk_widget_set_margin_top(empty_label, 20);
        gtk_widget_add_css_class(empty_label, "dim-label");
        gtk_box_append(GTK_BOX(chat_messages_box), empty_label);
    }
}

// Public function to refresh current chat (can be called from outside)
void refresh_current_chat(void){
    if(current_room_index >= 0){
        show_chat_with_room(current_room_index);
    }
}

void clear_current_message(void){
    gtk_editable_set_text(GTK_EDITABLE(message_entry), "");
}

void refresh_friends_list(void){
    if(!friends_list_box){
        g_print("Friends list box not initialized\n");
        return;
    }
    
    // Clear existing friends
    GtkWidget *child = gtk_widget_get_first_child(friends_list_box);
    while(child != NULL){
        GtkWidget *next = gtk_widget_get_next_sibling(child);
        gtk_box_remove(GTK_BOX(friends_list_box), child);
        child = next;
    }
    
    // Add updated friend names
    for(int i = 0; i < user_data.friend_amount; i++){
        GtkWidget *friend_label = gtk_label_new(user_data.friends[i]);
        gtk_widget_set_halign(friend_label, GTK_ALIGN_START);
        gtk_widget_set_margin_top(friend_label, 3);
        gtk_widget_set_margin_bottom(friend_label, 3);
        gtk_box_append(GTK_BOX(friends_list_box), friend_label);
    }
    
    g_print("Friends list refreshed (%d friends)\n", user_data.friend_amount);
}

static void on_room_clicked(GtkButton *button, gpointer user_data) {
    ChatData *data = (ChatData *)user_data;
    show_chat_with_room(data->room_index);
}

void refresh_chats_list(void){
    if(!chats_list_box){
        g_print("Chats list box not initialized\n");
        return;
    }

    // Clear existing chats
    GtkWidget *child = gtk_widget_get_first_child(chats_list_box);
    while (child != NULL) {
        GtkWidget *next = gtk_widget_get_next_sibling(child);
        gtk_box_remove(GTK_BOX(chats_list_box), child);
        child = next;
    }

    // Free old chat_data_array and reallocate
    static ChatData *chat_data_array = NULL;
    if (chat_data_array) {
        g_free(chat_data_array);
    }
    chat_data_array = g_malloc(sizeof(ChatData) * user_data.room_amount);

    // Rebuild chat buttons
    for (int i = 0; i < user_data.room_amount; i++) {
        char chat_name[512] = "";
        int first = 1;

        for (int j = 0; j < user_data.rooms[i].user_amount; j++) {
            if (strcmp(user_data.rooms[i].users[j], user_data.username) != 0) {
                if (!first) {
                    strcat(chat_name, ", ");
                }
                strcat(chat_name, user_data.rooms[i].users[j]);
                first = 0;
            }
        }

        GtkWidget *chat_btn = gtk_button_new_with_label(chat_name);
        gtk_widget_set_size_request(chat_btn, -1, 35);

        chat_data_array[i].window = NULL;
        chat_data_array[i].room_index = i;

        g_signal_connect(chat_btn, "clicked", G_CALLBACK(on_room_clicked), &chat_data_array[i]);
        gtk_box_append(GTK_BOX(chats_list_box), chat_btn);
    }

    g_print("Chats list refreshed (%d chats)\n", user_data.room_amount);
}

static void on_add_friend_accept_clicked(GtkButton *button, gpointer data){
    if(is_request_pending(RESPONSE_ADD_FRIEND)){
        g_print("Friend add request is pending...\n");
        return;
    }

    GtkWidget **entries = (GtkWidget **)data;
    GtkWidget *entry = entries[0];
    GtkWidget *dialog = entries[1];

    const char *new_friend = gtk_editable_get_text(GTK_EDITABLE(entry));
    if (strlen(new_friend) >= NAME_MIN_SIZE && strlen(new_friend) <= NAME_MAX_SIZE) {
        g_print("Adding friend: %s\n", new_friend);

        if(start_request(RESPONSE_ADD_FRIEND)){
            g_print("Friend add request started\n");

            char* packet = get_friend_add_packet(user_data.session_key, new_friend);
            if(!send_to_server(packet)){
                printf("Successfully send friend's packet\n");
            }
            else{
                printf("Sending friend packet failed!\n");
            }
            free(packet);
        } 
        else{
            g_print("Friend add request already in progress\n");
        }
    }
    else{
        GtkWidget *dialog_err = gtk_message_dialog_new(NULL,
                                                   GTK_DIALOG_MODAL,
                                                   GTK_MESSAGE_ERROR,
                                                   GTK_BUTTONS_OK,
                                                   "Invalid username!");
        gtk_window_present(GTK_WINDOW(dialog_err));
        g_signal_connect_swapped(dialog_err, "response", G_CALLBACK(gtk_window_destroy), dialog_err);
    }
    //gtk_window_destroy(GTK_WINDOW(dialog));
}

static void on_add_friend_clicked(GtkButton *button, gpointer user_data){
    GtkWidget *dialog = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "Add Friend");
    gtk_window_set_default_size(GTK_WINDOW(dialog), 300, 150);
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_start(box, 20);
    gtk_widget_set_margin_end(box, 20);
    gtk_widget_set_margin_top(box, 20);
    gtk_widget_set_margin_bottom(box, 20);
    
    GtkWidget *label = gtk_label_new("Enter friend's username:");
    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Username");
    
    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_halign(button_box, GTK_ALIGN_CENTER);
    
    GtkWidget *add_btn = gtk_button_new_with_label("Add");
    GtkWidget *cancel_btn = gtk_button_new_with_label("Cancel");
    
    gtk_box_append(GTK_BOX(button_box), add_btn);
    gtk_box_append(GTK_BOX(button_box), cancel_btn);
    
    gtk_box_append(GTK_BOX(box), label);
    gtk_box_append(GTK_BOX(box), entry);
    gtk_box_append(GTK_BOX(box), button_box);
    
    gtk_box_append(GTK_BOX(content_area), box);
    
    static GtkWidget *entries[2];
    entries[0] = entry;
    entries[1] = dialog;

    // Connect add button to get the username
    g_signal_connect(add_btn, "clicked", G_CALLBACK(on_add_friend_accept_clicked), entries);
    
    g_signal_connect_swapped(cancel_btn, "clicked", G_CALLBACK(gtk_window_destroy), dialog);
    
    gtk_window_present(GTK_WINDOW(dialog));
}

static void on_logout_clicked(GtkButton *button, gpointer user_data) {
    if(is_request_pending(RESPONSE_LOGOUT)){
        g_print("Logout request is pending...\n");
        return;
    }

    if(start_request(RESPONSE_LOGOUT)){
        g_print("Logout request started!\n");
        usleep(500000);
        signal_response(RESPONSE_LOGOUT, TRUE);

        //printf("asd1\n");
        // Disconnect from server
        //disconnect_any_server(); // doesnt work properly
        //printf("asd2\n");
        // Clear old user data
        //free_user_data();
        //printf("asd3\n");
        // Switch to login window
        //switch_to_login_window();

        g_print("Logged out successfully\n");
    }
    else{
        g_print("Logout request is already in progress...\n");
    }
}

int get_current_chat_id(void) {
    if (current_room_index == -1) {
        g_print("No chat is currently open\n");
        return -1;
    }
    
    if (current_room_index >= user_data.room_amount) {
        g_print("Invalid room index\n");
        return -1;
    }
    
    return user_data.rooms[current_room_index].id;
}

static void on_send_clicked(GtkButton *button, gpointer data) {
    int chat_id = get_current_chat_id();
    if(chat_id == -1){
        return;
    }

    const char *message = gtk_editable_get_text(GTK_EDITABLE(message_entry));
    
    // Do nothing if message is empty
    if (!message || strlen(message) == 0) {
        return;
    }

    if(is_request_pending(RESPONSE_SEND_MESSAGE)){
        g_print("Message request is pending...\n");
        return;
    }
    
    if(start_request(RESPONSE_SEND_MESSAGE)){
        g_print("Message request created!\n");

        char* packet = get_message_packet(user_data.session_key, chat_id, message);

        if(send_to_server(packet) == 0){
            printf("Message sent successfully\n");
        } else {
            printf("Failed to send message\n");
        }
        free(packet);

        //gtk_editable_set_text(GTK_EDITABLE(message_entry), "");
    }
    else{
        g_print("Message request is already in progress...\n");
    }

    // Here you would typically:
    // - Send message to server
    // - Add message to current room's messages
    // - Refresh the chat display
    // - Clear the input field
    
    // Example: refresh chat after sending
    // refresh_current_chat();
}

void show_chat_window(GtkApplication *app, GtkWidget *window) {
    GtkWidget *child = gtk_window_get_child(GTK_WINDOW(window));
    if (child) {
        gtk_window_set_child(GTK_WINDOW(window), NULL);
    }
    
    gtk_window_set_title(GTK_WINDOW(window), "Chat Application");
    gtk_window_set_default_size(GTK_WINDOW(window), 900, 600);
    
    // Main container
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    
    // ========== LEFT SIDEBAR - FRIENDS LIST ==========
    GtkWidget *friends_sidebar = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_size_request(friends_sidebar, 120, -1);
    gtk_widget_set_hexpand(friends_sidebar, FALSE);
    
    // Friends header
    GtkWidget *friends_header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_margin_start(friends_header, 10);
    gtk_widget_set_margin_end(friends_header, 10);
    gtk_widget_set_margin_top(friends_header, 10);
    gtk_widget_set_margin_bottom(friends_header, 10);
    
    GtkWidget *friends_title = gtk_label_new("Friends");
    gtk_widget_add_css_class(friends_title, "title-2");
    GtkWidget *add_friend_btn = gtk_button_new_with_label("+");
    gtk_widget_set_size_request(add_friend_btn, 30, 30);
    
    gtk_box_append(GTK_BOX(friends_header), friends_title);
    gtk_box_append(GTK_BOX(friends_header), add_friend_btn);
    gtk_widget_set_hexpand(friends_title, TRUE);
    gtk_widget_set_halign(friends_title, GTK_ALIGN_START);
    
    g_signal_connect(add_friend_btn, "clicked", G_CALLBACK(on_add_friend_clicked), NULL);
    
    // Scrollable friends list (text only)
    GtkWidget *friends_scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(friends_scroll),
                                   GTK_POLICY_NEVER,
                                   GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(friends_scroll, TRUE);
    
    friends_list_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_widget_set_margin_start(friends_list_box, 10);
    gtk_widget_set_margin_end(friends_list_box, 10);
    
    // Add friend names as text labels
    for (int i = 0; i < user_data.friend_amount; i++) {
        GtkWidget *friend_label = gtk_label_new(user_data.friends[i]);
        gtk_widget_set_halign(friend_label, GTK_ALIGN_START);
        gtk_widget_set_margin_top(friend_label, 3);
        gtk_widget_set_margin_bottom(friend_label, 3);
        gtk_box_append(GTK_BOX(friends_list_box), friend_label);
    }
    
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(friends_scroll), friends_list_box);
    
    // Username label at bottom
    GtkWidget *username_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_widget_set_margin_start(username_box, 10);
    gtk_widget_set_margin_end(username_box, 10);
    gtk_widget_set_margin_top(username_box, 5);
    gtk_widget_set_margin_bottom(username_box, 10);
    
    GtkWidget *username_label = gtk_label_new(user_data.username);
    gtk_widget_add_css_class(username_label, "dim-label");
    gtk_widget_set_halign(username_label, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(username_box), username_label);
    
    gtk_box_append(GTK_BOX(friends_sidebar), friends_header);
    gtk_box_append(GTK_BOX(friends_sidebar), friends_scroll);
    gtk_box_append(GTK_BOX(friends_sidebar), username_box);
    
    // First separator
    GtkWidget *separator1 = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
    
    // ========== MIDDLE SIDEBAR - CHATS LIST ==========
    GtkWidget *chats_sidebar = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_size_request(chats_sidebar, 180, -1);
    gtk_widget_set_hexpand(chats_sidebar, FALSE);
    
    // Chats header
    GtkWidget *chats_header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_margin_start(chats_header, 10);
    gtk_widget_set_margin_end(chats_header, 10);
    gtk_widget_set_margin_top(chats_header, 10);
    gtk_widget_set_margin_bottom(chats_header, 10);
    
    GtkWidget *chats_title = gtk_label_new("Chats");
    gtk_widget_add_css_class(chats_title, "title-2");
    GtkWidget *add_chat_btn = gtk_button_new_with_label("+");
    gtk_widget_set_size_request(add_chat_btn, 30, 30);

    gtk_box_append(GTK_BOX(chats_header), chats_title);
    gtk_box_append(GTK_BOX(chats_header), add_chat_btn);
    gtk_widget_set_hexpand(chats_title, TRUE);
    gtk_widget_set_halign(chats_title, GTK_ALIGN_START);
    
    g_signal_connect(add_chat_btn, "clicked", G_CALLBACK(on_add_friend_clicked), NULL);
    
    // Scrollable chats list (buttons)
    GtkWidget *chats_scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(chats_scroll),
                                   GTK_POLICY_NEVER,
                                   GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(chats_scroll, TRUE);
    
    chats_list_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_widget_set_margin_start(chats_list_box, 5);
    gtk_widget_set_margin_end(chats_list_box, 5);
    
    // Add chat buttons (showing users without current username)
    static ChatData *chat_data_array = NULL;
    chat_data_array = g_malloc(sizeof(ChatData) * user_data.room_amount);
    
    for (int i = 0; i < user_data.room_amount; i++) {
        // Build chat name (all users except current user)
        char chat_name[512] = "";
        int first = 1;
        
        for (int j = 0; j < user_data.rooms[i].user_amount; j++) {
            if (strcmp(user_data.rooms[i].users[j], user_data.username) != 0) {
                if (!first) {
                    strcat(chat_name, ", ");
                }
                strcat(chat_name, user_data.rooms[i].users[j]);
                first = 0;
            }
        }
        
        GtkWidget *chat_btn = gtk_button_new_with_label(chat_name);
        gtk_widget_set_size_request(chat_btn, -1, 35);
        
        chat_data_array[i].window = window;
        chat_data_array[i].room_index = i;
        
        g_signal_connect(chat_btn, "clicked", G_CALLBACK(on_room_clicked), &chat_data_array[i]);
        gtk_box_append(GTK_BOX(chats_list_box), chat_btn);
    }
    
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(chats_scroll), chats_list_box);
    
    gtk_box_append(GTK_BOX(chats_sidebar), chats_header);
    gtk_box_append(GTK_BOX(chats_sidebar), chats_scroll);
    
    // Second separator
    GtkWidget *separator2 = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
    
    // ========== RIGHT SIDE - CHAT AREA ==========
    chat_container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_size_request(chat_container, 600, -1);
    gtk_widget_set_hexpand(chat_container, TRUE);
    
    // Chat header
    GtkWidget *chat_header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_margin_start(chat_header, 15);
    gtk_widget_set_margin_end(chat_header, 15);
    gtk_widget_set_margin_top(chat_header, 10);
    gtk_widget_set_margin_bottom(chat_header, 10);
    
    chat_name_label = gtk_label_new("Select a chat to start messaging");
    gtk_widget_add_css_class(chat_name_label, "title-2");
    gtk_widget_set_halign(chat_name_label, GTK_ALIGN_START);
    gtk_widget_set_hexpand(chat_name_label, TRUE);
    
    GtkWidget *logout_btn = gtk_button_new_with_label("Logout");
    gtk_widget_add_css_class(logout_btn, "destructive-action");
    g_signal_connect(logout_btn, "clicked", G_CALLBACK(on_logout_clicked), user_data.username);
    
    gtk_box_append(GTK_BOX(chat_header), chat_name_label);
    gtk_box_append(GTK_BOX(chat_header), logout_btn);
    
    // Chat messages area
    chat_scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(chat_scroll),
                                   GTK_POLICY_NEVER,
                                   GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(chat_scroll, TRUE);
    
    chat_messages_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(chat_scroll), chat_messages_box);
    
    // Message input area
    GtkWidget *input_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_margin_start(input_box, 15);
    gtk_widget_set_margin_end(input_box, 15);
    gtk_widget_set_margin_top(input_box, 10);
    gtk_widget_set_margin_bottom(input_box, 10);
    
    message_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(message_entry), "Type a message...");
    gtk_widget_set_hexpand(message_entry, TRUE);
    
    GtkWidget *send_btn = gtk_button_new_with_label("Send");
    gtk_widget_add_css_class(send_btn, "suggested-action");
    g_signal_connect(send_btn, "clicked", G_CALLBACK(on_send_clicked), NULL);
    
    gtk_box_append(GTK_BOX(input_box), message_entry);
    gtk_box_append(GTK_BOX(input_box), send_btn);
    
    gtk_box_append(GTK_BOX(chat_container), chat_header);
    gtk_box_append(GTK_BOX(chat_container), chat_scroll);
    gtk_box_append(GTK_BOX(chat_container), input_box);
    
    // Assemble main layout
    gtk_box_append(GTK_BOX(main_box), friends_sidebar);
    gtk_box_append(GTK_BOX(main_box), separator1);
    gtk_box_append(GTK_BOX(main_box), chats_sidebar);
    gtk_box_append(GTK_BOX(main_box), separator2);
    gtk_box_append(GTK_BOX(main_box), chat_container);
    
    gtk_window_set_child(GTK_WINDOW(window), main_box);
}