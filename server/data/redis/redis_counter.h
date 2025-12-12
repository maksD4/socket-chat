#ifndef REDIS_COUNTER_H
#define REDIS_COUNTER_H

// Counter methods for every packet besides room
int redis_counters_set(char* packet_id, char* session);
int redis_counters_get(char* packet_id, char* session);
int redis_counters_del(char* packet_id, char* session);
int redis_counters_increment(char* packet_id, char* session);

// Counter methods for room packet
int redis_counters_room_set(char* session, int id);
int redis_counters_room_get(char* session, int id);
int redis_counter_room_increment(char* session, int id);

#endif