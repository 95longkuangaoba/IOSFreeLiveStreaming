#include "self-reliance-media.h"


#include "self-reliance-media-io.h"


#include <pthread.h>



typedef struct sr___muxer___t
{
	char *url;

	bool running;

	bool with_audio;

	bool with_video;

	pthread_t tid;

	sr___queue___t *queue;

	sr___event_listener___t *listener;

	sr___audio_format___t *audio_format;

	sr___video_format___t *video_format;

	sr___media_io___t *io;

}sr___muxer___t;



static void* muxer_loop(void *p)
{
	int result = 0;

	bool send_format = false;

	int64_t timestamp = 0;
	int64_t audio_start_time = 0;
	int64_t video_start_time = 0;

	sr___node___t *node = NULL;
	sr___media_frame___t *frame = NULL;

	sr___muxer___t *muxer = (sr___muxer___t *)p;


	result = muxer->io->open(muxer->io, muxer->url, MEDIA_IO_WRONLY);
	if (result != 0){
		log_error(result);
		goto LOOP_EXIT;
	}


	sr___event_listener___push_state(muxer->listener, EVENT_OPENING);


	while(ISTRUE(muxer->running)){

		while((result = sr___queue___pop_first(muxer->queue, &node)) == ERRTRYAGAIN){
			if (ISFALSE(muxer->running)){
				result = ERRTERMINATION;
				log_error(ERRTERMINATION);
				goto LOOP_EXIT;
			}
			nanosleep((const struct timespec[]){{0, 1000L}}, NULL);
		}

		if (result != 0){
			log_error(result);
			goto LOOP_EXIT;
		}

		frame = (sr___media_frame___t *)(node->content);

		if (frame->slice.type.media == SR___Media_Type___Audio && frame->slice.type.payload == SR___Payload_Type___Format){
			muxer->audio_format = sr___media_frame___to___audio_format(frame);
			result = muxer->io->write(muxer->io, frame);
			if (result != 0){
				log_error(result);
				goto LOOP_EXIT;
			}
			if (!muxer->with_video || muxer->video_format != NULL){
				sr___queue___clean(muxer->queue);
				send_format = true;
			}
		}else if (frame->slice.type.media == SR___Media_Type___Video && frame->slice.type.payload == SR___Payload_Type___Format){
			muxer->video_format = sr___media_frame___to___video_format(frame);
			result = muxer->io->write(muxer->io, frame);
			if (result != 0){
				log_error(result);
				goto LOOP_EXIT;
			}
			if (!muxer->with_audio || muxer->audio_format != NULL){
				sr___queue___clean(muxer->queue);
				send_format = true;
			}
		}


		if (send_format && frame->slice.type.payload == SR___Payload_Type___Frame){
			if (frame->slice.type.media == SR___Media_Type___Video && frame->slice.type.extend == SR___Video_Extend_Type___KeyFrame){
				sr___queue___push_head(muxer->queue, &(frame->node));
				break;
			}
		}


		sr___media_frame___release(&frame);

	}


	sr___event_listener___push_state(muxer->listener, EVENT_PLAYING);



	while (ISTRUE(muxer->running)){

		while((result = sr___queue___pop_first(muxer->queue, &node)) == ERRTRYAGAIN){
			if (ISFALSE(muxer->running)){
				result = ERRTERMINATION;
				log_error(ERRTERMINATION);
				goto LOOP_EXIT;
			}
			nanosleep((const struct timespec[]){{0, 1000L}}, NULL);
		}

		if (result != 0){
			log_error(result);
			goto LOOP_EXIT;
		}

		frame = (sr___media_frame___t *)(node->content);

		if (frame->slice.type.media == SR___Media_Type___Audio){
			if (audio_start_time == 0){
				audio_start_time = frame->slice.timestamp;
				timestamp = 0;
			}else{
				timestamp = frame->slice.timestamp - audio_start_time;
			}
		}else if (frame->slice.type.media == SR___Media_Type___Video){
			if (video_start_time == 0){
				video_start_time = frame->slice.timestamp;
				timestamp = 0;
			}else{
				timestamp = frame->slice.timestamp - video_start_time;
			}
		}

		frame->slice.timestamp = timestamp;

		result = muxer->io->write(muxer->io, frame);
		if (result != 0){
			log_error(result);
			goto LOOP_EXIT;
		}

		sr___media_frame___release(&frame);
	}



	LOOP_EXIT:


	if (muxer->io->close){
		muxer->io->close(muxer->io);
	}

	sr___media_event___t event = {.type = EVENT_ENDING, .i32 = result};
	sr___event_listener___push_event(muxer->listener, event);

	log_debug("muxer_loop exit\n");


	return NULL;
}


