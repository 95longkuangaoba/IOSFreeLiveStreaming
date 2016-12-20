//
//  MyLog.m
//  MyFreeLiveStreamingFramework
//
//  Created by 陈龙 on 16/11/30.
//  Copyright © 2016年 陈龙. All rights reserved.
//

#import "MyLog.h"

#include "self-reliance.h"

@implementation MyLog
-(void)log:(NSString *)message
{
    
   
    
    sr___memory___initialize(1024*1034*4);
    NSLog(@"My-Log-%@",message);
}
@end
