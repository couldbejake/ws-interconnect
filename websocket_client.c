#include "websocket_client.h"
#include <string.h>
#include <pthread.h>

static struct lws_context *context = NULL;
static struct lws *wsi = NULL;
static data_received_callback received_callback = NULL;
static char message_buffer[256] = {0}; // Buffer to store message to be sent

static int callback_websockets(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len) {
    switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            lws_callback_on_writable(wsi);
            break;
        case LWS_CALLBACK_CLIENT_RECEIVE:
            if (received_callback && in) {
                received_callback((const char *)in);
            }
            break;
        case LWS_CALLBACK_CLIENT_WRITEABLE:
            if (strlen(message_buffer) > 0) {
                unsigned char buf[LWS_SEND_BUFFER_PRE_PADDING + strlen(message_buffer) + LWS_SEND_BUFFER_POST_PADDING];
                unsigned char *p = &buf[LWS_SEND_BUFFER_PRE_PADDING];
                size_t n = strlen(message_buffer);

                memcpy(p, message_buffer, n);
                lws_write(wsi, p, n, LWS_WRITE_TEXT);
                memset(message_buffer, 0, sizeof(message_buffer)); // Clear the buffer after sending
            }
            break;
        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            printf("Connection error.\n");
            break;
        default:
            break;
    }
    return 0;
}

static struct lws_protocols protocols[] = {
    {
        "example-protocol",
        callback_websockets,
        0,
        128,
    },
    { NULL, NULL, 0, 0 }
};

void* websocket_thread(void *data) {
    while (lws_service(context, 1000) >= 0); // wait 1 second between checks
    return NULL;
}

int initialize_websocket_client(const char *server_address, int port) {
    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));

    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;

    context = lws_create_context(&info);
    if (!context) {
        return -1;
    }

    struct lws_client_connect_info ccinfo = {0};
    ccinfo.context = context;
    ccinfo.address = server_address;
    ccinfo.port = port;
    ccinfo.path = "/";
    ccinfo.host = lws_canonical_hostname(context);
    ccinfo.origin = "origin";
    ccinfo.protocol = protocols[0].name;

    wsi = lws_client_connect_via_info(&ccinfo);
    if (!wsi) {
        return -1;
    }

    pthread_t ws_thread;
    pthread_create(&ws_thread, NULL, websocket_thread, NULL);
    pthread_detach(ws_thread);

    return 0;
}

int send_message(const char *message) {
    strncpy(message_buffer, message, sizeof(message_buffer) - 1);
    lws_callback_on_writable(wsi);
    return 0;
}

void set_data_received_callback(data_received_callback callback) {
    received_callback = callback;
}

void cleanup_websocket_client() {
    lws_context_destroy(context);
}
