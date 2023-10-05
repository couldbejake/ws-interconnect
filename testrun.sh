gcc -o ws_client websocket_client.c main.c -lwebsockets -lpthread &&
./ws_client;
