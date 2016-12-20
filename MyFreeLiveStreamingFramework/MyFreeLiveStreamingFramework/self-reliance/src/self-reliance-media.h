#ifndef ___SELF_RELIANCE_MEDIA_H___
#define ___SELF_RELIANCE_MEDIA_H___



#include "self-reliance.h"


enum
{
	SR___Media_Type___Begin = 0x00,
	SR___Media_Type___Video = 0x01,
	SR___Media_Type___Audio = 0x02,
	SR___Media_Type___Words = 0x03,
	SR___Media_Type___Ended = 0x07,
};


enum
{
	SR___Payload_Type___Frame = 0x00,
	SR___Payload_Type___Format = 0x01
};


enum
{
	SR___Video_Extend_Type___None = 0x00,
	SR___Video_Extend_Type___KeyFrame = 0x01
};


typedef struct sr___frame_slice___t
{
	struct
	{
		uint8_t media:3;
		uint8_t payload:1;
		uint8_t extend:4;

	}type;

	uint8_t frame_index;
	uint16_t slice_total;
	uint16_t slice_index;
	uint16_t slice_size;
	uint64_t timestamp;

}sr___frame_slice___t;


enum
{
	SR___Audio_Codec_Type___AAC = -1,
	SR___Audio_Codec_Type___NONE = 0,
};


enum
{
	SR___Video_Codec_Type___AVC = -1,
	SR___Video_Codec_Type___NONE = 0,
};


enum
{
	SR___Audio_Sample_Type___U8 = 0,
	SR___Audio_Sample_Type___S16 = 1,
};


enum
{
	SR___Video_Pixel_Type___YUV = 0,
	SR___Video_Pixel_Type___RGB = 1
};


typedef struct sr___audio_format___t
{
	int32_t codec_type;
	int32_t bit_rate;
	int32_t channels;
	int32_t sample_rate;
	int32_t sample_size;
	int32_t sample_type;
	int32_t sample_per_frame;
	int32_t reserved;
	uint8_t extend_data_size;
	uint8_t extend_data[255];

}sr___audio_format___t;


typedef struct sr___video_format___t
{	
	int32_t codec_type;
	int32_t bit_rate;
	int32_t width;
	int32_t height;
	int32_t pixel_size;
	int32_t pixel_type;
	int32_t frame_per_second;
	int32_t key_frame_interval;
	uint8_t extend_data_size;
	uint8_t extend_data[255];

}sr___video_format___t;


typedef struct sr___media_frame___t
{
	sr___node___t node;
	sr___frame_slice___t slice;
	size_t size;
	uint8_t *data;

}sr___media_frame___t;


extern int sr___media_frame___create(sr___media_frame___t **pp_frame);
extern void sr___media_frame___release(sr___media_frame___t **pp_frame);

extern sr___media_frame___t* sr___audio_format___to___media_frame(sr___audio_format___t *format);
extern sr___media_frame___t* sr___video_format___to___media_frame(sr___video_format___t *format);

extern sr___audio_format___t* sr___media_frame___to___audio_format(sr___media_frame___t *frame);
extern sr___video_format___t* sr___media_frame___to___video_format(sr___media_frame___t *frame);


enum {
	EVENT_ERROR = -1,
	EVENT_OPENING,
	EVENT_PLAYING,
	EVENT_ENDING,
	EVENT_PAUSING,
	EVENT_SEEKING,
	EVENT_BUFFERING,
	EVENT_DURATION = 100,
	EVENT_PROGRESS
};

typedef struct sr___media_event___t
{
	int type;
	union {
		int32_t i32;
		uint32_t u32;
		int64_t i64;
		uint64_t u64;
		double f100;
	};
} sr___media_event___t;

typedef struct sr___media_event_callback___t
{
	void *reciever;
	void (*send_event)(struct sr___media_event_callback___t *, sr___media_event___t event);

} sr___media_event_callback___t;


typedef struct sr___event_listener___t sr___event_listener___t;

int sr___event_listener___create(sr___media_event_callback___t *cb, sr___event_listener___t **pp_listener);
void sr___event_listener___release(sr___event_listener___t **pp_listener);

void sr___event_listener___push_state(sr___event_listener___t *listener, int type);
void sr___event_listener___push_error(sr___event_listener___t *listener, int errorcode);
void sr___event_listener___push_event(sr___event_listener___t *listener, sr___media_event___t event);
void sr___event_listener___push_progress(sr___event_listener___t *listener, int64_t microsecond);


typedef struct sr___synchronous_clock___t sr___synchronous_clock___t;

int sr___synchronous_clock___create(sr___event_listener___t *listener, sr___synchronous_clock___t **pp_clock);
void sr___synchronous_clock___release(sr___synchronous_clock___t **pp_clock);

void sr___synchronous_clock___reboot(sr___synchronous_clock___t *clock);
int64_t sr___synchronous_clock___update_audio_time(sr___synchronous_clock___t *clock, int64_t microsecond);
int64_t sr___synchronous_clock___update_video_time(sr___synchronous_clock___t *clock, int64_t microsecond);


typedef struct sr___muxer___t sr___muxer___t;

extern int sr___muxer___create(const char *url,
		bool with_audio,
		bool with_video,
		sr___event_listener___t *listener,
		sr___muxer___t **pp_muxer);
extern void sr___muxer___release(sr___muxer___t **pp_muxer);
extern void sr___muxer___stop(sr___muxer___t *muxer);
extern int sr___muxer___frame(sr___muxer___t *muxer, sr___media_frame___t *frame);


typedef struct sr___demux_callback___t
{
	void *user;
	int (*demux_audio)(struct sr___demux_callback___t *callback, sr___media_frame___t *frame);
	int (*demux_video)(struct sr___demux_callback___t *callback, sr___media_frame___t *frame);

}sr___demux_callback___t;

typedef struct sr___demux___t sr___demux___t;

extern int sr___demux___create(const char *url,
		sr___demux_callback___t *callback,
		sr___event_listener___t *listener,
		sr___demux___t **pp_demux);
extern void sr___demux___release(sr___demux___t **pp_demux);
extern void sr___demux___stop(sr___demux___t *demux);
extern void sr___demux___seek(sr___demux___t *demux, int64_t seek_time);



#endif //___SELF_RELIANCE_MEDIA_H___
