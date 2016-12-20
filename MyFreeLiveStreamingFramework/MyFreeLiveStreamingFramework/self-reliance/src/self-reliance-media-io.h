/*
 * self-reliance-media-io.h
 *
 *  Created on: 2016年9月24日
 *      Author: kly
 */

#ifndef SRC_SELF_RELIANCE_MEDIA_IO_H_
#define SRC_SELF_RELIANCE_MEDIA_IO_H_



#include "self-reliance-media.h"


#define	MEDIA_IO_RDONLY		0x01
#define	MEDIA_IO_WRONLY		0x02


typedef struct sr___media_io___t
{
	void *protocol;
	int (*open)(struct sr___media_io___t *io, const char *url, int mode);
	int (*write)(struct sr___media_io___t *io, sr___media_frame___t *frame);
	int (*read)(struct sr___media_io___t *io, sr___media_frame___t **pp_frame);
	void (*seek)(struct sr___media_io___t *io, int64_t time);
	void (*stop)(struct sr___media_io___t *io);
	void (*close)(struct sr___media_io___t *io);
	int64_t (*get_duration)(struct sr___media_io___t *io);
}sr___media_io___t;


int sr___media_io___create(sr___media_io___t **pp_io);
void sr___media_io___release(sr___media_io___t **pp_io);


#endif /* SRC_SELF_RELIANCE_MEDIA_IO_H_ */
