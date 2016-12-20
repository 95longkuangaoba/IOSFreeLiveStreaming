#include "self-reliance-media.h"


#include "self-reliance-media-io.h"


#include <pthread.h>



typedef struct sr___demux___t
{
	char *url;

	int64_t seek_time;

	bool running;
	bool seeking;
	bool audio_running;
	bool video_running;
	bool audio_device_ready;
	bool video_device_ready;

	pthread_t audio_tid;
	pthread_t video_tid;
	pthread_t demux_tid;

	sr___queue___t *audio_queue;
	sr___queue___t *video_queue;

	sr___demux_callback___t *cb;
	sr___event_listener___t *listener;
	sr___synchronous_clock___t *sync_lock;

	sr___media_io___t *io;

}sr___demux___t;



static void* audio_loop(void *p)
{
	int result = 0;
	int64_t delay = 0;
	int64_t begin_time = 0;
	int64_t used_time = 0;

	sr___demux___t *demux = (sr___demux___t*)p;

	sr___node___t *node = NULL;
	sr___media_frame___t *frame = NULL;

	while (ISTRUE(demux->running)){

		begin_time = sr___start_timing();

		while((result = sr___queue___pop_first(demux->audio_queue, &node)) == ERRTRYAGAIN){
			if (ISFALSE(demux->running) || ISFALSE(demux->audio_running)){
				result = ERRTERMINATION;
				log_error(ERRTERMINATION);
				goto AUDIO_EXIT;
			}
			nanosleep((const struct timespec[]){{0, 1000L}}, NULL);
		}

		if (result != 0){
			log_error(result);
			goto AUDIO_EXIT;
		}

		frame = (sr___media_frame___t *)(node->content);
		if (frame->slice.type.media != SR___Media_Type___Audio){
			sr___media_frame___release(&frame);
			continue;
		}

		result = demux->cb->demux_audio(demux->cb, frame);
		if (result != 0){
//			sr___media_frame___release(&frame);
			log_error(result);
			goto AUDIO_EXIT;
		}


		if (frame->slice.type.payload == SR___Payload_Type___Frame){

			used_time = sr___complete_timing(begin_time);
			if (delay > 0){
				nanosleep((const struct timespec[]){{0, delay * 1000L}}, NULL);
			}
			delay = sr___synchronous_clock___update_audio_time(demux->sync_lock, frame->slice.timestamp * 1000);
			delay -= used_time;

		}else{

			SETTRUE(demux->audio_device_ready);

			while (ISFALSE(demux->video_device_ready) && ISTRUE(demux->audio_running)){
//				log_debug("audio delay time ============ %lld  %lld\n", frame->slice.timestamp, delay);
				nanosleep((const struct timespec[]){{0, 1000L}}, NULL);
			}

		}


		sr___media_frame___release(&frame);
	}



	AUDIO_EXIT:


	sr___media_frame___release(&frame);


	return NULL;
}


static void* video_loop(void *p)
{
	int result = 0;
	int64_t delay = 0;
	int64_t begin_time = 0;
	int64_t used_time = 0;

	sr___demux___t *demux = (sr___demux___t*)p;

	sr___node___t *node = NULL;
	sr___media_frame___t *frame = NULL;

	while (ISTRUE(demux->running)){

		begin_time = sr___start_timing();

		while((result = sr___queue___pop_first(demux->video_queue, &node)) == ERRTRYAGAIN){
			if (ISFALSE(demux->running) || ISFALSE(demux->video_running)){
				result = ERRTERMINATION;
				log_error(ERRTERMINATION);
				goto VIDEO_EXIT;
			}
			nanosleep((const struct timespec[]){{0, 1000L}}, NULL);
		}

		if (result != 0){
			log_error(result);
			goto VIDEO_EXIT;
		}


		frame = (sr___media_frame___t *)(node->content);
		if (frame->slice.type.media != SR___Media_Type___Video){
			sr___media_frame___release(&frame);
			continue;
		}


		result = demux->cb->demux_video(demux->cb, frame);
		if (result != 0){
//			sr___media_frame___release(&frame);
			log_error(result);
			goto VIDEO_EXIT;
		}


		if (frame->slice.type.payload == SR___Payload_Type___Frame){

			used_time = sr___complete_timing(begin_time);

			if (delay > 0){
				nanosleep((const struct timespec[]){{0, delay * 1000L}}, NULL);
			}

			delay = sr___synchronous_clock___update_video_time(demux->sync_lock, frame->slice.timestamp * 1000);
			delay -= used_time;

		}else{

			SETTRUE(demux->video_device_ready);

			while (ISFALSE(demux->audio_device_ready) && ISTRUE(demux->video_running)){
//				log_debug("video delay time ======= %lld  %lld\n", frame->slice.timestamp, delay);
				nanosleep((const struct timespec[]){{0, 1000L}}, NULL);
			}
		}


		sr___media_frame___release(&frame);
	}



	VIDEO_EXIT:


	sr___media_frame___release(&frame);


	return NULL;
}


