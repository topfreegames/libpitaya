//
//  PitayaClientTests.m
//  PitayaClientTests
//
//  Created by Guthyerrz Silva on 26/02/19.
//  Copyright © 2019 TFG. All rights reserved.
//

#import <XCTest/XCTest.h>
#import "pitaya.h"
#import "pc_pitaya_i.h"
#import "SessionData.h"

@interface PitayaClientTests : XCTestCase

@end

static XCTestExpectation *connect_expectation;
static XCTestExpectation *request_expectation;
static XCTestExpectation *push_expectation;

@implementation PitayaClientTests

- (void)setUp {
    pc_lib_client_info_t client_info;
    client_info.platform = "iOS";
    client_info.build_number = "90";
    client_info.version = "1.0";
    
    pc_lib_init(NULL, NULL, NULL, NULL, client_info);
}

- (void)tearDown {
    // Put teardown code here. This method is called after the invocation of each test method in the class.
}

- (void)testConnect {
    connect_expectation = [self expectationWithDescription:@"Connection oppened"];
    
    pc_client_config_t config = PC_CLIENT_CONFIG_DEFAULT;
    config.transport_name = PC_TR_NAME_UV_TCP;
    config.enable_reconn = false;
    
    pc_client_init_result_t result = pc_client_init(NULL, &config);
    
    pc_client_t* client = result.client;
    pc_client_add_ev_handler(client, event_cb, NULL, NULL);
    pc_client_connect(client, "a1d127034f31611e8858512b1bea90da-838011280.us-east-1.elb.amazonaws.com", 3251, NULL);
    
    [self waitForExpectationsWithTimeout:10 handler:^(NSError * _Nullable error) {
        if(error){
            NSLog(@"%@", error);
            XCTFail(@"Failed to connect to the server");
        }
    }];
}


- (void)testRequest {
    
    connect_expectation = [self expectationWithDescription:@"Connection oppened"];
    
    pc_client_config_t config = PC_CLIENT_CONFIG_DEFAULT;
    config.transport_name = PC_TR_NAME_UV_TCP;
    config.enable_reconn = false;
    
    pc_client_init_result_t result = pc_client_init(NULL, &config);
    
    pc_client_t* client = result.client;
    pc_client_add_ev_handler(client, event_cb, NULL, NULL);
    pc_client_connect(client, "a1d127034f31611e8858512b1bea90da-838011280.us-east-1.elb.amazonaws.com", 3251, NULL);
    
    [self waitForExpectations:@[connect_expectation] timeout:10];
    
    request_expectation = [self expectationWithDescription:@"Request"];
    pc_string_request_with_timeout(client, "connector.getsessiondata", "{\"key\":\"test\"}", NULL, 15, request_cb, error_cb);
    
    [self waitForExpectations:@[request_expectation] timeout:10];
}

- (void)testPush {
    
    
    connect_expectation = [self expectationWithDescription:@"Connection oppened"];
    
    pc_client_config_t config = PC_CLIENT_CONFIG_DEFAULT;
    config.transport_name = PC_TR_NAME_UV_TCP;
    config.enable_reconn = false;
    
    pc_client_init_result_t result = pc_client_init(NULL, &config);
    
    pc_client_t* client = result.client;
    pc_client_add_ev_handler(client, event_cb, NULL, NULL);
    pc_client_connect(client, "a1d127034f31611e8858512b1bea90da-838011280.us-east-1.elb.amazonaws.com", 3351, NULL);
    
    [self waitForExpectations:@[connect_expectation] timeout:10];
    
    push_expectation = [self expectationWithDescription:@"Push"];
    pc_client_set_push_handler(client, push_cb);
    
    SessionData * d = [[SessionData alloc] init];
    d.data_p = @"This is a message";
    pc_binary_notify_with_timeout(client, "connector.notifysessiondata", [d.data bytes] ,[d.data length], NULL, 15, notify_error_cb);
    
    [self waitForExpectations:@[push_expectation] timeout:15];
    
    
}

static void push_cb(pc_client_t *client, const char *route, const pc_buf_t *payload){
    [push_expectation fulfill];
}

static void event_cb(pc_client_t* client, int ev_type, void* ex_data, const char* arg1, const char* arg2)
{
    if(ev_type == PC_EV_CONNECTED){
        NSLog(@"CONNECTED");
        [connect_expectation fulfill];
    }
    printf("test: get event %s, arg1: %s, arg2: %s\n", pc_client_ev_str(ev_type),
           arg1 ? arg1 : "", arg2 ? arg2 : "");
}

static void request_cb(const pc_request_t* req, const pc_buf_t *resp)
{
    printf("test get resp %s \n", resp->base);
    [request_expectation fulfill];
}

static void error_cb(const pc_request_t* req, const pc_error_t *error)
{
    printf("Error %s \n", error->payload.base);
}


static void notify_error_cb(const pc_notify_t* req, const pc_error_t *error){
    printf("Error %s \n", error->payload.base);
}

@end
