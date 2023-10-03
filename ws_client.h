#ifndef WEBSOCKET_CLIENT_H
#define WEBSOCKET_CLIENT_H

#include <libwebsockets.h>

typedef struct {
    char *address;
    int port;
    struct lws *wsi;
    struct lws_client_connect_info connect_info;
    void (*data_received_callback)(const char*);
} WebSocketClient;

WebSocketClient *initialize_ws(const char *address, int port, void (*callback)(const char*));
void send_message(WebSocketClient *client, const char *message);

#endif
