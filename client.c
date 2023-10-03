#include <stdio.h>
#include <string.h>
#include <libwebsockets.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>  // for sleep on Unix-like systems

static struct lws *global_wsi = NULL;
static struct lws_client_connect_info connect_info;

// Basic linked list structure for message queue
typedef struct node {
    char *message;
    struct node *next;
} node_t;

static node_t *head = NULL;
static node_t *tail = NULL;

static pthread_mutex_t queue_lock = PTHREAD_MUTEX_INITIALIZER;

void enqueue(const char *message) {
    node_t *new_node = malloc(sizeof(node_t));
    if (new_node) {
        new_node->message = strdup(message);
        new_node->next = NULL;

        if (tail == NULL) {
            head = new_node;
            tail = new_node;
        } else {
            tail->next = new_node;
            tail = new_node;
        }
    }
}

char *dequeue() {
    if (head == NULL) {
        return NULL;
    }
    node_t *old_head = head;
    char *message = old_head->message;
    head = head->next;
    if (head == NULL) {
        tail = NULL;
    }
    free(old_head);
    return message;
}

void send_to_ws(const char *message) {
    pthread_mutex_lock(&queue_lock);
    
    enqueue(message);
    if (global_wsi) {
        lws_callback_on_writable(global_wsi);
    }
    
    pthread_mutex_unlock(&queue_lock);
}

static int callback_client(
    struct lws *wsi,
    enum lws_callback_reasons reason,
    void *user,
    void *in,
    size_t len
) {
    switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            printf("Connected to server\n");
            global_wsi = wsi;
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

                // If more messages, trigger another writable event
                if (head) {
                    lws_callback_on_writable(global_wsi);
                }
            }

            pthread_mutex_unlock(&queue_lock);
            break;
        }
        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
        case LWS_CALLBACK_CLIENT_CLOSED:
            printf("Disconnected from server. Waiting 5 seconds before reconnecting...\n");
            global_wsi = NULL;

            sleep(5);  // Wait for 5 seconds

            while (!global_wsi) {
                printf("Attempting to reconnect...\n");
                global_wsi = lws_client_connect_via_info(&connect_info);
                if (!global_wsi) {
                    printf("Reconnection failed. Retrying in 5 seconds...\n");
                    sleep(5);  // Wait for 5 seconds before retrying
                }
            }
            break;
        case LWS_CALLBACK_CLIENT_RECEIVE:
            printf("Received data: %s\n", (char *)in);
            break;
        default:
            break;
    }
    return 0;
}

int main(void) {
    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));

    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = (const struct lws_protocols[]) {
        {
            .name = "example-protocol",
            .callback = callback_client,
            .rx_buffer_size = 128
        },
        { NULL, NULL, 0 }
    };

    struct lws_context *context = lws_create_context(&info);
    if (!context) {
        printf("Failed to create context\n");
        return -1;
    }

    memset(&connect_info, 0, sizeof(connect_info));  // Update to use memset for initialization
    connect_info.context = context;
    connect_info.address = "localhost";
    connect_info.port = 8082;
    connect_info.path = "/";
    connect_info.protocol = "example-protocol";
    lws_client_connect_via_info(&connect_info);

    while (1) {
        lws_service(context, 1000);
        send_to_ws("TEST MESSAGE");
    }

    lws_context_destroy(context);
    return 0;
}
