#include "websocket_client.h"
#include <stdio.h>

void on_data_received(const char *data) {
    printf("Received data: %s\n", data);
}

int main() {
    if (ws_init_client("localhost", 9014) != 0) {
        printf("Failed to initialize websocket client.\n");
        return -1;
    }

    ws_set_callback(on_data_received);

    char message[256];
    while (1) {
        printf("Enter message to send (or 'exit' to quit): ");
        fgets(message, sizeof(message), stdin);
        if (strncmp(message, "exit", 4) == 0) {
            break;
        }
        ws_send(message);
    }

    ws_cleanup();
    return 0;
}
