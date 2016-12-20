#include "self-reliance-media.h"


#include <pthread.h>


#define EVENT_QUEUE_SIZE            1024

typedef struct sr___event_listener___t
{
	bool running;
	bool stopped;
	pthread_t tid;
	sr___atom___t *atom;
	sr___pipe___t *pipe;
	sr___media_event_callback___t *cb;

} sr___event_listener___t;


static void *sr___event_listener___loop(void *p)
{
	log_debug("sr___event_listener___loop enter\n");

	int result = 0;
	sr___media_event___t event = {0};
	unsigned int size = sizeof(event);
	sr___event_listener___t *listener = (sr___event_listener___t *) p;

	while (ISTRUE(listener->running)) {

		sr___atom___lock(listener->atom);

		if (sr___pipe___readable(listener->pipe) < size){
			sr___atom___wait(listener->atom);
			if (ISFALSE(listener->running)) {
				sr___atom___unlock(listener->atom);
				break;
			}
		}

		result = sr___pipe___read(listener->pipe, (char *) &event, size);

		sr___atom___unlock(listener->atom);

		if (result < 0) {
			log_error(result);
			break;
		}

		listener->cb->send_event(listener->cb, event);
	}

	SETTRUE(listener->stopped);

	log_debug("sr___event_listener___loop exit\n");

	return NULL;
}


int sr___event_listener___create(sr___media_event_callback___t *cb, sr___event_listener___t **pp_listener)
{
	log_debug("sr___event_listener___create enter\n");

	int result = 0;
	sr___event_listener___t *listener = NULL;

	if (pp_listener == NULL || cb == NULL || cb->send_event == NULL) {
		log_error(ERRPARAM);
		return ERRPARAM;
	}

	if ((listener = (sr___event_listener___t *) srmalloc(sizeof(sr___event_listener___t))) == NULL) {
		log_error(ERRMEMORY);
		return ERRMEMORY;
	}

	listener->cb = cb;
	result = sr___pipe___create(sizeof(sr___media_event___t) * EVENT_QUEUE_SIZE, &(listener->pipe));
	if (result != 0) {
		sr___event_listener___release(&listener);
		log_error(result);
		return result;
	}

	result = sr___atom___create(&(listener->atom));
	if (result != 0) {
		sr___event_listener___release(&listener);
		log_error(result);
		return result;
	}

	SETTRUE(listener->running);
	result = pthread_create(&(listener->tid), NULL, sr___event_listener___loop, listener);
	if (result != 0) {
		listener->tid = 0;
		sr___event_listener___release(&listener);
		log_error(ERRSYSCALL);
		return ERRSYSCALL;
	}

	*pp_listener = listener;

	log_debug("sr___event_listener___create exit\n");

	return 0;
}


void sr___event_listener___release(sr___event_listener___t **pp_listener)
{
	log_debug("sr___event_listener___release enter\n");

	if (pp_listener && *pp_listener) {
		sr___event_listener___t *listener = *pp_listener;
		*pp_listener = NULL;
		if (listener->tid != 0){
			SETFALSE(listener->running);
			while(ISFALSE(listener->stopped)){
				sr___atom___lock(listener->atom);
				sr___atom___broadcast(listener->atom);
				sr___atom___unlock(listener->atom);
				nanosleep((const struct timespec[]){{0, 10L}}, NULL);
			}
			pthread_join(listener->tid, NULL);
		}
		sr___pipe___release(&(listener->pipe));
		sr___atom___release(&(listener->atom));
		srfree(listener);
	}

	log_debug("sr___event_listener___release exit\n");
}


void sr___event_listener___push_event(sr___event_listener___t *listener, sr___media_event___t event)
{
	int result = 0;

	if (listener != NULL) {
		if (sr___pipe___writable(listener->pipe) >= sizeof(event)) {
			sr___atom___lock(listener->atom);
			if ((result = sr___pipe___write(listener->pipe, (char *) &event, sizeof(event))) != sizeof(event)) {
				log_error(result);
			}
			sr___atom___signal(listener->atom);
			sr___atom___unlock(listener->atom);
		}
	}
}


