#include "websocket_client.h"
#include <string.h>
#include <pthread.h>
#include <stdlib.h>

static struct lws_context *context = NULL;
static struct lws *wsi = NULL;
static data_received_callback received_callback = NULL;
static const char *server_address_global;
static int port_global;
static int reconnect_flag = 0;

#define MAX_MESSAGES 1024 // Adjust as needed

// Circular buffer for the message queue
typedef struct {
    char *messages[MAX_MESSAGES];
    int head;
    int tail;
} CircularBuffer;

struct lws_client_connect_info ccinfo = {0};

static CircularBuffer buffer = { .head = 0, .tail = 0 };
static pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;

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
            pthread_mutex_lock(&buffer_mutex);
            while (buffer.head != buffer.tail) {
                char *message = buffer.messages[buffer.head];
                unsigned char buf[LWS_SEND_BUFFER_PRE_PADDING + strlen(message) + LWS_SEND_BUFFER_POST_PADDING];
                unsigned char *p = &buf[LWS_SEND_BUFFER_PRE_PADDING];

                memcpy(p, message, strlen(message));
                lws_write(wsi, p, strlen(message), LWS_WRITE_TEXT);

                free(message);
                buffer.head = (buffer.head + 1) % MAX_MESSAGES;
            }
            pthread_mutex_unlock(&buffer_mutex);
            break;
        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
        case LWS_CALLBACK_CLIENT_CLOSED:
            printf("Connection lost. Attempting to reconnect...\n");
            wsi = NULL;
            reconnect_flag = 1;
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
        1024, // Increased frame size for audio buffers
    },
    { NULL, NULL, 0, 0 }
};

void reconnect_ws_client() {
    if (wsi) {
        lws_client_connect_via_info(&ccinfo);
    }
}

void* websocket_thread(void *data) {
    while (1) {
        if (reconnect_flag) {
            reconnect_ws_client();
            reconnect_flag = 0;
            sleep(2);
        }
        lws_service(context, 1);
    }
    return NULL;
}

int ws_init_client(const char *server_address, int port) {
    server_address_global = server_address;
    port_global = port;
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

int ws_send(const char *message) {
    pthread_mutex_lock(&buffer_mutex);
    int nextTail = (buffer.tail + 1) % MAX_MESSAGES;
    if (nextTail != buffer.head) { // Ensure there's space in the buffer
        buffer.messages[buffer.tail] = strdup(message);
        buffer.tail = nextTail;
    }
    pthread_mutex_unlock(&buffer_mutex);
    lws_callback_on_writable(wsi);
    return 0;
}

void ws_set_callback(data_received_callback callback) {
    received_callback = callback;
}

void ws_cleanup() {
    lws_context_destroy(context);
    pthread_mutex_destroy(&buffer_mutex);

    // Cleanup the message buffer
    while (buffer.head != buffer.tail) {
        free(buffer.messages[buffer.head]);
        buffer.head = (buffer.head + 1) % MAX_MESSAGES;
    }
}
