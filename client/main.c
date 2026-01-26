#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close
#include <pthread.h>
#include "lib/constants.h"
#include "gui/app.h"
#include "gui/login.h"
#include "modules/app/user_app.h"
#include "client/handlers/response_handler.h"

GtkApplication *global_app = NULL;
GtkWidget *global_window = NULL;

static void init_sample_data(void) {
    // Set username and session
    set_username("john_doe");
    set_session("0123456789abcde");
    
    // Set friends
    char* friends[] = {"Alice", "Bob", "Charlie"};
    set_friend_data(3, friends);
    
    // Add room for Alice
    char* room1_users[] = {"john_doe", "Alice"};
    add_room_data(1, 2, 3, room1_users);
    
    // Add messages for Alice's room
    msg_data_t alice_messages[3];
    alice_messages[0] = create_msg_data(1, "Alice", "Hey! How are you?");
    alice_messages[1] = create_msg_data(2, "john_doe", "I'm good! Thanks for asking.");
    alice_messages[2] = create_msg_data(3, "Alice", "Want to grab lunch later?");
    add_messages_data(1, 3, alice_messages);
    
    // Add room for Bob
    char* room2_users[] = {"john_doe", "Bob"};
    add_room_data(2, 2, 2, room2_users);
    
    // Add messages for Bob's room
    msg_data_t bob_messages[2];
    bob_messages[0] = create_msg_data(1, "john_doe", "Did you finish the project?");
    bob_messages[1] = create_msg_data(2, "Bob", "Yes, just submitted it!");
    add_messages_data(2, 2, bob_messages);
    
    // Add room for Charlie (empty)
    char* room3_users[] = {"john_doe", "Charlie", "Alice", "Bob"};
    add_room_data(3, 4, 20, room3_users);
    
    msg_data_t room3_messages[10];
    room3_messages[0] = create_msg_data(1, "john_doe", "Hello");
    room3_messages[1] = create_msg_data(2, "Charlie", "Hello");
    room3_messages[2] = create_msg_data(3, "Alice", "Hello");
    room3_messages[3] = create_msg_data(4, "Bob", "Hello");
    room3_messages[4] = create_msg_data(5, "john_doe", "adsdadadssa");
    room3_messages[5] = create_msg_data(6, "Bob", "p1i23pi12j3msa,mndlajsd");
    room3_messages[6] = create_msg_data(7, "Charlie", "192830912uakjdksajda");
    room3_messages[7] = create_msg_data(8, "Alice", "???");
    room3_messages[8] = create_msg_data(9, "Bob", "?");
    room3_messages[9] = create_msg_data(10, "Charlie", "Hello");
    add_messages_data(3, 10, room3_messages);
    add_messages_data(3, 10, room3_messages);


    // Print data to verify
    print_user_data();
}

static void activate(GtkApplication *app, gpointer user_data_ptr){
    global_window = gtk_application_window_new(app);
    
    show_login_window(app, global_window);
    
    gtk_window_present(GTK_WINDOW(global_window));
}

void switch_to_chat_window(){
    if(global_app && global_window){
        show_chat_window(global_app, global_window);
    }
}

int main(int argc, char **argv){
    init_response_handlers();
    
    /*
    init_sample_data();
    printf("freeing\n");
    free_user_data();
    print_user_data();
    */

    int status;
    
    //init_sample_data();
    
    global_app = gtk_application_new("com.example.chatapp", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(global_app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(global_app), argc, argv);
    g_object_unref(global_app);
    
    return status;
    //run_app(argc, argv);
    //return 0;
}