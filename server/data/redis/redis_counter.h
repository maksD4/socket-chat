#ifndef REDIS_COUNTER_H
#define REDIS_COUNTER_H

// Counter methods for every packet besides room
int redis_counters_set(char* packet_id, char* session);
int redis_counters_get(char* packet_id, char* session);
int redis_counters_del(char* packet_id, char* session);
int redis_counters_increment(char* packet_id, char* session);

// Counter id's room methods
int reids_counter_room_ids_del(char* session);
int redis_counter_room_id_set(char* session, int id);
int redis_counter_room_ids_set(char* session, int amount, int* ids);
int redis_counter_room_ids_exist(char* session);
int redis_counter_room_ids_next(char* session, int old_id);

// Counter methods for room packet
int redis_counter_room_set(char* session, int id);
int redis_counter_room_get(char* session, int id);
int redis_counter_room_increment(char* session, int id);

#endif