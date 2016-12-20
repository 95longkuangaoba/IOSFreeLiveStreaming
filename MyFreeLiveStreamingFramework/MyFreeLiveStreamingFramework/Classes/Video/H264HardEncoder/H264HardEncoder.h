//
//  H264HardEncoder.h
//  MyFreeLiveStreamingFramework
//
//  Created by 陈龙 on 16/12/1.
//  Copyright © 2016年 陈龙. All rights reserved.
//

#import <Foundation/Foundation.h>
@import AVFoundation;
@import VideoToolbox;


@protocol H264HardEncoderDelegate <NSObject>

-(void)gotSpsPps:(NSData *)sps pps:(NSData *)pps;
-(void)gotEncodedData:(NSData *)data iskeyFrame:(BOOL)isKeyFrame;

@end


@interface H264HardEncoder : NSObject

-(void)initWithConfiguration;
-(void)initEncode:(int)width height:(int)height;
-(void)encode:(CMSampleBufferRef)sampleBuffer;
-(void)endEncoder;

@property (nonatomic, assign) id<H264HardEncoderDelegate> delegate;

@end
