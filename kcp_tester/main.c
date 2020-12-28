//
// Created by lichong on 2020/12/21.
//

#include "pitaya.h"
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

static void event_cb(pc_client_t* client, int ev_type, void* ex_data, const char* arg1, const char* arg2) {
    printf("receive event %d", ev_type);
}

static void request_cb(const pc_request_t* req, const pc_buf_t *resp) {
    printf("request cb with resp %s\n", resp->base);
}

static void request_err(const pc_request_t* req, const pc_error_t *error) {
    printf("request cb with error %s\n", error->payload.base);
}

static int running = 1;
static void* thread_fn(void *arg) {
    pc_client_t *client = arg;
    while (running) {
        sleep(2);
        pc_string_request_with_timeout(client, "metagame.gameInfo.getGameInfo", "{}", NULL, 3, request_cb, request_err);
    }
    return NULL;
}

int main() {
    pc_lib_client_info_t client_info;
    client_info.platform = "macos";
    client_info.build_number = "1024";
    client_info.version = "1.0";
    pc_lib_init(NULL, NULL, NULL, NULL, client_info);

    pc_client_config_t config = PC_CLIENT_CONFIG_DEFAULT;
    config.transport_name = PC_TR_NAME_KCP;
    config.enable_reconn = 1;
    pc_client_init_result_t result = pc_client_init(NULL, &config);
    pc_client_t *client = result.client;
    pc_client_add_ev_handler(client, event_cb, NULL, NULL);
    pc_client_connect(client, "127.0.0.1", 3110, NULL);

    int c;

    pthread_t thread;
    pthread_create(&thread, NULL, thread_fn, client);
    do {
        c = getchar();
        if (c == 'a') {
            const char *msg = "{\"private_id\":\"8d89b428-4e06-43fb-a018-96653dd8b383\"}";
            pc_string_request_with_timeout(client, "metagame.player.authenticatePlayer", msg, NULL, 3, request_cb, request_err);
        }
        if (c == 'b') {
            pc_string_request_with_timeout(client, "connector.entry.entry", "{}", NULL, 3, request_cb, request_err);
        }
        if (c == 'z') {
            pc_string_request_with_timeout(client ,"metagame.player.failed", "{}", NULL, 3, request_cb, request_err);
        }
    } while (c != 'c');
    running = 0;
    pthread_join(thread, NULL);
    pc_client_disconnect(client);
    pc_client_cleanup(client);

    return 0;
}