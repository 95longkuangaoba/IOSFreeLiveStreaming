//
//  myAACDecoder.h
//  MyFreeLiveStreamingFramework
//
//  Created by 陈龙 on 16/12/18.
//  Copyright © 2016年 陈龙. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "neaacdec.h"

@protocol myAACDecoderDelegate <NSObject>



@end


@interface myAACDecoder : NSObject


-(instancetype)initAACDecoderbuffer:(unsigned char *)buffer buffer_size:(unsigned long)buffer_size samplerate:(unsigned long)samplerate channels:(unsigned char *)channels;


-(NeAACDecFrameInfo *)getAACFrameInfosamples:(unsigned long)samples channels:(unsigned long)channels samplerate:(unsigned long)samplerate bitrate:(int32_t)bitrate sampleperframe:(int32_t)sample_per_frame;

-(void *)aacDecoderDatahInfo:(NeAACDecFrameInfo *)hInfo buffer:(unsigned char *)buffer buffer_size:(unsigned long)buffer_size;


-(void)closeAACDeoder;

@end