static void* demux_loop(void *p)
{
	log_debug("demux_loop enter\n");

	int result = 0;

	sr___media_frame___t *frame = NULL;
	sr___demux___t *demux = (sr___demux___t*)p;


	result = demux->io->open(demux->io, demux->url, MEDIA_IO_RDONLY);
    if (result != 0){
    	log_error(result);
    	goto DEMUX_EXIT;
    }


	if (demux->io->get_duration){
		sr___media_event___t event = {.type = EVENT_DURATION, .i64 = demux->io->get_duration(demux->io)};
		sr___event_listener___push_event(demux->listener, event);
	}


    result = sr___queue___create(64, &(demux->audio_queue));
    if (result != 0){
    	log_error(result);
    	goto DEMUX_EXIT;
    }

    result = sr___queue___create(32, &(demux->video_queue));
    if (result != 0){
    	log_error(result);
    	goto DEMUX_EXIT;
    }

    result = sr___synchronous_clock___create(demux->listener, &(demux->sync_lock));
    if (result != 0){
    	log_error(result);
    	goto DEMUX_EXIT;
    }


    if (demux->cb->demux_audio != NULL){
		demux->audio_running = true;
        result = pthread_create(&(demux->audio_tid), NULL, audio_loop, demux);
        if (result != 0){
        	log_error(ERRSYSCALL);
        	goto DEMUX_EXIT;
        }
    }

    if (demux->cb->demux_video != NULL){
		demux->video_running = true;
        result = pthread_create(&(demux->video_tid), NULL, video_loop, demux);
        if (result != 0){
        	log_error(ERRSYSCALL);
        	goto DEMUX_EXIT;
        }
    }


	sr___event_listener___push_state(demux->listener, EVENT_PLAYING);


	TO_SEEK:


    while (ISTRUE(demux->running)){

		if (SETFALSE(demux->seeking)){
			if (demux->io->seek != NULL){
				sr___queue___clean(demux->audio_queue);
				sr___queue___clean(demux->video_queue);
				demux->io->seek(demux->io, demux->seek_time);
				sr___synchronous_clock___reboot(demux->sync_lock);
				sr___event_listener___push_state(demux->listener, EVENT_PLAYING);
			}
		}

		if ((result = demux->io->read(demux->io, &frame)) != 0){
			log_error(result);
			goto DEMUX_EXIT;
		}

    	if (frame->slice.type.media == SR___Media_Type___Audio){
    		while ((result = sr___queue___push_end(demux->audio_queue, &(frame->node))) == ERRTRYAGAIN){
    			if (ISFALSE(demux->running)){
					sr___media_frame___release(&frame);
    	    		goto DEMUX_EXIT;
    			}
    			nanosleep((const struct timespec[]){{0, 1000L}}, NULL);
    		}
    		if (result != 0){
	    		log_error(result);
	    		goto DEMUX_EXIT;
    		}
    	}else if (frame->slice.type.media == SR___Media_Type___Video){
    		while ((result = sr___queue___push_end(demux->video_queue, &(frame->node))) == ERRTRYAGAIN){
    			if (ISFALSE(demux->running)){
					sr___media_frame___release(&frame);
    	    		goto DEMUX_EXIT;
    			}
    			nanosleep((const struct timespec[]){{0, 1000L}}, NULL);
    		}
    		if (result != 0){
	    		log_error(result);
	    		goto DEMUX_EXIT;
    		}
    	}
    }


	if (ISTRUE(demux->seeking)){
		goto TO_SEEK;
	}


    DEMUX_EXIT:


	SETFALSE(demux->audio_running);
	SETFALSE(demux->video_running);


	if (demux->video_tid != 0){
		pthread_join(demux->video_tid, NULL);
	}

	if (demux->audio_tid != 0){
		pthread_join(demux->audio_tid, NULL);
	}

	sr___queue___release(&(demux->audio_queue));
	sr___queue___release(&(demux->video_queue));
	sr___synchronous_clock___release(&(demux->sync_lock));

	if (demux->io->close != NULL){
		demux->io->close(demux->io);
	}

	sr___media_event___t event = {.type = EVENT_ENDING, .i32 = result};
	sr___event_listener___push_event(demux->listener, event);

	log_debug("demux_loop exit\n");

	return NULL;
}


