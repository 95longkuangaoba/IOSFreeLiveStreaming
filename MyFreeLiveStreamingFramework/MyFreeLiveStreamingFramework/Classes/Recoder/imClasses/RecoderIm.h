//
//  RecoderIm.h
//  MyFreeLiveStreamingFramework
//
//  Created by 陈龙 on 16/12/1.
//  Copyright © 2016年 陈龙. All rights reserved.
//

#import <Foundation/Foundation.h>

#import "H264HardEncoder.h"
#import "videoCaptureBaseClass.h"
#import "audioCaptureBaseClass.h"
#import "videoCaptureInBaseClass.h"
#import "audioCaptureInBaseClass.h"
#import "aacEncoder.h"


@import AVFoundation;

@interface RecoderIm : NSObject<H264HardEncoderDelegate,videoCaptureBaseClassDelegate,aacEncoderDelegate,audioCaptureBaseClassDelegate>




-(instancetype)initWithvideoCapture:(videoCaptureInBaseClass *)videoCBS audioCapture:(audioCaptureBaseClass *)audioCBS;

-(void)end;

@end
