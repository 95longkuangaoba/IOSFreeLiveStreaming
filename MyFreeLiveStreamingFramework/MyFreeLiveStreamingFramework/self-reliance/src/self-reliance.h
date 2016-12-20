#ifndef	___SELF_RELIANCE_H___
#define	___SELF_RELIANCE_H___



#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <errno.h>
#include <string.h>

#define ___TESTTEST_MEMORY___
#define NDEBUG


///////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////


#define	ERRUNKONWN			(-1)
#define	ERRSYSCALL			(-10000)
#define	ERRMEMORY			(-10001)
#define	ERRPARAM			(-10002)
#define	ERRTRYAGAIN			(-10003)
#define	ERRTIMEOUT			(-10004)
#define	ERRTERMINATION		(-10005)

extern const char* sr___error_to_message(int errorcode);
#define	err2msg(e)	sr___error_to_message((e))


///////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////


#define	SR_LOG_DEBUG			0
#define	SR_LOG_INFO				1
#define	SR_LOG_WARNING			2
#define	SR_LOG_ERROR			3


extern void sr___log___initialize(int level, void (*callback) (int level, const char *tag, const char *log));
extern void sr___log___release();
extern void sr___log___debug(int level, const char *file, const char *function, int line, const char *format, ...);
extern void sr___log___info(const char *format, ...);


