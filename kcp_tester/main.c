//
// Created by lichong on 2020/12/21.
//

#include "pitaya.h"
#include <stdio.h>

static void event_cb(pc_client_t* client, int ev_type, void* ex_data, const char* arg1, const char* arg2) {
    printf("receive event %d", ev_type);
}

static void request_cb(const pc_request_t* req, const pc_buf_t *resp) {

}

static void request_err(const pc_request_t* req, pc_error_t *error) {

}

int main() {
    pc_lib_client_info_t client_info;
    client_info.platform = "macos";
    client_info.build_number = "1024";
    client_info.version = "1.0";
    pc_lib_init(NULL, NULL, NULL, NULL, client_info);

    pc_client_config_t config = PC_CLIENT_CONFIG_DEFAULT;
    config.transport_name = PC_TR_NAME_KCP;
    config.enable_reconn = false;
    pc_client_init_result_t result = pc_client_init(NULL, &config);
    pc_client_t *client = result.client;
    pc_client_add_ev_handler(client, event_cb, NULL, NULL);
    pc_client_connect(client, "localhost", 3110, NULL);
    pc_string_request_with_timeout(client, "connector.entry.entry", "hello", NULL, 3000, request_cb, request_err);

    int c;
    do {
        c = getchar();
    } while (c != 'c');

    return 0;
}