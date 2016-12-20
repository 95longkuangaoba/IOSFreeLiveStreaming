/*
 * file.c
 *
 *  Created on: 2016年9月24日
 *      Author: kly
 */




#include "file.h"
#include "../self-reliance-media.h"
#include "../self-reliance-media-io.h"


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>




typedef struct frame_position_t
{
	int64_t position;
	int64_t timestamp;
}frame_position_t;



typedef struct frame_position_node_t
{
	sr___node___t node;
	frame_position_t frame_position;
}frame_position_node_t;



typedef struct file_io_t
{
	int fd;
	int mode;
	uint8_t audio_frame_index;
	uint8_t video_frame_index;
	int frame_head_size;
	uint8_t *frame_head;
	int64_t duration;
	int64_t position;
	sr___media_io___t *io;
	int frame_position_count;
	frame_position_t *frame_position_array;
	sr___queue___t *frame_position_queue;
}file_io_t;




static void sr___node___release(sr___node___t *node)
{
	if (node && node->content){
		frame_position_node_t *frame_node = (frame_position_node_t *)(node->content);
		srfree(frame_node);
	}
}


static int64_t get_duration(sr___media_io___t *io)
{
	file_io_t *file_io = NULL;

	if (io != NULL || io->protocol != NULL){
		file_io = (file_io_t *)(io->protocol);
		return file_io->duration;
	}

	return 0;
}


static void file_seek(sr___media_io___t *io, int64_t seek_time)
{
	file_io_t *file_io = NULL;

	if (io != NULL || io->protocol != NULL){

		file_io = (file_io_t *)(io->protocol);

		float position = 0.0f;
		int frame_index = 0;
		frame_position_t *frame_position = NULL;

		position = (float)seek_time / file_io->duration;
		frame_index = (int)(file_io->frame_position_count * position);
		frame_position = &(file_io->frame_position_array[frame_index]);
		lseek(file_io->fd, frame_position->position, SEEK_SET);
	}
}


static int file_write(sr___media_io___t *io, sr___media_frame___t *frame)
{
	int result = 0;
	uint8_t *ptr = NULL;
	uint8_t frame_type = 0;
	uint8_t frame_index = 0;
	file_io_t *file_io = NULL;

	if (io == NULL || io->protocol == NULL || frame == NULL){
		log_error(ERRPARAM);
		return ERRPARAM;
	}

	file_io = (file_io_t *)(io->protocol);

	ptr = &(file_io->frame_head[0]);

	if (frame->slice.type.media == SR___Media_Type___Audio){
		frame_index = ++file_io->audio_frame_index;
	}else if(frame->slice.type.media == SR___Media_Type___Video){
		frame_index = ++file_io->video_frame_index;
	}

	frame_type = frame->slice.type.media << 5
			| frame->slice.type.payload << 4
			| frame->slice.type.extend;
	frame->slice.slice_size = (uint16_t)(frame->size & 0xFFFF);
	frame->slice.slice_index = (uint16_t)((frame->size >> 16) & 0xFFFF);
	frame->slice.slice_total = 1;

	file_io->duration = frame->slice.timestamp * 1000;
	if (frame->slice.type.media == SR___Media_Type___Video && frame->slice.type.extend == SR___Video_Extend_Type___KeyFrame){
		frame_position_node_t *pos_node = srmalloc(sizeof(frame_position_node_t));
		pos_node->node.content = pos_node;
		pos_node->node.release = sr___node___release;
		pos_node->frame_position.position = lseek(file_io->fd, 0, SEEK_CUR);
		pos_node->frame_position.timestamp = frame->slice.timestamp;
		sr___queue___push_end(file_io->frame_position_queue, &(pos_node->node));
	}

	PUSH_INT8(ptr, frame_type);
	PUSH_INT8(ptr, frame_index);
	PUSH_INT16(ptr, frame->slice.slice_total);
	PUSH_INT16(ptr, frame->slice.slice_index);
	PUSH_INT16(ptr, frame->slice.slice_size);
	PUSH_INT64(ptr, frame->slice.timestamp);


	for (size_t write_size = 0; write_size < file_io->frame_head_size; write_size += result){
		result = write(file_io->fd, file_io->frame_head + write_size, file_io->frame_head_size - write_size);
		if (result < 0){
			log_error(ERRSYSCALL);
			return ERRSYSCALL;
		}
	}

	for (int write_size = 0; write_size < frame->size; write_size += result){
		result = write(file_io->fd, frame->data + write_size, frame->size - write_size);
		if (result < 0){
			log_error(ERRSYSCALL);
			return ERRSYSCALL;
		}
	}

	return 0;
}