int sr___demux___create(const char *url,
		sr___demux_callback___t *callback,
		sr___event_listener___t *listener,
		sr___demux___t **pp_demux)
{
	log_debug("sr___demux___create enter\n");

	int result = 0;
	sr___demux___t *demux = NULL;

	if (url == NULL || callback == NULL
			|| listener == NULL
			|| pp_demux == NULL
			|| (callback->demux_audio == NULL && callback->demux_video == NULL)){
		log_error(ERRPARAM);
		return ERRPARAM;
	}

	if ((demux = (sr___demux___t *)srmalloc(sizeof(sr___demux___t))) == NULL){
		log_error(ERRMEMORY);
		return ERRMEMORY;
	}

	demux->cb = callback;
	demux->listener = listener;
	demux->audio_device_ready = (callback->demux_audio == NULL);
	demux->video_device_ready = (callback->demux_video == NULL);

	if ((demux->url = (char *)srmalloc(strlen(url) + 1)) == NULL){
		srfree(demux);
		log_error(ERRMEMORY);
		return ERRMEMORY;
	}
    memcpy(demux->url, url, strlen(url));
    demux->url[strlen(url)] = '\0';


    result = sr___media_io___create(&(demux->io));
    if (result != 0){
    	sr___demux___release(&demux);
    	log_error(result);
    	return result;
    }


    demux->running = true;
    result = pthread_create(&(demux->demux_tid), NULL, demux_loop, demux);
    if (result != 0){
    	sr___demux___release(&demux);
    	log_error(ERRSYSCALL);
    	return ERRSYSCALL;
    }

	*pp_demux = demux;

	log_debug("sr___demux___create exit\n");

	return 0;
}


void sr___demux___release(sr___demux___t **pp_demux)
{
	log_debug("sr___demux___release enter\n");
	if (pp_demux && *pp_demux){
		sr___demux___t *demux = *pp_demux;
		*pp_demux = NULL;
		SETFALSE(demux->running);
		if (demux->demux_tid != 0){
			pthread_join(demux->demux_tid, NULL);
		}
		sr___media_io___release(&(demux->io));
		srfree(demux->url);
		srfree(demux);
	}
	log_debug("sr___demux___release exit\n");
}


void sr___demux___stop(sr___demux___t *demux)
{
	log_debug("sr___demux___stop enter\n");
	if (demux){
		SETFALSE(demux->running);
	}
	log_debug("sr___demux___stop exit\n");
}


void sr___demux___seek(sr___demux___t *demux, int64_t seek_time)
{
	log_debug("sr___demux___seek enter\n");
	if (demux && demux->io->seek != NULL){
		demux->seek_time = seek_time;
		SETTRUE(demux->seeking);
	}
	log_debug("sr___demux___seek exit\n");
}