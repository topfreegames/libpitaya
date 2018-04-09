//
//  ViewController.m
//  PitayaClient
//
//  Created by Guthyerrz Maciel on 09/04/18.
//  Copyright © 2018 TFG. All rights reserved.
//

#import "ViewController.h"
#import "pomelo.h"

static pc_client_t* client;


@interface ViewController ()

@property (weak, nonatomic) IBOutlet UIButton *connectButton;
@property (weak, nonatomic) IBOutlet UITextField *serverAddressTF;
@property (weak, nonatomic) IBOutlet UITextField *routeTF;
@property (weak, nonatomic) IBOutlet UILabel *responseLabel;
@property (weak, nonatomic) IBOutlet UIButton *requestButton;
@property (weak, nonatomic) IBOutlet UILabel *isConnectedLabel;

@end

@implementation ViewController

- (void)viewDidLoad
{
    [super viewDidLoad];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(connectionEstablished:) name:@"connectionOkNotification" object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(gotResponse:) name:@"gotResponse" object:nil];
}

- (IBAction)onConnectBt:(id)sender {
    pc_client_config_t config = PC_CLIENT_CONFIG_DEFAULT;
    config.transport_name = PC_TR_NAME_UV_TCP;
    pc_lib_init(NULL, NULL, NULL, NULL);
    client = (pc_client_t*)malloc(pc_client_size());
    pc_client_init(client, NULL, &config);
    pc_client_add_ev_handler(client, event_cb, NULL, NULL);
    NSArray<NSString*>* address = [[_serverAddressTF text] componentsSeparatedByString:@":"];
    pc_client_connect(client, address[0].cString, [address[1] intValue], NULL);
}

static void event_cb(pc_client_t* client, int ev_type, void* ex_data, const char* arg1, const char* arg2)
{
    if(ev_type == PC_EV_CONNECTED){
        [[NSNotificationCenter defaultCenter] postNotificationName:@"connectionOkNotification" object:nil];
    }
    printf("test: get event %s, arg1: %s, arg2: %s\n", pc_client_ev_str(ev_type),
           arg1 ? arg1 : "", arg2 ? arg2 : "");
}

static void request_cb(const pc_request_t* req, int rc, const char* resp)
{
    printf("test get resp %s\n", resp);
    [[NSNotificationCenter defaultCenter] postNotificationName:@"gotResponse" object:NULL userInfo:@{@"message": [NSString stringWithCString:resp]}];
}

- (void)gotResponse:(NSNotification *) notification
{
    dispatch_async(dispatch_get_main_queue(), ^{
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
    pc_request_with_timeout(client, [[_routeTF text] cString], "{}", NULL, 10, request_cb);
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

@end
