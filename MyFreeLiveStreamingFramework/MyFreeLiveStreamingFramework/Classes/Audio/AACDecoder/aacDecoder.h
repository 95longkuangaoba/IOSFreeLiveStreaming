//
//  aacDecoder.h
//  MyFreeLiveStreamingFramework
//
//  Created by 陈龙 on 16/12/7.
//  Copyright © 2016年 陈龙. All rights reserved.
//

#import <Foundation/Foundation.h>
@import AudioToolbox;
@interface aacDecoder : NSObject
- (void)play;

- (double)getCurrentTime;



@end
