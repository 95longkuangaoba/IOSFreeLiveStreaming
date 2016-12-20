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
#import "../../Audio/AACDecoder/aacDecoder.h"
#import <UIKit/UIKit.h>
//#import "aacDecoder.h"
@interface Player : NSObject<H264HardDecoderDelegate>

-(void)audioPlay;

-(void)videoPlay:(UIView *)view;


-(void)videoStop;
@end
