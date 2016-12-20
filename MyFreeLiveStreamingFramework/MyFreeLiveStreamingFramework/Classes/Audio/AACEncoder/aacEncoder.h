//
//  aacEncoder.h
//  MyFreeLiveStreamingFramework
//
//  Created by 陈龙 on 16/12/3.
//  Copyright © 2016年 陈龙. All rights reserved.
//

#import <Foundation/Foundation.h>
@import AVFoundation;
@import AudioToolbox;

@protocol aacEncoderDelegate <NSObject>


//-(AudioClassDescription *)getAudioClassDescriptionWithType:(UInt32)type
//                                           fromManufacturer:(UInt32)manufacturer;



-(NSData *) adtsDataForPacketLength:(NSUInteger)packetLength;
-(NSData *)gotAudioEncodedData;


@end


@interface aacEncoder : NSObject


@property(weak,nonatomic)id<aacEncoderDelegate> myaacEncoderDelegate;

@property(assign,nonatomic)AudioBufferList outAudioBufferList;


-(void)encodeSampleBuffer:(CMSampleBufferRef)sampleBuffer completionBlock:(void (^)(NSData * encodedData,NSError * error))completionBlock;

-(void)audioEndEncoder;

@end
