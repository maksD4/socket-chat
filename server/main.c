#include <stdio.h>
#include "server/modules/server.h"

//gcc server/main.c server/data/*.c server/modules/*.c -I. -L. -lhiredis -Wl,-rpath='$ORIGIN' -o a.out $(pkg-config --cflags --libs libmongoc-1.0 libbson-1.0)

// server: make, make clean
// temp: gcc temp.c -o temp.out
// client: gcc client/main.c client/modules/*.c -o client.out
// temp_server: gcc client/temp_server.c -o server.out

// git clone https://github.com/maksD4/socket-chat
// sudo zypper install docker
// sudo systemctl enable docker
// sudo usermod -G docker -a $USER
// sudo systemctl restart docker
// sudo docker run --name mongodb -d mongo:latest
// cp ./lib/*.pc /usr/lib/pkgconfig
// export PKG_CONFIG_PATH=/usr/lib/pkgconfig:$PKG_CONFIG_PATH


int main(){
    start_server();
}