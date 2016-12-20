//
//  Player.h
//  MyFreeLiveStreamingFramework
//
//  Created by 陈龙 on 16/12/6.
//  Copyright © 2016年 陈龙. All rights reserved.
//


#import <Foundation/Foundation.h>
#import "../../Video/H264HardDecoder/H264HardDecoder.h"
//#import "H264HardDecoder.h"
//#import "../../Audio/AACDecoder/aacDecoder.h"
#import <UIKit/UIKit.h>

@protocol PlayerDelegate <NSObject>

-(int)fillPCMBuffer:(uint8_t *)buffer bufferSize:(uint32_t)size;

@end



//#import "aacDecoder.h"
@interface Player : NSObject<H264HardDecoderDelegate>


@property(weak,nonatomic)id<PlayerDelegate> delegate;
-(void)audioPlay;


//- (void)play:(void*)pcmData length:(unsigned int)length;

//-(void)display:(CALayer *)layer;
-(void)displayVideo:(CALayer *)layer;

-(void)stopDisplay;
//-(void)xiaohui;
@end
