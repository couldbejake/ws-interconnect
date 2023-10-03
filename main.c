#include "ws_client.h"
#include <stdio.h>
#include <unistd.h>

void data_received(const char *data) {
    printf("Data received: %s\n", data);
}

int main(void) {
    WebSocketClient *client = initialize_ws("localhost", 8083, data_received);
    sleep(5);
    send_message(client, "Hello, World!");
    sleep(5);
    free(client);
    return 0;
}