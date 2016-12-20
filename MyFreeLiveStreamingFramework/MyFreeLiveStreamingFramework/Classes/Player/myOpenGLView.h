//
//  myOpenGLView.h
//  MyFreeLiveStreamingFramework
//
//  Created by 陈龙 on 16/12/7.
//  Copyright © 2016年 陈龙. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>

@interface myOpenGLView : UIView


- (void)setupGL;
- (void)displayPixelBuffer:(CVPixelBufferRef)pixelBuffer;


@end
