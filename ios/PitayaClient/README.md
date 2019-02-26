# Libpitaya

Libpitaya is a SDK for building clients for projects using the pitaya game server framework and is built on top of [libpomelo2](https://github.com/NetEase/libpomelo2)

## How to Use


### Initialize the pitaya lib

The `pc_lib_init` must be called before any clients are created and should only be called once. The `client_info` struct is pased on to the pitaya server in the handshake, you can use this for some clever routing.  

``` c
#import "pitaya.h"
#import "pc_pitaya_i.h"

// This info is passed on to the pitaya server in the handshake. You can use this for routing
pc_lib_client_info_t client_info;
client_info.platform = "iOS";
client_info.build_number = "90";
client_info.version = "1.0";

pc_lib_set_default_log_level(PC_LOG_DEBUG);

pc_lib_init(NULL, NULL, NULL, NULL, client_info);
```

To update the `client_info` after the initialization you can use this:
``` c
pc_update_client_info(client_info.version)
```

### Create a pitaya client and start a connection

This config if for connecting with a TCP port
``` c
static void event_cb(pc_client_t* client, int ev_type, void* ex_data, const char* arg1, const char* arg2) {
    if(ev_type == PC_EV_CONNECTED){
        NSLog(@"CONNECTED");
    }
    printf("test: get event %s, arg1: %s, arg2: %s\n", pc_client_ev_str(ev_type), arg1 ? arg1 : "", arg2 ? arg2 : "");
}

void connect(){
    pc_client_config_t config = PC_CLIENT_CONFIG_DEFAULT;
    config.transport_name = PC_TR_NAME_UV_TCP;
    config.enable_reconn = false;

    pc_client_init_result_t result = pc_client_init(NULL, &config);

    pc_client_t* client = result.client;
    pc_client_add_ev_handler(client, event_cb, NULL, NULL);
    pc_client_connect(client, "a1d127034f31611e8858512b1bea90da-838011280.us-east-1.elb.amazonaws.com", 3251, NULL);
}

```

To connect with a TLS port use this example

``` c
// This certificate is used for all clients, you can set this in your setup function
NSString *path = [[NSBundle mainBundle] pathForResource:@"ca_root" ofType:@"crt"];
tr_uv_tls_set_ca_file(path.cString, NULL);

pc_client_config_t config = PC_CLIENT_CONFIG_DEFAULT;
config.transport_name = PC_TR_NAME_UV_TLS;
config.enable_reconn = false;

pc_client_init_result_t result = pc_client_init(NULL, &config);

pc_client_t* client = result.client;
pc_client_add_ev_handler(client, event_cb, NULL, NULL);
pc_client_connect(client, "a1d127034f31611e8858512b1bea90da-838011280.us-east-1.elb.amazonaws.com", 3251, NULL);

```

### Making requests


To make a request with a callback use `pc_string_request_with_timeout`
```
static void request_cb(const pc_request_t* req, const pc_buf_t *resp) {
    printf("test get resp %s \n", resp->base);
}

static void error_cb(const pc_request_t* req, const pc_error_t *error) {
    printf("test get resp %s \n", error->payload.base);
}

pc_string_request_with_timeout(client, "connector.getsessiondata", "{\"key\":\"test\"}", NULL, 15, request_cb, error_cb);

```
To make a request without a timeout or a callback 

``` c
static void notify_error_cb(const pc_notify_t* req, const pc_error_t *error){
    printf("Error %s \n", error->payload.base);
}

pc_string_notify_with_timeout(client, "connector.notifysessiondata", "{\"key\":\"test\"}", NULL, 15, notify_error_cb);
```

### Using protobuf

Generate the objc protobuf files

``` bash
protoc --objc_out=. --proto_path=./Protos/ session-data.proto
```

After generating the files you can include then in your project and use it.
``` objc 
SessionData * d = [[SessionData alloc] init];
d.data_p = @"This is a message";
pc_binary_notify_with_timeout(client, "connector.notifysessiondata", [d.data bytes] ,[d.data length], NULL, 15, notify_error_cb);
```

### The test server

The source code for the test server used in  this example is [here](https://github.com/topfreegames/libpitaya/tree/master/pitaya-servers)
