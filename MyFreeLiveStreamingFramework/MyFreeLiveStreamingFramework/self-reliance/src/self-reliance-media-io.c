/*
 * self-reliance-media-io.c
 *
 *  Created on: 2016年9月24日
 *      Author: kly
 */




#include "self-reliance-media-io.h"


#include "io/file.h"


extern int rtmp_open(sr___media_io___t *io, const char *url, int mode);


struct protocol_entry_t{
	const char *name;
	int (*open)(sr___media_io___t *io, const char *url, int mode);
};

static struct protocol_entry_t protocol_entry[] = {
		{"file://", file_open},
		{NULL, NULL}
};


static int sr___media_io___open(sr___media_io___t *io, const char *url, int mode)
{
	if (io == NULL || url == NULL){
		log_error(ERRPARAM);
		return ERRPARAM;
	}

	return rtmp_open(io, url, mode);

//	if (url[0] == '/'){
//		return file_open(io, url, mode);
//	}
//
//	struct protocol_entry_t *entry = &protocol_entry[0];
//
//	while (entry->name != NULL && entry->open != NULL){
//		if (strncmp(entry->name, url, strlen(entry->name)) == 0){
//			return entry->open(io, url, mode);
//		}
//		entry ++;
//	}

	return ERRPARAM;
}


int sr___media_io___create(sr___media_io___t **pp_io)
{
	int result = 0;
	sr___media_io___t *io = NULL;

	if (pp_io == NULL){
		log_error(ERRPARAM);
		return ERRPARAM;
	}

	io = (sr___media_io___t *)srmalloc(sizeof(sr___media_io___t));
	if (io == NULL){
		log_error(ERRMEMORY);
		return ERRMEMORY;
	}

	io->open = sr___media_io___open;

	*pp_io = io;

	return result;
}


void sr___media_io___release(sr___media_io___t **pp_io)
{
	if (pp_io && *pp_io){
		sr___media_io___t *io = *pp_io;
		*pp_io = NULL;
		if (io->close != NULL){
			io->close(io);
		}
		srfree(io);
	}
}