void sr___event_listener___push_state(sr___event_listener___t *listener, int type)
{
	int result = 0;

	if (listener != NULL) {
		sr___media_event___t event = { type, 0 };
		if (sr___pipe___writable(listener->pipe) >= sizeof(event)) {
			sr___atom___lock(listener->atom);
			if ((result = sr___pipe___write(listener->pipe, (char *) &event, sizeof(event))) != sizeof(event)) {
				log_error(result);
			}
			sr___atom___signal(listener->atom);
			sr___atom___unlock(listener->atom);
		}
	}
}


void sr___event_listener___push_error(sr___event_listener___t *listener, int errorcode)
{
	int result = 0;

	if (listener != NULL) {
		sr___media_event___t event = { .type = EVENT_ERROR, .i32 = errorcode };
		if (sr___pipe___writable(listener->pipe) >= sizeof(event)) {
			sr___atom___lock(listener->atom);
			if ((result = sr___pipe___write(listener->pipe, (char *) &event, sizeof(event))) != sizeof(event)) {
				log_error(result);
			}
			sr___atom___signal(listener->atom);
			sr___atom___unlock(listener->atom);
		}
	}
}


void sr___event_listener___push_progress(sr___event_listener___t *listener, int64_t microsecond)
{
	int result = 0;

	if (listener != NULL) {
		sr___media_event___t event = { .type = EVENT_PROGRESS, .u64 = microsecond };
		if (sr___pipe___writable(listener->pipe) >= sizeof(event)) {
			sr___atom___lock(listener->atom);
			if ((result = sr___pipe___write(listener->pipe, (char *) &event, sizeof(event))) != sizeof(event)) {
				log_error(result);
			}
			sr___atom___signal(listener->atom);
			sr___atom___unlock(listener->atom);
		}
	}
}



static void sr___node___release(sr___node___t *node)
{
	if (node && node->content){
		sr___media_frame___t *frame = (sr___media_frame___t *)(node->content);
		if (frame->data){
			srfree(frame->data);
		}
		srfree(frame);
	}
}


int sr___media_frame___create(sr___media_frame___t **pp_frame)
{
	sr___media_frame___t *frame = (sr___media_frame___t *)srmalloc(sizeof(sr___media_frame___t));

	if (frame == NULL){
		log_error(ERRMEMORY);
		return ERRMEMORY;
	}

	frame->node.content = frame;
	frame->node.release = sr___node___release;

	*pp_frame = frame;

	return 0;
}


void sr___media_frame___release(sr___media_frame___t **pp_frame)
{
	if (pp_frame && *pp_frame){
		sr___media_frame___t *frame = *pp_frame;
		*pp_frame = NULL;
		srfree(frame->data);
		srfree(frame);
	}
}


sr___media_frame___t* sr___audio_format___to___media_frame(sr___audio_format___t *format)
{
	int result = 0;
	uint8_t *ptr = NULL;
	sr___media_frame___t *frame = NULL;

	if ((result = sr___media_frame___create(&frame)) != 0){
		log_error(result);
		return NULL;
	}

	frame->size = sizeof(sr___audio_format___t);
	frame->data = (uint8_t *)srmalloc(frame->size);
	if (frame->data == NULL){
		srfree(frame);
		log_error(ERRMEMORY);
		return NULL;
	}

	frame->slice.type.media = SR___Media_Type___Audio;
	frame->slice.type.payload = SR___Payload_Type___Format;
	frame->slice.timestamp = 0;

	ptr = frame->data;
	PUSH_INT32(ptr, format->codec_type);
	PUSH_INT32(ptr, format->bit_rate);
	PUSH_INT32(ptr, format->channels);
	PUSH_INT32(ptr, format->sample_rate);
	PUSH_INT32(ptr, format->sample_size);
	PUSH_INT32(ptr, format->sample_type);
	PUSH_INT32(ptr, format->sample_per_frame);
	PUSH_INT32(ptr, format->reserved);
	PUSH_INT8(ptr, format->extend_data_size);
	memcpy(ptr, format->extend_data, format->extend_data_size);

	return frame;
}