static int file_read(sr___media_io___t *io, sr___media_frame___t **pp_frame)
{
	int result = 0;
	uint8_t *ptr = NULL;
	uint8_t frame_type = 0;
	uint8_t frame_index = 0;
	file_io_t *file_io = NULL;
	sr___media_frame___t *frame = NULL;

	if (io == NULL || io->protocol == NULL || pp_frame == NULL){
		log_error(ERRPARAM);
		return ERRPARAM;
	}

	file_io = (file_io_t *)(io->protocol);

	result = sr___media_frame___create(&frame);
	if (result != 0){
		log_error(result);
		return result;
	}

	for (int read_size = 0; read_size < file_io->frame_head_size; read_size += result){
		result = read(file_io->fd, file_io->frame_head + read_size, file_io->frame_head_size - read_size);
		if (result <= 0){
			sr___media_frame___release(&frame);
			if (result == 0){
				return ERRTERMINATION;
			}
			log_error(ERRSYSCALL);
			return ERRSYSCALL;
		}
	}


	ptr = &(file_io->frame_head[0]);

	POP_INT8(ptr, frame_type);
	POP_INT8(ptr, frame_index);
	POP_INT16(ptr, frame->slice.slice_total);
	POP_INT16(ptr, frame->slice.slice_index);
	POP_INT16(ptr, frame->slice.slice_size);
	POP_INT64(ptr, frame->slice.timestamp);


	frame->slice.type.media = frame_type >> 5;
	frame->slice.type.payload = (frame_type >> 4) & 0x01;
	frame->slice.type.extend = frame_type & 0x0F;


	frame->size = frame->slice.slice_index << 16 | frame->slice.slice_size;
	frame->data = (uint8_t *)srmalloc(frame->size);
	if (frame->data == NULL){
		sr___media_frame___release(&frame);
		log_error(ERRMEMORY);
		return ERRMEMORY;
	}

	for (int read_size = 0; read_size < frame->size; read_size += result){
		result = read(file_io->fd, frame->data + read_size, frame->size - read_size);
		if (result <= 0){
			sr___media_frame___release(&frame);
			if (result == 0){
				return ERRTERMINATION;
			}
			log_error(ERRSYSCALL);
			return ERRSYSCALL;
		}
	}

	*pp_frame = frame;

	return 0;
}


static int write_last_frame(file_io_t *file_io)
{
	int result = 0;
	int frame_size = 0;
	int frame_position_count = 0;
	int64_t position;
	uint8_t frame_type = 0;
	uint8_t *ptr = NULL;
	sr___media_frame___t *frame = NULL;

	frame_position_count = sr___queue___popable(file_io->frame_position_queue);
	if (frame_position_count == 0){
		return 0;
	}

	position = lseek(file_io->fd, 0, SEEK_CUR);

	ptr = &(file_io->frame_head[0]);
	PUSH_INT64(ptr, position);

	lseek(file_io->fd, 3, SEEK_SET);
	write(file_io->fd, file_io->frame_head, sizeof(position));
	lseek(file_io->fd, position, SEEK_SET);

	result = sr___media_frame___create(&frame);

	frame->size = frame_position_count * sizeof(frame_position_t);
	frame->data = (uint8_t *)srmalloc(frame->size);

	frame->slice.type.media = SR___Media_Type___Ended;
	frame->slice.type.payload = SR___Payload_Type___Frame;
	frame->slice.type.extend = SR___Video_Extend_Type___KeyFrame;

	frame->slice.frame_index = 0;
	frame->slice.slice_total = 1;
	frame->slice.slice_size = frame->size & 0xFFFF;
	frame->slice.slice_index = (frame->size >> 16) & 0xFFFF;
	frame->slice.timestamp = file_io->duration;


	ptr = &(file_io->frame_head[0]);
	PUSH_INT8(ptr, frame_type);
	PUSH_INT8(ptr, frame->slice.frame_index);
	PUSH_INT16(ptr, frame->slice.slice_total);
	PUSH_INT16(ptr, frame->slice.slice_index);
	PUSH_INT16(ptr, frame->slice.slice_size);
	PUSH_INT64(ptr, frame->slice.timestamp);


	ptr = &(frame->data[0]);
	sr___node___t *node = NULL;
	frame_position_node_t *frame_node = NULL;
	for (int i = 0; i < frame_position_count; ++i){
		result = sr___queue___pop_first(file_io->frame_position_queue, &node);
		frame_node = (frame_position_node_t *)(node->content);
		PUSH_INT64(ptr, frame_node->frame_position.position);
		PUSH_INT64(ptr, frame_node->frame_position.timestamp);
		srfree(frame_node);
	}

	for (int write_size = 0; write_size < file_io->frame_head_size; write_size += result){
		result = write(file_io->fd, file_io->frame_head + write_size, file_io->frame_head_size - write_size);
		if (result < 0){
			log_error(ERRSYSCALL);
			return ERRSYSCALL;
		}
	}

	for (int write_size = 0; write_size < frame->size; write_size += result){
		result = write(file_io->fd, frame->data + write_size, frame->size - write_size);
		if (result < 0){
			log_error(ERRSYSCALL);
			return ERRSYSCALL;
		}
	}

	sr___media_frame___release(&frame);

	return result;
}


