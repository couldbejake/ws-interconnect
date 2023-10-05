#ifndef WEBSOCKET_CLIENT_H
#define WEBSOCKET_CLIENT_H

#include <libwebsockets.h>

typedef void (*data_received_callback)(const char *data);

// Initialize the web socket client
int ws_init_client(const char *server_address, int port);

// Send a message over the web socket
int ws_send(const char *message);

// Set the callback function to be called when new data is received
void ws_set_callback(data_received_callback callback);

// Cleanup and close the websocket client
void ws_cleanup();

#endif // WEBSOCKET_CLIENT_H
