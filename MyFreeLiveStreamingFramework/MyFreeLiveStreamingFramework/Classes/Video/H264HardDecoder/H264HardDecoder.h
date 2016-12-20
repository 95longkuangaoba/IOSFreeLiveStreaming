//
//  H264HardDecoder.h
//  MyFreeLiveStreamingFramework
//
//  Created by 陈龙 on 16/12/6.
//  Copyright © 2016年 陈龙. All rights reserved.
//

#import <Foundation/Foundation.h>
//#import "myOpenGLView.h"

@import AVFoundation;
@import VideoToolbox;

/*
@protocol H264HardDecoderDelegate <NSObject>

-(void)displayPixelBuffer:(CVImageBufferRef)imageBuffer;

@end



@interface H264HardDecoder : NSObject
@property(weak,nonatomic)id<H264HardDecoderDelegate> videoDecoderDelegate;


-(void)decoderNalu:(uint8_t *)frame withSize:(uint32_t)frameSize;

-(void)startDecode;
*/








@protocol H264HardDecoderDelegate <NSObject>



-(void)displayDecodedFrame:(CVImageBufferRef)imageBuffer;

@end



@interface H264HardDecoder : NSObject
@property(weak,nonatomic)id<H264HardDecoderDelegate> videoDecoderDelegate;


//-(void)decodeNalu:(uint8_t *)frame withSize:(uint32_t)frameSize;
-(BOOL)initH264Decoder;
//-(CVPixelBufferRef)decode:(uint8_t *)frame withSize:(uint32_t)frameSize;
-(void)decodeH264HeaderSPS:(uint8_t *)sps andwithSpsSize:(NSInteger)sps_size andwithpps:(uint8_t *)pps withppsSize:(NSInteger)pps_size;


-(CVPixelBufferRef)decode:(uint8_t *)videoData withSize:(uint32_t)dataSize;
//-(void)getpixelBuffer:(CVPixelBufferRef )pixBuffer;
@end
