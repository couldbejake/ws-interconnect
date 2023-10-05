#ifndef WEBSOCKET_CLIENT_H
#define WEBSOCKET_CLIENT_H

#include <libwebsockets.h>

typedef void (*data_received_callback)(const char *data);

// Initialize the web socket client
int initialize_websocket_client(const char *server_address, int port);

// Send a message over the web socket
int send_message(const char *message);

// Set the callback function to be called when new data is received
void set_data_received_callback(data_received_callback callback);

// Cleanup and close the websocket client
void cleanup_websocket_client();

#endif // WEBSOCKET_CLIENT_H
