#include "self-reliance.h"



typedef struct sr___pipe___t {
	char *buffer;
	unsigned int size;
	unsigned int writer;
	unsigned int reader;
}sr___pipe___t;


int fls( int x )
{
	int i, position;

	if ( x != 0 ){
		for ( i = x >> 1, position = 1; i != 0; position ++ ){
			i >>= 1;
		}
	}else{
		position = ( -1 );
	}

	return position;
}


int sr___pipe___create(unsigned int size, sr___pipe___t **pp_pipe)
{
	int result = 0;
	sr___pipe___t *pipe = NULL;

	if ( pp_pipe == NULL || size == 0 ){
		log_error(ERRPARAM);
		return ERRPARAM;
	}

	if ((pipe = (sr___pipe___t *)srmalloc(sizeof(sr___pipe___t))) == NULL){
		log_error(ERRMEMORY);
		return ERRMEMORY;
	}

	if ( ( size != 0 && ( size & ( size - 1 ) ) == 0 ) ){
		pipe->size = size;
	}else{
		pipe->size = (unsigned int)(1UL << fls( size - 1 ));
	}

	if ((pipe->buffer = ( char * )srmalloc( pipe->size )) == NULL){
		sr___pipe___release(&pipe);
		log_error(ERRMEMORY);
		return ERRMEMORY;
	}

	pipe->writer = pipe->reader = 0;

	*pp_pipe = pipe;

	return 0;
}


void sr___pipe___release(sr___pipe___t **pp_pipe)
{
	if (pp_pipe && *pp_pipe){
		sr___pipe___t *pipe = *pp_pipe;
		*pp_pipe = NULL;
		srfree(pipe->buffer);
		srfree(pipe);
	}
}


unsigned int sr___pipe___writable(sr___pipe___t *pipe)
{
	if (pipe){
		return pipe->size - pipe->writer + pipe->reader;
	}
	return 0;
}


unsigned int sr___pipe___readable(sr___pipe___t *pipe)
{
	if (pipe){
		return pipe->writer - pipe->reader;
	}
	return 0;
}


void sr___pipe___clean(sr___pipe___t *pipe)
{
	if (pipe){
		pipe->writer = pipe->reader = 0;
	}
}


unsigned int sr___pipe___write(sr___pipe___t *pipe, char *data, unsigned int size)
{
	if (pipe == NULL || data == NULL || size == 0){
		log_error(ERRPARAM);
		return 0;
	}

	unsigned int writable = pipe->size - pipe->writer + pipe->reader;
	unsigned int remain = pipe->size - ( pipe->writer & ( pipe->size - 1 ) );

	if ( writable == 0 ){
		return 0;
	}

	size = writable < size ? writable : size;

	if ( remain >= size ){
		memcpy( pipe->buffer + ( pipe->writer & ( pipe->size - 1 ) ), data, size);
	}else{
		memcpy( pipe->buffer + ( pipe->writer & ( pipe->size - 1 ) ), data, remain);
		memcpy( pipe->buffer, data + remain, size - remain);
	}

	SR___ATOM___ADD( pipe->writer, size );

	return size;
}


unsigned int sr___pipe___read(sr___pipe___t *pipe, char *buffer, unsigned int size)
{
	if (pipe == NULL || buffer == NULL || size == 0){
		log_error(ERRPARAM);
		return 0;
	}

	unsigned int readable = pipe->writer - pipe->reader;
	unsigned int remain = pipe->size - ( pipe->reader & ( pipe->size - 1 ) );

	if ( readable == 0 ){
		return 0;
	}

	size = readable < size ? readable : size;

	if ( remain >= size ){
		memcpy( buffer, pipe->buffer + ( pipe->reader & ( pipe->size - 1 ) ), size);
	}else{
		memcpy( buffer, pipe->buffer + ( pipe->reader & ( pipe->size - 1 ) ), remain);
		memcpy( buffer + remain, pipe->buffer, size - remain);
	}

	SR___ATOM___ADD( pipe->reader, size );

	return size;
}




typedef struct sr___queue___t {
	int size;
	int pushable;
	int popable;
	sr___node___t head;
	sr___node___t end;
	bool lock;
}sr___queue___t;


int sr___queue___create(size_t size, sr___queue___t **pp_queue)
{
	sr___queue___t *queue = NULL;

	if (pp_queue == NULL || size <= 0){
		log_error(ERRPARAM);
		return ERRPARAM;
	}

	if ((queue = (sr___queue___t *) srmalloc(sizeof(sr___queue___t))) == NULL){
		log_error(ERRMEMORY);
		return ERRMEMORY;
	}

	queue->lock = false;
	queue->size = size;
	queue->pushable = size;
	queue->popable = 0;
	queue->head.prev = NULL;
	queue->head.next = &(queue->end);
	queue->end.next = NULL;
	queue->end.prev = &(queue->head);

	*pp_queue = queue;

	return 0;
}


void sr___queue___release(sr___queue___t **pp_queue)
{
	if (pp_queue && *pp_queue){
		sr___queue___t *queue = *pp_queue;
		*pp_queue = NULL;
		sr___queue___clean(queue);
		srfree(queue);
	}
}


