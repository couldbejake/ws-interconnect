#include "websocket_client.h"
#include <stdio.h>

void on_data_received(const char *data) {
    printf("Received data: %s\n", data);
}

int main() {
    if (initialize_websocket_client("localhost", 9013) != 0) {
        printf("Failed to initialize websocket client.\n");
        return -1;
    }

    set_data_received_callback(on_data_received);

    char message[256];
    while (1) {
        printf("Enter message to send (or 'exit' to quit): ");
        fgets(message, sizeof(message), stdin);
        if (strncmp(message, "exit", 4) == 0) {
            break;
        }
        send_message(message);
    }

    cleanup_websocket_client();
    return 0;
}
