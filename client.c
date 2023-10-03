#include <stdio.h>
#include <libwebsockets.h>

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
            lws_callback_on_writable(wsi);
            break;
        case LWS_CALLBACK_CLIENT_WRITEABLE: {
            unsigned char msg[512];
            unsigned char *p = &msg[LWS_PRE];
            size_t msg_len = sprintf((char *)p, "Hello from C!");
            lws_write(wsi, p, msg_len, LWS_WRITE_TEXT);
            break;
        }
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

    struct lws_client_connect_info connect_info = {0};
    connect_info.context = context;
    connect_info.address = "localhost";
    connect_info.port = 8080;
    connect_info.path = "/";
    connect_info.protocol = "example-protocol";
    lws_client_connect_via_info(&connect_info);

    while (1) {
        lws_service(context, 1000);
    }

    lws_context_destroy(context);
    return 0;
}
