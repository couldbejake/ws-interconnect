#include "ws_client.h"
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

static pthread_mutex_t queue_lock = PTHREAD_MUTEX_INITIALIZER;

void send_to_ws(WebSocketClient *client, const char *message);

// Linked list structure for message queue
typedef struct node {
    char *message;
    struct node *next;
} node_t;

static node_t *head = NULL;
static node_t *tail = NULL;

void enqueue(const char *message) {
    node_t *new_node = malloc(sizeof(node_t));
    if (new_node) {
        new_node->message = strdup(message);
        new_node->next = NULL;
        if (!tail) {
            head = tail = new_node;
        } else {
            tail->next = new_node;
            tail = new_node;
        }
    }
}

char *dequeue() {
    if (!head) return NULL;
    node_t *old_head = head;
    char *message = old_head->message;
    head = head->next;
    if (!head) tail = NULL;
    free(old_head);
    return message;
}
static int ws_callback(
    struct lws *wsi,
    enum lws_callback_reasons reason,
    void *user,
    void *in,
    size_t len
) {
    WebSocketClient *client = (WebSocketClient *)user;

    switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            printf("Connected to %s:%d\n", client->address, client->port);
            client->wsi = wsi;
            break;

        case LWS_CALLBACK_CLIENT_WRITEABLE: {
            pthread_mutex_lock(&queue_lock);
            char *message = dequeue();
            if (message) {
                unsigned char buffer[512];
                unsigned char *p = &buffer[LWS_PRE];
                size_t msg_len = snprintf((char *)p, sizeof(buffer) - LWS_PRE, "%s", message);
                lws_write(wsi, p, msg_len, LWS_WRITE_TEXT);
                free(message);
                if (head) {
                    lws_callback_on_writable(client->wsi);
                }
            }
            pthread_mutex_unlock(&queue_lock);
            break;
        }

        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            printf("Connection error. Attempting to reconnect...\n");
            sleep(5);
            client->wsi = NULL;
            client->wsi = lws_client_connect_via_info(&client->connect_info);
            break;

        case LWS_CALLBACK_CLIENT_RECEIVE:
            if (client->data_received_callback) {
                client->data_received_callback((char *)in);
            }
            break;

        default:
            break;
    }

    return 0;
}

void *ws_thread(void *arg) {
    WebSocketClient *client = (WebSocketClient *)arg;
    struct lws_context_creation_info info = {0};
    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = (const struct lws_protocols[]) {
        {"custom-protocol", ws_callback, 0, 128, 0, client},
        {NULL, NULL, 0}
    };


    struct lws_context *context = lws_create_context(&info);
    if (!context) {
        printf("Failed to create context\n");
        return NULL;
    }

    client->connect_info.context = context;
    client->connect_info.address = client->address;
    client->connect_info.port = client->port;
    client->connect_info.path = "/";
    client->connect_info.protocol = "custom-protocol";
    client->connect_info.userdata = client;
    lws_client_connect_via_info(&client->connect_info);

    while (1) {
        lws_service(context, 1000);
    }

    lws_context_destroy(context);
    return NULL;
}

WebSocketClient *initialize_ws(const char *address, int port, void (*callback)(const char*)) {
    WebSocketClient *client = (WebSocketClient *)malloc(sizeof(WebSocketClient));
    if (!client) {
        return NULL;
    }

    client->address = strdup(address);
    client->port = port;
    client->data_received_callback = callback;

    pthread_t ws_thread_id;
    pthread_create(&ws_thread_id, NULL, ws_thread, client);

    return client;
}

void send_message(WebSocketClient *client, const char *message) {
    send_to_ws(client, message);
}

void send_to_ws(WebSocketClient *client, const char *message) {
    pthread_mutex_lock(&queue_lock);
    enqueue(message);
    if (client->wsi) {
        lws_callback_on_writable(client->wsi);
    }
    pthread_mutex_unlock(&queue_lock);
}