int sr___queue___push_head(sr___queue___t *queue, sr___node___t *node)
{
	if (NULL == queue || NULL == node){
		log_error(ERRPARAM);
		return ERRPARAM;
	}

	if (!SR___ATOM___TRYLOCK(queue->lock)){
		return ERRTRYAGAIN;
	}

	if (queue->pushable == 0){
		SR___ATOM___UNLOCK(queue->lock);
		return ERRTRYAGAIN;
	}

	node->next = queue->head.next;
	node->next->prev = node;

	node->prev = &(queue->head);
	queue->head.next = node;

	queue->popable++;
	queue->pushable--;

	SR___ATOM___UNLOCK(queue->lock);

	return 0;
}


int sr___queue___push_end(sr___queue___t *queue, sr___node___t *node)
{
	if (NULL == queue || NULL == node){
		return ERRPARAM;
		log_error(ERRPARAM);
	}

	if (!SR___ATOM___TRYLOCK(queue->lock)){
		return ERRTRYAGAIN;
	}

	if (queue->pushable == 0){
		SR___ATOM___UNLOCK(queue->lock);
		return ERRTRYAGAIN;
	}

	node->prev = queue->end.prev;
	node->prev->next = node;

	queue->end.prev = node;
	node->next = &(queue->end);

	queue->popable++;
	queue->pushable--;

	SR___ATOM___UNLOCK(queue->lock);

	return 0;
}


int sr___queue___pop_first(sr___queue___t *queue, sr___node___t **pp_node)
{
	if (NULL == queue || NULL == pp_node){
		log_error(ERRPARAM);
		return ERRPARAM;
	}

	if (!SR___ATOM___TRYLOCK(queue->lock)){
		return ERRTRYAGAIN;
	}

	if (queue->head.next == &(queue->end)){
		SR___ATOM___UNLOCK(queue->lock);
		return ERRTRYAGAIN;
	}

	(*pp_node) = queue->head.next;
	queue->head.next = (*pp_node)->next;
	(*pp_node)->next->prev = &queue->head;

	queue->pushable++;
	queue->popable--;

	SR___ATOM___UNLOCK(queue->lock);

	return 0;
}


int sr___queue___pop_last(sr___queue___t *queue, sr___node___t **pp_node)
{
	if (NULL == queue || NULL == pp_node){
		log_error(ERRPARAM);
		return ERRPARAM;
	}

	if (!SR___ATOM___TRYLOCK(queue->lock)){
		return ERRTRYAGAIN;
	}

	if (queue->end.prev == &(queue->head)){
		SR___ATOM___UNLOCK(queue->lock);
		return ERRTRYAGAIN;
	}

	(*pp_node) = queue->end.prev;
	queue->end.prev = (*pp_node)->prev;
	(*pp_node)->prev->next = &queue->end;

	queue->pushable++;
	queue->popable--;

	SR___ATOM___UNLOCK(queue->lock);

	return 0;
}


int sr___queue___get_first(sr___queue___t *queue, sr___node___t **pp_node)
{
	if (NULL == queue || NULL == pp_node){
		log_error(ERRPARAM);
		return ERRPARAM;
	}

	if (!SR___ATOM___TRYLOCK(queue->lock)){
		return ERRTRYAGAIN;
	}

	if (queue->end.prev == &(queue->head)){
		SR___ATOM___UNLOCK(queue->lock);
		return ERRTRYAGAIN;
	}

	*pp_node = queue->head.next;

	SR___ATOM___UNLOCK(queue->lock);

	return 0;
}


int sr___queue___get_last(sr___queue___t *queue, sr___node___t **pp_node)
{
	if (NULL == queue || NULL == pp_node){
		log_error(ERRPARAM);
		return ERRPARAM;
	}

	if (!SR___ATOM___TRYLOCK(queue->lock)){
		return ERRTRYAGAIN;
	}

	if (queue->end.prev == &(queue->head)){
		SR___ATOM___UNLOCK(queue->lock);
		return ERRTRYAGAIN;
	}

	*pp_node = queue->end.prev;

	SR___ATOM___UNLOCK(queue->lock);

	return 0;
}


int sr___queue___remove_node(sr___queue___t *queue, sr___node___t *node)
{
	if (NULL == queue || NULL == node || NULL == node->prev || NULL == node->next){
		log_error(ERRPARAM);
		return ERRPARAM;
	}

	SR___ATOM___LOCK(queue->lock);

	node->prev->next = node->next;
	node->next->prev = node->prev;

	queue->pushable++;
	queue->popable--;

	SR___ATOM___UNLOCK(queue->lock);

	return 0;
}


int sr___queue___clean(sr___queue___t *queue)
{
	if (NULL == queue){
		log_error(ERRPARAM);
		return ERRPARAM;
	}

	SR___ATOM___LOCK(queue->lock);

	sr___node___t *node = NULL;
	while (queue->head.next != &(queue->end)){
		node = queue->head.next;
		queue->head.next = queue->head.next->next;
		if (node && node->release){
			node->release(node);
		}
	}

	queue->head.prev = NULL;
	queue->head.next = &(queue->end);
	queue->end.next = NULL;
	queue->end.prev = &(queue->head);

	queue->pushable = queue->size;
	queue->popable = 0;

	SR___ATOM___UNLOCK(queue->lock);

	return 0;
}


int sr___queue___pushable(sr___queue___t *queue)
{
	if (NULL == queue){
		log_error(ERRPARAM);
		return ERRPARAM;
	}
	return queue->pushable;
}


int sr___queue___popable(sr___queue___t *queue)
{
	if (NULL == queue){
		log_error(ERRPARAM);
		return ERRPARAM;
	}
	return queue->popable;
}
