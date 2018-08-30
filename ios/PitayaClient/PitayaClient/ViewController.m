//
//  ViewController.m
//  PitayaClient
//
//  Created by Guthyerrz Maciel on 09/04/18.
//  Copyright © 2018 TFG. All rights reserved.
//

#import "ViewController.h"
#import "pitaya.h"
#import "pc_pitaya_i.h"


@interface ViewController ()

@property (weak, nonatomic) IBOutlet UIButton *connectButton;
@property (weak, nonatomic) IBOutlet UITextField *serverAddressTF;
@property (weak, nonatomic) IBOutlet UITextField *routeTF;
@property (weak, nonatomic) IBOutlet UILabel *responseLabel;
@property (weak, nonatomic) IBOutlet UIButton *requestButton;
@property (weak, nonatomic) IBOutlet UILabel *isConnectedLabel;
@property (nonatomic, strong) NSMutableArray *clients;

@end

@implementation ViewController

- (void)viewDidLoad
{
    [super viewDidLoad];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(connectionEstablished:) name:@"connectionOkNotification" object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(gotResponse:) name:@"gotResponse" object:nil];
    pc_lib_client_info_t client_info;
    client_info.platform = "iOS";
    client_info.build_number = "90";
    client_info.version = "1.0";
    NSString *path = [[NSBundle mainBundle] pathForResource:@"ca_root" ofType:@"crt"];
    
    
    pc_lib_init(NULL, NULL, NULL, NULL, client_info);
    // Load trusted CAs from default paths.
    int result = tr_uv_tls_set_ca_file(path.cString, NULL);
    tr_uv_tls_set_ca_file([[NSBundle mainBundle] pathForResource:@"ca" ofType:@"crt"].cString, NULL);
    

    
    
    self.clients = [NSMutableArray array];
}

- (IBAction)onConnectBt:(id)sender {
    pc_client_config_t config = PC_CLIENT_CONFIG_DEFAULT;
    config.transport_name = PC_TR_NAME_UV_TLS;
    config.enable_reconn = false;
    
    pc_client_init_result_t result = pc_client_init(NULL, &config);
    
    pc_client_t* client = result.client;
    [self.clients addObject:[NSValue valueWithBytes:&client objCType:@encode(pc_client_t*)]];
    client->config.reconn_delay = 10;
    pc_client_add_ev_handler(client, event_cb, NULL, NULL);
    NSArray<NSString*>* address = [[_serverAddressTF text] componentsSeparatedByString:@":"];
    pc_client_connect(client, address[0].cString, [address[1] intValue], NULL);
}

static void event_cb(pc_client_t* client, int ev_type, void* ex_data, const char* arg1, const char* arg2)
{
    if(ev_type == PC_EV_CONNECTED){
        NSLog(@"CONNECTED");
        [[NSNotificationCenter defaultCenter] postNotificationName:@"connectionOkNotification" object:nil];
    }
    printf("test: get event %s, arg1: %s, arg2: %s\n", pc_client_ev_str(ev_type),
           arg1 ? arg1 : "", arg2 ? arg2 : "");
}

static void request_cb(const pc_request_t* req, const pc_buf_t *resp)
{
    printf("test get resp %s \n", resp->base);
    [[NSNotificationCenter defaultCenter] postNotificationName:@"gotResponse" object:NULL userInfo:@{@"message": [NSString stringWithCString:resp->base]}];
}

static void error_cb(const pc_request_t* req, pc_error_t *error)
{
    NSLog(@"Error %d %@", error->code, [NSString stringWithCString:error->payload.base]);
}

- (void)gotResponse:(NSNotification *) notification
{
    dispatch_async(dispatch_get_main_queue(), ^{
        NSLog(@"Got response");
        [_responseLabel setText: [@"Response: " stringByAppendingString:[[notification userInfo]objectForKey:@"message"]]];
        [_responseLabel setNeedsDisplay];
    });
}

- (void)connectionEstablished:(NSNotification *) notification
{
    dispatch_async(dispatch_get_main_queue(), ^{
        [_isConnectedLabel setText: @"Connected: true"];
        [_isConnectedLabel setNeedsDisplay];
    });
}

- (IBAction)requestButtonClicked:(id)sender {
    int i = 0;
    for(NSValue *value in self.clients){
        pc_client_t* client;
        [value getValue:&client];
        pc_string_request_with_timeout(client, [@"data.redisHandler.get" cString], [@"{\"key\":\"test\"}" cString], NULL, 10, request_cb, error_cb);
    }
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

@end
