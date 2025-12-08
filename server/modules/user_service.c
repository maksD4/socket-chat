#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "server/data/mongodb/mongodb_client.h"
#include "server/data/mongodb/mongodb_user.h"
#include "server/data/redis/redis_user.h"
#include "server/utils/models/models_print.h"
#include "lib/constants.h"


int account_creation(char* name, char* password){
    char fixed_name[NAME_MAX_SIZE + 1] = {0};
    snprintf(fixed_name, NAME_MAX_SIZE, name);

    if(get_user_id(fixed_name) > 0){
        printf("[%d][SERVICE] >> USER WITH %s NAME ALREADY EXIST!\n", getpid(), fixed_name);
        return -1;
    }

    char fixed_password[PASSWORD_MAX_SIZE + 1] = {0};
    snprintf(fixed_password, PASSWORD_MAX_SIZE, password);


    int id = get_next_id("USER");
    if(id == -1){
        printf("[%d][SERVICE] >> NEXT ID IS EQUAL -1!\n", getpid());
        return -1;
    }

    user_t user = create_user(id, fixed_name, fixed_password, NULL, 0, NULL, 0);
    
    if(mongodb_user_write(user)){
        printf("[%d][SERVICE] >> DB USER WRITE FAILED!\n", getpid());
        return -1;
    }

    free(user.name);
    free(user.password);
    return 0;
}

int auth(char* name, char* password){
    char fixed_name[NAME_MAX_SIZE + 1], fixed_password[PASSWORD_MAX_SIZE + 1];
    snprintf(fixed_name, NAME_MAX_SIZE, name);
    snprintf(fixed_password, PASSWORD_MAX_SIZE, password);

    // Check if user exist
    if(get_user_id(fixed_name) < 1){
        return -1;
    }
    user_t user;

    if(mongodb_user_read(fixed_name, &user)){
        printf("[%d][AUTH] >> DB USER READ FAIL!\n", getpid());
        return -1;
    }

    if(!strcmp(fixed_password, user.password)){
        // create session

        return 0;
    }

    return -1;
}