int sr___muxer___create(const char *url,
		bool with_audio,
		bool with_video,
		sr___event_listener___t *listener,
		sr___muxer___t **pp_muxer)
{
	log_debug("sr___muxer___create enter\n");

	int result = 0;
	sr___muxer___t *muxer = NULL;

	if (url == NULL || listener == NULL || pp_muxer == NULL || (!with_audio && !with_video)){
		log_error(ERRPARAM);
		return ERRPARAM;
	}

	if ((muxer = (sr___muxer___t*)srmalloc(sizeof(sr___muxer___t))) == NULL){
		log_error(ERRMEMORY);
		return ERRMEMORY;
	}

	muxer->listener = listener;
	muxer->with_audio = with_audio;
	muxer->with_video = with_video;

	if ((muxer->url = (char *)srmalloc(strlen(url) + 1)) == NULL){
		srfree(muxer);
		log_error(ERRMEMORY);
		return ERRMEMORY;
	}
    memcpy(muxer->url, url, strlen(url));
    muxer->url[strlen(url)] = '\0';
	log_debug("open url %s\n", url);

	if ((result = sr___queue___create(1024, &(muxer->queue))) != 0){
		srfree(muxer->url);
		srfree(muxer);
		log_error(result);
		return result;
	}

	result = sr___media_io___create(&(muxer->io));
	if (result != 0){
		sr___queue___release(&(muxer->queue));
		srfree(muxer->url);
		srfree(muxer);
		log_error(result);
		return result;
	}

	muxer->running = true;
	result = pthread_create(&(muxer->tid), NULL, muxer_loop, muxer);
	if (result != 0){
		sr___media_io___release(&(muxer->io));
		sr___queue___release(&(muxer->queue));
		srfree(muxer->url);
		srfree(muxer);
		log_error(ERRSYSCALL);
		return ERRSYSCALL;
	}

	*pp_muxer = muxer;

	log_debug("sr___muxer___create exit\n");

	return 0;
}


void sr___muxer___release(sr___muxer___t **pp_muxer)
{
	log_debug("sr___muxer___release enter\n");
	if (pp_muxer && *pp_muxer){
		sr___muxer___t *muxer = *pp_muxer;
		*pp_muxer = NULL;
		SETFALSE(muxer->running);
		if (muxer->tid != 0){
			pthread_join(muxer->tid, NULL);
		}
		sr___queue___release(&(muxer->queue));
		sr___media_io___release(&(muxer->io));
		srfree(muxer->audio_format);
		srfree(muxer->video_format);
		srfree(muxer->url);
		srfree(muxer);
	}
	log_debug("sr___muxer___release exit\n");
}


void sr___muxer___stop(sr___muxer___t *muxer)
{
	log_debug("sr___muxer___stop enter\n");
	if (muxer != NULL){
		if (muxer->io && muxer->io->stop){
			muxer->io->stop(muxer->io);
		}
		SETFALSE(muxer->running);
	}
	log_debug("sr___muxer___stop exit\n");
}


int sr___muxer___frame(sr___muxer___t *muxer, sr___media_frame___t *frame)
{
	int result = 0;

	if (muxer == NULL || frame == NULL){
		log_error(ERRPARAM);
		return ERRPARAM;
	}

	while((result = sr___queue___push_end(muxer->queue, &(frame->node))) == ERRTRYAGAIN){
		if (ISFALSE(muxer->running)){
			sr___media_frame___release(&frame);
			result = ERRTERMINATION;
			break;
		}
		nanosleep((const struct timespec[]){{0, 1000L}}, NULL);
	}

	return result;
}
