#include <stdio.h>
#include "server/headers/modules/server.h"

//gcc server/main.c server/data/*.c server/modules/*.c -I. -L. -lhiredis -Wl,-rpath='$ORIGIN' -o a.out $(pkg-config --cflags --libs libmongoc-1.0 libbson-1.0)
int main(){
    start_server();
}