static int read_last_frame(file_io_t *file_io)
{
	int result = 0;
	int frame_position_count = 0;
	uint8_t *ptr = NULL;
	frame_position_t *frame_node = NULL;
	sr___media_frame___t *frame = NULL;

	int64_t position = lseek(file_io->fd, 0, SEEK_CUR);
	lseek(file_io->fd, file_io->position, SEEK_SET);

	result = file_read(file_io->io, &frame);
	if (result != 0){
		log_error(result);
		return result;
	}

	lseek(file_io->fd, position, SEEK_SET);

	file_io->duration = frame->slice.timestamp;
	frame_position_count = frame->size / sizeof(frame_position_t);

	file_io->frame_position_array = (frame_position_t *)srmalloc(sizeof(frame_position_t) * frame_position_count);
	frame_position_t *temp = file_io->frame_position_array;

	ptr = &(frame->data[0]);
	for (int i = 0; i < frame_position_count; ++i){
		POP_INT64(ptr, (temp + i)->position);
		POP_INT64(ptr, (temp + i)->timestamp);
	}

	sr___media_frame___release(&frame);

	file_io->frame_position_count = frame_position_count;

	return result;
}


static void file_close(sr___media_io___t *io)
{
	if (io && io->protocol){
		file_io_t *file_io = (file_io_t *)(io->protocol);
		io->protocol = NULL;
		if (file_io->mode == MEDIA_IO_WRONLY){
			write_last_frame(file_io);
		}
		if (file_io->fd > 0){
			close(file_io->fd);
		}
		sr___queue___release(&(file_io->frame_position_queue));
		srfree(file_io->frame_position_array);
		srfree(file_io->frame_head);
		srfree(file_io);
	}
}


int file_open(sr___media_io___t *io, const char *url, int mode)
{
	int result = 0;
	char *ptr = NULL;
	char head[] = {'F', 'L', 'S', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'};
	int head_size = sizeof(head);
	file_io_t *file_io = NULL;

	if (io == NULL || url == NULL
			|| (mode != MEDIA_IO_RDONLY && mode != MEDIA_IO_WRONLY)){
		log_error(ERRPARAM);
		return ERRPARAM;
	}

	log_debug("open file %s\n", url);

	file_io = (file_io_t *)srmalloc(sizeof(file_io_t));
	if (file_io == NULL){
		log_error(ERRMEMORY);
		return ERRMEMORY;
	}

	file_io->io = io;
	file_io->mode = mode;
	io->protocol = file_io;

	file_io->frame_head_size = 16;
	file_io->frame_head = (uint8_t *)srmalloc(file_io->frame_head_size);
	if (file_io->frame_head == NULL){
		log_error(ERRMEMORY);
		result = ERRMEMORY;
		goto ERROR_EXIT;
	}

	result = sr___queue___create(INT32_MAX, &(file_io->frame_position_queue));
	if (result != 0){
		log_error(result);
		goto ERROR_EXIT;
	}

	if (mode == MEDIA_IO_WRONLY){
		file_io->fd = open(url, O_WRONLY | O_CREAT | O_TRUNC, 0755);
		if (file_io->fd < 0){
			log_error(ERRSYSCALL);
			result = ERRSYSCALL;
			goto ERROR_EXIT;
		}

		for (int write_size = 0; write_size < head_size; write_size += result){
			result = write(file_io->fd, head + write_size, head_size - write_size);
			if (result < 0){
				log_error(ERRSYSCALL);
				goto ERROR_EXIT;
			}
		}

	}else{

		file_io->fd = open(url, O_RDONLY, 0755);
		if (file_io->fd < 0){
			log_error(ERRSYSCALL);
			result = ERRSYSCALL;
			goto ERROR_EXIT;
		}

		for (size_t read_size = 0; read_size < head_size; read_size += result){
			result = read(file_io->fd, head + read_size, head_size - read_size);
			if (result <= 0){
				if (result == 0){
					result = ERRTERMINATION;
					goto ERROR_EXIT;
				}
				log_error(ERRSYSCALL);
				result = ERRSYSCALL;
				goto ERROR_EXIT;
			}
		}

		if (head[0] != 'F' || head[1] != 'L' || head[2] != 'S'){
			log_error(-1);
			result = -1;
			goto ERROR_EXIT;
		}

		ptr = &(head[3]);
		POP_INT64(ptr, file_io->position);

		result = read_last_frame(file_io);
		if (result != 0){
			log_error(result);
			goto ERROR_EXIT;
		}
	}


	io->write = file_write;
	io->read = file_read;
	io->close = file_close;
	io->seek = file_seek;
	io->get_duration = get_duration;


	return 0;


	ERROR_EXIT:


	if (file_io->fd > 0){
		close(file_io->fd);
	}
	sr___queue___release(&(file_io->frame_position_queue));
	srfree(file_io->frame_head);
	srfree(file_io);

	return result;
}
