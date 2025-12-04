#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Account creation packet
// accc;<name>;<password>
char* get_packet_accc(char *name, char *password){
    size_t packet_size = strlen(name) + strlen(password) + 7;

    char *packet = malloc(packet_size);
    if(!packet){
        return NULL;
    }

    snprintf(packet, packet_size, "accc;%s;%s", name, password);
    packet[packet_size] = '\0';
    printf("packet: %s\npacket_size: %d\n", packet, packet_size);
    return packet;
}

// Log in packet
// logi;<name>;<password>
char* get_packet_logi(char *name, char *password){
    size_t packet_size = strlen(name) + strlen(password) + 7;

    char *packet = malloc(packet_size);
    if(!packet){
        return NULL;
    }

    snprintf(packet, packet_size, "logi;%s;%s", name, password);
    packet[packet_size] = '\0';
    printf("packet: %s\npacket_size: %d\n", packet, packet_size);
    return packet;
}

