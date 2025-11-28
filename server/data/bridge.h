#ifndef BRIDGE_H
#define BRIDGE_H

int mongodb_to_redis(); // Load every user data from db to redis (e.g. when user log in)
int redis_to_mongodb(); // Load every user data from redis to db (e.g. when user log out)

int sync_mongodb(); // Synchronizing every data inside redis with db
//int sync_redis() - usage?

#endif