#define SR_LOGGER_DEBUG(level, format, ...) \
	sr___log___debug(level, __FILE__, __FUNCTION__, __LINE__, format, ##__VA_ARGS__ )

#ifdef NDEBUG
# define log_debug(format, ...)	SR_LOGGER_DEBUG(SR_LOG_DEBUG, format, ##__VA_ARGS__)
#else
# define log_debug(format, ...)	do {} while(0)
#endif

#define log_warning(format, ...)	SR_LOGGER_DEBUG(SR_LOG_WARNING, format, ##__VA_ARGS__)
#define log_error(errorcode)		SR_LOGGER_DEBUG(SR_LOG_ERROR, "%s", err2msg(errorcode))
#define log_info(format, ...)		sr___log___info(format, ##__VA_ARGS__)


///////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////


#define	ISTRUE(x)			__sync_bool_compare_and_swap(&(x), true, true)
#define	ISFALSE(x)			__sync_bool_compare_and_swap(&(x), false, false)
#define	SETTRUE(x)			__sync_bool_compare_and_swap(&(x), false, true)
#define	SETFALSE(x)			__sync_bool_compare_and_swap(&(x), true, false)

#define SR___ATOM___SUB(x, y)		__sync_sub_and_fetch(&(x), (y))
#define SR___ATOM___ADD(x, y)		__sync_add_and_fetch(&(x), (y))

#define SR___ATOM___LOCK(x) \
	while(!SETTRUE(x)) \
		nanosleep((const struct timespec[]){{0, 1L}}, NULL)
#define SR___ATOM___TRYLOCK(x)			SETTRUE(x)
#define SR___ATOM___UNLOCK(x)			SETFALSE(x)


///////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////


extern int sr___memory___initialize(size_t page_size);
extern void sr___memory___release();
extern void sr___memory___flush_cache();
extern void sr___memory___dump();

#ifdef NDEBUG
extern void* sr___memory___alloc(size_t size, const char *file, const char *function, int line);
#else
extern void* sr___memory___alloc(size_t size);
#endif
extern void sr___memory___free(void *p);


#ifdef ___TESTTEST_MEMORY___
# ifdef NDEBUG
#  define srmalloc(x)	sr___memory___alloc((x), __FILE__, __FUNCTION__, __LINE__)
# else //NDEBUG
#  define srmalloc(x)	sr___memory___alloc((x))
# endif //NDEBUG
# define srfree(x) \
	do { \
		if ((x)) sr___memory___free((x)); \
	} while(0)
#else //___TESTTEST_MEMORY___
# define srmalloc(x)	calloc(1, (x))
# define srfree(x) \
	do { \
		if ((x)) free((x)); \
	} while(0)
#endif //___TESTTEST_MEMORY___


///////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////


extern int64_t sr___start_timing();
extern int64_t sr___complete_timing(int64_t start_microsecond);


///////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////


#define PUSH_INT8(p, i) \
	(*p++) = (uint8_t)(i)

#define PUSH_INT16(p, i) \
	(*p++) = (uint16_t)(i) >> 8; \
	(*p++) = (uint16_t)(i) & 0xFF

#define PUSH_INT24(p, i) \
	(*p++) = ((uint32_t)(i) >> 16) & 0xFF; \
	(*p++) = ((uint32_t)(i) >> 8) & 0xFF; \
	(*p++) = (uint32_t)(i) & 0xFF

#define PUSH_INT32(p, i) \
	(*p++) = (uint32_t)(i) >> 24; \
	(*p++) = ((uint32_t)(i) >> 16) & 0xFF; \
	(*p++) = ((uint32_t)(i) >> 8) & 0xFF; \
	(*p++) = (uint32_t)(i) & 0xFF

#define PUSH_INT64(p, i) \
	(*p++) = (uint64_t)(i) >> 56; \
	(*p++) = ((uint64_t)(i) >> 48) & 0xFF; \
	(*p++) = ((uint64_t)(i) >> 40) & 0xFF; \
	(*p++) = ((uint64_t)(i) >> 32) & 0xFF; \
	(*p++) = ((uint64_t)(i) >> 24) & 0xFF; \
	(*p++) = ((uint64_t)(i) >> 16) & 0xFF; \
	(*p++) = ((uint64_t)(i) >> 8) & 0xFF; \
	(*p++) = (uint64_t)(i) & 0xFF

#define POP_INT8(p, i) \
	(i) = (*p); \
	(p)++

#define POP_INT16(p, i) \
	(i) = (*p) << 8; \
	(p)++; \
	(i) |= (*p); \
	(p)++

#define POP_INT24(p, i) \
	(i) = (*p) << 16; \
	(p)++; \
	(i) |= (*p) << 8; \
	(p)++; \
	(i) |= (*p); \
	(p)++

#define POP_INT32(p, i) \
	(i) = (*p) << 24; \
	(p)++; \
	(i) |= (*p) << 16; \
	(p)++; \
	(i) |= (*p) << 8; \
	(p)++; \
	(i) |= (*p); \
	(p)++

#define POP_INT64(p, i) \
	(i) = (int64_t)(*p) << 56; \
	(p)++; \
	(i) |= (int64_t)(*p) << 48; \
	(p)++; \
	(i) |= (int64_t)(*p) << 40; \
	(p)++; \
	(i) |= (int64_t)(*p) << 32; \
	(p)++; \
	(i) |= (int64_t)(*p) << 24; \
	(p)++; \
	(i) |= (int64_t)(*p) << 16; \
	(p)++; \
	(i) |= (int64_t)(*p) << 8; \
	(p)++; \
	(i) |= (int64_t)(*p); \
	(p)++


typedef struct sr___node___t {
	void *content;
	struct sr___node___t *prev;
	struct sr___node___t *next;
	void (*release)(struct sr___node___t*);
} sr___node___t;


typedef struct sr___queue___t sr___queue___t;


extern int sr___queue___create(size_t size, sr___queue___t **pp_queue);
extern void sr___queue___release(sr___queue___t **pp_queue);

extern int sr___queue___push_head(sr___queue___t *queue, sr___node___t *node);
extern int sr___queue___push_end(sr___queue___t *queue, sr___node___t *node);

extern int sr___queue___pop_first(sr___queue___t *queue, sr___node___t **pp_node);
extern int sr___queue___pop_last(sr___queue___t *queue, sr___node___t **pp_node);

extern int sr___queue___get_first(sr___queue___t *queue, sr___node___t **pp_node);
extern int sr___queue___get_last(sr___queue___t *queue, sr___node___t **pp_node);

extern int sr___queue___remove_node(sr___queue___t *queue, sr___node___t *node);

extern int sr___queue___clean(sr___queue___t *queue);
extern int sr___queue___pushable(sr___queue___t *queue);
extern int sr___queue___popable(sr___queue___t *queue);


typedef struct sr___pipe___t sr___pipe___t;

extern int sr___pipe___create(unsigned int size, sr___pipe___t **pp_pipe);
extern void sr___pipe___release(sr___pipe___t **pp_pipe);

extern unsigned int sr___pipe___read(sr___pipe___t *pipe, char *data, unsigned int size);
extern unsigned int sr___pipe___write(sr___pipe___t *pipe, char *data, unsigned int size);

extern unsigned int sr___pipe___writable(sr___pipe___t *pipe);
extern unsigned int sr___pipe___readable(sr___pipe___t *pipe);

extern void sr___pipe___clean(sr___pipe___t *pipe);


///////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////


typedef struct sr___atom___t sr___atom___t;

extern int sr___atom___create(sr___atom___t **pp_atom);
extern void sr___atom___release(sr___atom___t **pp_atom);

extern void sr___atom___lock(sr___atom___t *atom);
extern void sr___atom___unlock(sr___atom___t *atom);

extern void sr___atom___wait(sr___atom___t *atom);
extern void sr___atom___signal(sr___atom___t *atom);
extern void sr___atom___broadcast(sr___atom___t *atom);


///////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////


#endif	//___SELF_RELIANCE_H___