sr___media_frame___t* sr___video_format___to___media_frame(sr___video_format___t *format)
{
	int result = 0;
	uint8_t *ptr = NULL;
	sr___media_frame___t *frame = NULL;

	if ((result = sr___media_frame___create(&frame)) != 0){
		log_error(result);
		return NULL;
	}

	frame->size = sizeof(sr___video_format___t);
	frame->data = (uint8_t *)srmalloc(frame->size);
	if (frame->data == NULL){
		srfree(frame);
		log_error(ERRMEMORY);
		return NULL;
	}

	frame->slice.type.media = SR___Media_Type___Video;
	frame->slice.type.payload = SR___Payload_Type___Format;
	frame->slice.timestamp = 0;

	ptr = frame->data;

	PUSH_INT32(ptr, format->codec_type);
	PUSH_INT32(ptr, format->bit_rate);
	PUSH_INT32(ptr, format->width);
	PUSH_INT32(ptr, format->height);
	PUSH_INT32(ptr, format->pixel_size);
	PUSH_INT32(ptr, format->pixel_type);
	PUSH_INT32(ptr, format->frame_per_second);
	PUSH_INT32(ptr, format->key_frame_interval);
	PUSH_INT8(ptr, format->extend_data_size);
	memcpy(ptr, format->extend_data, format->extend_data_size);

	return frame;
}


sr___audio_format___t* sr___media_frame___to___audio_format(sr___media_frame___t *frame)
{
	uint8_t *ptr = NULL;
	sr___audio_format___t *format = NULL;

	format = (sr___audio_format___t *)srmalloc(sizeof(sr___audio_format___t));
	if (format == NULL){
		log_error(ERRMEMORY);
		return NULL;
	}

	ptr = frame->data;

	POP_INT32(ptr, format->codec_type);
	POP_INT32(ptr, format->bit_rate);
	POP_INT32(ptr, format->channels);
	POP_INT32(ptr, format->sample_rate);
	POP_INT32(ptr, format->sample_size);
	POP_INT32(ptr, format->sample_type);
	POP_INT32(ptr, format->sample_per_frame);
	POP_INT32(ptr, format->reserved);
	POP_INT8(ptr, format->extend_data_size);
	memcpy(format->extend_data, ptr, format->extend_data_size);

	log_debug("audio channels = %d sample rate = %d sps size = %d\n",
		format->channels, format->sample_rate, format->extend_data_size);

	return format;
}


sr___video_format___t* sr___media_frame___to___video_format(sr___media_frame___t *frame)
{
	uint8_t *ptr = NULL;
	sr___video_format___t *format = NULL;

	format = (sr___video_format___t *)srmalloc(sizeof(sr___video_format___t));
	if (format == NULL){
		log_error(ERRMEMORY);
		return NULL;
	}

	ptr = frame->data;

	POP_INT32(ptr, format->codec_type);
	POP_INT32(ptr, format->bit_rate);
	POP_INT32(ptr, format->width);
	POP_INT32(ptr, format->height);
	POP_INT32(ptr, format->pixel_size);
	POP_INT32(ptr, format->pixel_type);
	POP_INT32(ptr, format->frame_per_second);
	POP_INT32(ptr, format->key_frame_interval);
	POP_INT8(ptr, format->extend_data_size);
	memcpy(format->extend_data, ptr, format->extend_data_size);

	log_debug("video size = %dx%d sps size = %d\n",
			  format->width, format->height, format->extend_data_size);

	return format;
}


typedef struct sr___synchronous_clock___t {

	bool fixed_audio;
	bool fixed_video;

	int64_t current;
	int64_t duration;
	int64_t audio_duration;
	int64_t first_audio_time;
	int64_t first_video_time;

	int64_t begin_time;
	int64_t updated_every_second;

	sr___event_listener___t *listener;

	sr___atom___t *mutex;

}sr___synchronous_clock___t;


int sr___synchronous_clock___create(sr___event_listener___t *listener, sr___synchronous_clock___t **pp_sync_clock)
{
	int result = 0;
	sr___synchronous_clock___t *clock = (sr___synchronous_clock___t *)srmalloc(sizeof(sr___synchronous_clock___t));

	if (clock == NULL){
		log_error(ERRMEMORY);
		return ERRMEMORY;
	}

	clock->listener = listener;

	if ((result = sr___atom___create(&(clock->mutex))) != 0){
		sr___synchronous_clock___release(&(clock));
		log_error(result);
		return result;
	}

	sr___synchronous_clock___reboot(clock);

	*pp_sync_clock = clock;

	return 0;
}


