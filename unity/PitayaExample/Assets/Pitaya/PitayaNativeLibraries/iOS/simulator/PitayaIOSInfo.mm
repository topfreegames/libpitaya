//
//  PitayaIOSInfo.c
//  PitayaClient
//
//  Created by Guthyerrz Silva on 27/09/18.
//  Copyright © 2018 TFG. All rights reserved.
//

#import <Foundation/NSBundle.h>
#include <string.h>

extern "C" {
    const char * _PitayaGetCFBundleVersion() {
        NSString *version = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleVersion"];
        return strdup([version UTF8String]);
    }
    const char * _PitayaGetCFShortVersion() {
        NSString * appVersionString = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleShortVersionString"];
        return strdup([appVersionString UTF8String]);
    }
    const char * _PitayaGetCFBundleIdentifier() {
        NSString *version = [[NSBundle mainBundle] bundleIdentifier];
        return strdup([version UTF8String]);
    }
}