void sr___synchronous_clock___release(sr___synchronous_clock___t **pp_sync_clock)
{
	if (pp_sync_clock && *pp_sync_clock){
		sr___synchronous_clock___t *clock = *pp_sync_clock;
		*pp_sync_clock = NULL;
		sr___atom___release(&(clock->mutex));
		srfree(clock);
	}
}


void sr___synchronous_clock___reboot(sr___synchronous_clock___t *sync_clock)
{
	if (sync_clock){
		sr___atom___lock(sync_clock->mutex);
		sync_clock->fixed_audio = false;
		sync_clock->fixed_video = false;
		sync_clock->current = 0;
		sync_clock->duration = 0;
		sync_clock->audio_duration = 0;
		sync_clock->first_audio_time = 0;
		sync_clock->first_video_time = 0;
		sync_clock->begin_time = 0;
		sync_clock->updated_every_second = 0;
		sr___atom___unlock(sync_clock->mutex);
	}
}


int64_t sr___synchronous_clock___update_audio_time(sr___synchronous_clock___t *sync_clock, int64_t microsecond)
{
	if (sync_clock == NULL){
		log_error(ERRPARAM);
		return 0;
	}

	sr___atom___lock(sync_clock->mutex);

	if (!sync_clock->fixed_audio){
		sync_clock->fixed_audio = true;
		sync_clock->first_audio_time = microsecond;
		sync_clock->audio_duration = microsecond;
		sync_clock->current = microsecond;
		if (sync_clock->begin_time == 0){
			sync_clock->begin_time = sr___start_timing();
			sync_clock->begin_time -= sync_clock->first_audio_time;
//			sr___event_listener___push_progress(sync_clock->listener, sync_clock->first_audio_time);
		}else{
			if (sync_clock->first_audio_time > sync_clock->first_video_time){
				sync_clock->begin_time -= (sync_clock->first_audio_time - sync_clock->first_video_time);
			}
		}
	}else{
		sync_clock->audio_duration = microsecond;
		sync_clock->current = sr___complete_timing(sync_clock->begin_time);
		sync_clock->updated_every_second += (sync_clock->current - sync_clock->duration);
		if (sync_clock->updated_every_second >= 1000000){
			sync_clock->updated_every_second -= 1000000;
			sr___event_listener___push_progress(sync_clock->listener, sync_clock->audio_duration);
		}
	}

	sync_clock->duration = sync_clock->current;

	microsecond = sync_clock->audio_duration - sync_clock->duration;

	sr___atom___unlock(sync_clock->mutex);

	return microsecond;
}


int64_t sr___synchronous_clock___update_video_time(sr___synchronous_clock___t *sync_clock, int64_t microsecond)
{
	if (sync_clock == NULL){
		log_error(ERRPARAM);
		return 0;
	}

	sr___atom___lock(sync_clock->mutex);

	if (!sync_clock->fixed_video){
		sync_clock->fixed_video = true;
		sync_clock->first_video_time = microsecond;
		sync_clock->current = microsecond;

		if (sync_clock->begin_time == 0){
			sync_clock->begin_time = sr___start_timing();
			sync_clock->begin_time -= sync_clock->first_video_time;
//			sr___event_listener___push_progress(sync_clock->listener, sync_clock->first_video_time);
		}else{
			if (sync_clock->first_audio_time < sync_clock->first_video_time){
				sync_clock->begin_time -= (sync_clock->first_video_time - sync_clock->first_audio_time);
			}
		}

	}else{
		sync_clock->current = sr___complete_timing(sync_clock->begin_time);
	}

	if (sync_clock->audio_duration != 0){
		microsecond -= sync_clock->audio_duration;
	}else{
		sync_clock->updated_every_second += (sync_clock->current - sync_clock->duration);
		if (sync_clock->updated_every_second >= 1000000){
			sync_clock->updated_every_second -= 1000000;
			sr___event_listener___push_progress(sync_clock->listener, sync_clock->current);
		}
		sync_clock->duration = sync_clock->current;
		microsecond -= sync_clock->duration;
	}

	sr___atom___unlock(sync_clock->mutex);

	return microsecond;
}
