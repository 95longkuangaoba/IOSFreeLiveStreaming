#include "self-reliance.h"

#include <sys/mman.h>
//#include <sys/sysinfo.h>
#include <pthread.h>
#include <unistd.h>



#ifndef NDEBUG
# define MSG_SIZE			0
#else
# define MSG_SIZE			256
#endif


#define	MAX_POOL_MASK			8
#define	MAX_POOL_NUMBER			8
//#define	MAX_POOL_NUMBER			256

#define	MAX_PAGE_MASK			8
#define	MAX_PAGE_NUMBER			256

#define	RECYCLE_POOL_NUMBER		8


#define	ALIGN_SIZE			( sizeof(size_t) )
#define	ALIGN_MASK			( ALIGN_SIZE - 1 )


typedef struct sr___pointer___t{

	/**
	 * 已分配状态：
	 * bit(19->32)		未使用
	 * bit(11->18)		内存池索引
	 * bit(3->10)		分页索引
	 * bit(2)			直接映射标志位			1：直接映射	0：内存池分配
	 * bit(1)			指针状态标志位			1：已分配	0：已释放
	 * 已释放状态：
	 * 前一个指针的大小
	 */

	size_t flag;


	/**
	 * 当前指针的大小
	 */

	size_t size;


	/**
	 * 定位指针信息
	 */

#ifdef	NDEBUG
	char msg[MSG_SIZE];
#endif

	/**
	 * 指针队列索引
	 */

	struct sr___pointer___t *prev;
	struct sr___pointer___t *next;

}sr___pointer___t;


#define	FREE_POINTER_SIZE		( MSG_SIZE + ALIGN_SIZE * 4 )

#define	WORK_POINTER_SIZE		( MSG_SIZE + ALIGN_SIZE * 2 )

#define	MIN_MEMORY_SIZE			( FREE_POINTER_SIZE - WORK_POINTER_SIZE )

#define	MIN_ALLOCATE_SIZE		FREE_POINTER_SIZE


#define	request2allocation(req) \
		( ( (req) + WORK_POINTER_SIZE < MIN_ALLOCATE_SIZE ) ? MIN_ALLOCATE_SIZE : \
				( (req) + WORK_POINTER_SIZE + ALIGN_MASK ) & ~ALIGN_MASK )

#define	pointer2address(p)			( (void *)((char *)( p ) + WORK_POINTER_SIZE ) )
#define	address2pointer(a)			( (sr___pointer___t *)( (char *)( a ) - WORK_POINTER_SIZE ) )

#define	INUSE					0x01
#define	MMAPPING				0x02

#define	mergeable(p)				( ! ( ( p )->flag & INUSE ) )
#define	ismapped(p)				( ( ( p )->flag & MMAPPING ) )

#define	prev_pointer(p)				( (sr___pointer___t *)( (char *)( p ) - ( p )->flag ) )
#define	next_pointer(p)				( (sr___pointer___t *)( (char *)( p ) + ( p )->size ) )
#define	new_next_pointer(p, s)			( (sr___pointer___t *)( (char *)( p ) + s ) )



typedef struct sr___pointer_queue___t{
	bool lock;
	sr___pointer___t *head;
}sr___pointer_queue___t;


typedef struct sr___memory_page___t{

	bool lock;

	//本页内存大小
	size_t size;

	//本页的索引
	size_t page_id;

	//本页内存起始地址
	void *start_address;


	/**
	 * 分裂新指针
	 * 合并闲指针
	 */
	sr___pointer___t *recycling_pointer;


	/**
	 * 已释放但未合并的指针队列
	 */
	sr___pointer___t *head, *end;


	/**
	 * 为快速释放指针设计的回收池
	 */
	sr___pointer_queue___t recycle_pool[RECYCLE_POOL_NUMBER];

}sr___memory_page___t;


typedef struct sr___memory_pool___t{
	bool lock;
	int pool_id;
	size_t page_number;
	uint64_t direct_mapping_size;
	uint64_t remaining_size;
	void *end_address;
	unsigned int start_position;
	sr___memory_page___t page_arrary[MAX_PAGE_NUMBER];
}sr___memory_pool___t;


typedef struct sr___memory_manager___t{
	pthread_t tid;
	pthread_key_t key;
	size_t page_size;
	size_t pool_number;
	unsigned int thread_number;
	size_t page_aligned_mask;
	sr___memory_pool___t pool_arrary[MAX_POOL_NUMBER];
}sr___memory_manager___t;


static bool memory_lock = false;
static sr___memory_manager___t memory_manager = {0}, *mm = &memory_manager;




static int sr___memory_page___create(sr___memory_pool___t *pool, size_t page_size)
{
	SR___ATOM___LOCK(pool->page_arrary[pool->page_number].lock);

	int pool_id = pool->page_number;

	pool->page_arrary[pool_id].size = (page_size + mm->page_aligned_mask) & (~mm->page_aligned_mask);

	do{
		pool->page_arrary[pool_id].start_address = mmap(pool->end_address,
			pool->page_arrary[pool_id].size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

		if (pool->page_arrary[pool_id].start_address == MAP_FAILED){
			SR___ATOM___UNLOCK(pool->page_arrary[pool_id].lock);
			log_warning("%s\n", strerror(errno));
			return ERRSYSCALL;
		}else{
			if ((size_t)(pool->page_arrary[pool_id].start_address) & ALIGN_MASK != 0){
				pool->end_address = (void *)(((size_t)(pool->page_arrary[pool_id].start_address) + ALIGN_MASK) & ~ALIGN_MASK);
				munmap(pool->page_arrary[pool_id].start_address, pool->page_arrary[pool_id].size);
			}else{
				pool->end_address = pool->page_arrary[pool_id].start_address + pool->page_arrary[pool_id].size;
				break;
			}
		}
	}while ( true );


	pool->page_arrary[pool_id].page_id = pool_id;

	pool->page_arrary[pool_id].head = (sr___pointer___t*)(pool->page_arrary[pool_id].start_address);
	pool->page_arrary[pool_id].head->flag = ((pool->pool_id << 10) | (pool->page_arrary[pool_id].page_id << 2) | INUSE);
	pool->page_arrary[pool_id].head->size = FREE_POINTER_SIZE;


	pool->page_arrary[pool_id].recycling_pointer = next_pointer(pool->page_arrary[pool_id].head);
	pool->page_arrary[pool_id].recycling_pointer->flag = ((pool->pool_id << 10) | (pool->page_arrary[pool_id].page_id << 2) | INUSE);
	pool->page_arrary[pool_id].recycling_pointer->size = pool->page_arrary[pool_id].size - (FREE_POINTER_SIZE * 2);


	pool->page_arrary[pool_id].end = next_pointer(pool->page_arrary[pool_id].recycling_pointer);
	pool->page_arrary[pool_id].end->flag = pool->page_arrary[pool_id].recycling_pointer->size;
	pool->page_arrary[pool_id].end->size = FREE_POINTER_SIZE;


	pool->page_arrary[pool_id].head->prev = NULL;
	pool->page_arrary[pool_id].head->next = pool->page_arrary[pool_id].end;
	pool->page_arrary[pool_id].end->prev = pool->page_arrary[pool_id].head;
	pool->page_arrary[pool_id].end->next = NULL;

	memset(&(pool->page_arrary[pool_id].recycle_pool), 0, sizeof(pool->page_arrary[pool_id].recycle_pool));

	SR___ATOM___ADD(pool->remaining_size, pool->page_arrary[pool_id].recycling_pointer->size);
	SR___ATOM___ADD(pool->page_number, 1);


	SR___ATOM___UNLOCK(pool->page_arrary[pool_id].lock);

//	log_debug("pool[%d] page[%d]=[%p] length[%ld] freed[%ld]\n",
//			pool->pool_id, pool_id, pool->page_arrary[pool_id].start_address,
//			pool->page_arrary[pool_id].size, pool->page_arrary[pool_id].recycling_pointer->size);

	return 0;
}




inline static void release_pointer(sr___pointer___t *pointer, sr___memory_page___t *page, sr___memory_pool___t *pool)
{
	if (next_pointer(pointer) == page->recycling_pointer){
		//合并左边指针
		if (mergeable(pointer)){
			//如果指针已经释放就进行合并
			//把合并的指针从释放队列中移除
			prev_pointer(pointer)->size += pointer->size;
			pointer = prev_pointer(pointer);
			pointer->next->prev = pointer->prev;
			pointer->prev->next = pointer->next;
		}

		//更新空闲内存大小
		SR___ATOM___ADD(pool->remaining_size, pointer->size);
		//更新空闲指针大小
		pointer->size += page->recycling_pointer->size;
		//合并指针
		next_pointer(pointer)->flag = pointer->size;
		page->recycling_pointer = pointer;

	}else if (next_pointer(page->recycling_pointer) == pointer){
		//合并右边指针
		if (mergeable(next_pointer(next_pointer(pointer)))){
			//如果指针已经释放就进行合并
			//把合并的指针从释放队列中移除
			next_pointer(pointer)->prev->next = next_pointer(pointer)->next;
			next_pointer(pointer)->next->prev = next_pointer(pointer)->prev;
			pointer->size += next_pointer(pointer)->size;
		}

		SR___ATOM___ADD(pool->remaining_size, pointer->size);
		//合并指针
		page->recycling_pointer->size += pointer->size;
		next_pointer(page->recycling_pointer)->flag = page->recycling_pointer->size;

	}else{
		/**
		 * 不能与主指针合并的指针就先尝试与左右两边的指针合并
		 */
		if (mergeable(pointer)){
			prev_pointer(pointer)->size += pointer->size;
			pointer = prev_pointer(pointer);
			next_pointer(pointer)->flag = pointer->size;
			pointer->next->prev = pointer->prev;
			pointer->prev->next = pointer->next;
		}

		if (mergeable(next_pointer(next_pointer(pointer)))){
			next_pointer(pointer)->prev->next = next_pointer(pointer)->next;
			next_pointer(pointer)->next->prev = next_pointer(pointer)->prev;
			pointer->size += next_pointer(pointer)->size;
		}

		//设置释放状态
		next_pointer(pointer)->flag = pointer->size;
		//把最大指针放入队列首
		if (pointer->size >= page->head->next->size){
			pointer->next = page->head->next;
			pointer->prev = page->head;
			pointer->next->prev = pointer;
			pointer->prev->next = pointer;
		}else{
			pointer->next = page->end;
			pointer->prev = page->end->prev;
			pointer->next->prev = pointer;
			pointer->prev->next = pointer;
		}
	}
}


static void sr___memory_page___flush_cache(sr___memory_pool___t *pool, sr___memory_page___t *page)
{
	sr___pointer___t *pointer = NULL;
	for (int i = 0; i < RECYCLE_POOL_NUMBER; ++i){
		SR___ATOM___LOCK(page->recycle_pool[i].lock);
		while(page->recycle_pool[i].head){
			pointer = page->recycle_pool[i].head;
			page->recycle_pool[i].head = page->recycle_pool[i].head->next;
			release_pointer(pointer, page, pool);
		}
		SR___ATOM___UNLOCK(page->recycle_pool[i].lock);
	}
}


#ifdef	NDEBUG
void* sr___memory___alloc(size_t size, const char *file, const char *function, int line)
#else
void* sr___memory___alloc(size_t size)
#endif
{

	size_t i = 0;
	size_t reserved_size = 0;
	sr___pointer___t *pointer = NULL;

	//获取当前线程的内存池
	sr___memory_pool___t *pool = (sr___memory_pool___t *)pthread_getspecific(mm->key);


	if (pool == NULL){

		//处理当前线程还没有创建内存池的情况

		int pool_id = 0;

		if (mm->pool_number < MAX_POOL_NUMBER){
			/**
			 * 如果当前内存池数量未到达上限就为当前线程创建一个内存池
			 */
			pool_id = SR___ATOM___ADD(mm->pool_number, 1) - 1;
			SR___ATOM___LOCK(mm->pool_arrary[pool_id].lock);
			mm->pool_arrary[pool_id].pool_id = pool_id;
			pool = &(mm->pool_arrary[pool_id]);
			for (i = 0; i < 2; ++i){
				if (sr___memory_page___create(&(mm->pool_arrary[pool->pool_id]), mm->page_size) != 0){
					SR___ATOM___UNLOCK(mm->pool_arrary[pool_id].lock);
					log_error(ERRMEMORY);
					return NULL;
				}
			}
			SR___ATOM___UNLOCK(mm->pool_arrary[pool_id].lock);

			pthread_setspecific(mm->key, &(mm->pool_arrary[pool->pool_id]));

		}else{
			/**
			 * 如果当前内存池数量已经到达上限就复用已有的内存池
			 */
			pool_id = SR___ATOM___ADD(mm->thread_number, 1) % MAX_POOL_NUMBER;
			pool = &(mm->pool_arrary[pool_id]);
		}
	}


	size = request2allocation(size);

	/**
	 * 确保分配一个新的指针之后剩余的内存不会小于一个空闲指针的大小
	 */
	reserved_size = size + FREE_POINTER_SIZE;


	if(ISFALSE(pool->page_number)){
		/**
		 * 如果当前内存池尚未创建分页就创建一个分页
		 */
		SR___ATOM___LOCK(pool->lock);
		if (sr___memory_page___create(&(mm->pool_arrary[pool->pool_id]), mm->page_size) != 0){
			SR___ATOM___UNLOCK(pool->lock);
			log_error(ERRMEMORY);
			return NULL;
		}
		SR___ATOM___UNLOCK(pool->lock);
	}


	for(i = SR___ATOM___ADD(pool->start_position, 1) % pool->page_number; /*i < pool->page_number*/;  i = ++i % pool->page_number){

		if (!SR___ATOM___TRYLOCK(pool->page_arrary[i].lock)){
			continue;
		}

		if (reserved_size > pool->page_arrary[i].recycling_pointer->size){

			sr___memory_page___flush_cache(pool, &(pool->page_arrary[i]));
			/**
			 * 处理当前分页的空闲内存不足的情况
			 */
			if (pool->page_arrary[i].head->next->size > reserved_size){
				/**
				 * 如果空闲指针队列中有够大的空闲指针就使用他作为分裂指针
				 */
				pool->page_arrary[i].recycling_pointer->next = pool->page_arrary[i].end;
				pool->page_arrary[i].recycling_pointer->prev = pool->page_arrary[i].end->prev;
				pool->page_arrary[i].recycling_pointer->next->prev = pool->page_arrary[i].recycling_pointer;
				pool->page_arrary[i].recycling_pointer->prev->next = pool->page_arrary[i].recycling_pointer;
				next_pointer(pool->page_arrary[i].recycling_pointer)->flag = pool->page_arrary[i].recycling_pointer->size;
				pool->page_arrary[i].recycling_pointer = pool->page_arrary[i].head->next;
				pool->page_arrary[i].head->next = pool->page_arrary[i].head->next->next;
				pool->page_arrary[i].head->next->prev = pool->page_arrary[i].head;
			}
		}



		if (pool->page_arrary[i].recycling_pointer->size > reserved_size){
			/**
			 * 如果当前有足够的内存就分配一个新的指针
			 */
			pointer = new_next_pointer(pool->page_arrary[i].recycling_pointer, size);
			pointer->flag = ((pool->pool_id << 10) | (i << 2) | INUSE);
			pointer->size = pool->page_arrary[i].recycling_pointer->size - size;
			next_pointer(pointer)->flag = pointer->size;
			pointer->prev = pool->page_arrary[i].recycling_pointer;
			pool->page_arrary[i].recycling_pointer = pointer;
			pointer = pool->page_arrary[i].recycling_pointer->prev;
			pointer->size = size;

			SR___ATOM___UNLOCK(pool->page_arrary[i].lock);

			memset(pointer2address(pointer), 0, pointer->size - WORK_POINTER_SIZE);

#ifdef NDEBUG
			int len = snprintf(pointer->msg, MSG_SIZE - 1, "%s[%s(%d)]", file, function, line);
			pointer->msg[len] = '\0';
#endif

			SR___ATOM___SUB(pool->remaining_size, size);

			return pointer2address(pointer);

		}

		SR___ATOM___UNLOCK(pool->page_arrary[i].lock);


		{
			if (size >= mm->page_size >> 1){
				/**
                 * 如果申请的内存大于分页长度就直接映射
                 */
				void *address = NULL;
				size = (size + mm->page_aligned_mask) & ~mm->page_aligned_mask;
				address = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
				if (address == MAP_FAILED){
					log_warning("%s\n", strerror(errno));
					return NULL;
				}
				pointer = address;
				pointer->size = size;
				pointer->flag = MMAPPING;
				pointer->flag = (size_t)((pool->pool_id << 10) | (MMAPPING));
				SR___ATOM___ADD(pool->direct_mapping_size, size);
				log_debug("mmap(%d)\n", size);
				return pointer2address(pointer);
			}
		}


		{
			/**
			 * 创建新的内存页
			 */
			SR___ATOM___LOCK(pool->lock);

//			struct sysinfo sys_info;
//			if (sysinfo(&sys_info) != 0){
//				log_warning("%s\n", strerror(errno));
//				return NULL;
//			}

			if (/*sys_info.freeram > sys_info.totalram >> 4 && */pool->page_number < MAX_PAGE_NUMBER){
				/**
				 * 分页数尚未到达上限与系统中空闲内存大于系统内存总数的16分之一就创建一个分页
				 */
				if (sr___memory_page___create(pool, mm->page_size) != 0){
					SR___ATOM___UNLOCK(pool->lock);
					log_error(ERRMEMORY);
					log_warning("pool number ==== %u\n", mm->pool_number);
					log_warning("request memory size ==== %ld\n", size);
					return NULL;
				}
			}

			SR___ATOM___UNLOCK(pool->lock);
		}

	}


	return NULL;

}




void sr___memory___free(void *address)
{
	int page_id = 0;
	int pool_id = 0;
	sr___pointer___t *pointer = NULL;
	sr___memory_page___t *page = NULL;
	sr___memory_pool___t *pool = NULL;

	if (address){

		pointer = address2pointer(address);

		if (ismapped(pointer)){
			pool_id = ((pointer->flag >> 10) & 0xFF);
			if ((pool_id >= MAX_POOL_NUMBER) || (pool_id != mm->pool_arrary[pool_id].pool_id)){
				log_info("invalid pool id %d\n", pool_id);
				log_error(ERRPARAM);
				return;
			}
			pool = &(mm->pool_arrary[pool_id]);
			size_t size = pointer->size;
			SR___ATOM___SUB(pool->direct_mapping_size, size);
			munmap(pointer, size);
			return;
		}


		pool_id = ((next_pointer(pointer)->flag >> 10) & 0xFF);
		if ((pool_id >= MAX_POOL_NUMBER) || (pool_id != mm->pool_arrary[pool_id].pool_id)){
			log_info("invalid pool id %d\n", pool_id);
			log_error(ERRPARAM);
			return;
		}
		pool = &(mm->pool_arrary[pool_id]);

		page_id = ((next_pointer(pointer)->flag >> 2) & 0xFF);
		if ((page_id >= MAX_PAGE_NUMBER) || page_id != pool->page_arrary[page_id].page_id){
			log_info("invalid page id %d\n", pool_id);
			log_error(ERRPARAM);
			return;
		}
		page = &(pool->page_arrary[page_id]);

		for (int i = 0; /*i < MEMORY_UNSORT_ARRARY_NUMBER*/; i = ++i % RECYCLE_POOL_NUMBER){
			if (SR___ATOM___TRYLOCK(page->recycle_pool[i].lock)){
				pointer->next = page->recycle_pool[i].head;
				page->recycle_pool[i].head = pointer;
				if (SR___ATOM___TRYLOCK(page->lock)){
					while(page->recycle_pool[i].head){
						pointer = page->recycle_pool[i].head;
						page->recycle_pool[i].head = page->recycle_pool[i].head->next;
						release_pointer(pointer, page, &(mm->pool_arrary[pool_id]));
					}
					SR___ATOM___UNLOCK(page->lock);
				}
				SR___ATOM___UNLOCK(page->recycle_pool[i].lock);
				break;
			}
		}
	}
}




int sr___memory___initialize(size_t page_size)
{
	int result = 0;

#ifdef ___TESTTEST_MEMORY___
	log_debug("enter\n");

	if (SR___ATOM___TRYLOCK(memory_lock)){

		memset(mm, 0, sizeof(sr___memory_manager___t));

		if ((result = pthread_key_create(&(mm->key), NULL)) != 0){
			log_warning("%s\n", strerror(errno));
			log_error(ERRSYSCALL);
			return ERRSYSCALL;
		}

		mm->page_size = page_size;
		mm->page_aligned_mask = (size_t) sysconf(_SC_PAGESIZE) - 1;
	}

	log_debug("exit\n");
#endif //___TESTTEST_MEMORY___

	return result;
}




static void sr___memory___debug()
{
	sr___memory_pool___t *pool = NULL;
	for (int pool_id = 0; pool_id < mm->pool_number && pool_id < MAX_POOL_NUMBER; ++pool_id){
		pool = &(mm->pool_arrary[pool_id]);
		if (pool->direct_mapping_size != 0){
			log_warning("has not been munmap(%lu) in pool[%d]\n", pool->direct_mapping_size, pool->pool_id);
		}
		for (int page_id = 0; page_id < pool->page_number; ++page_id){
			if (pool->page_arrary[page_id].start_address
				&& pool->page_arrary[page_id].recycling_pointer->size
				!= pool->page_arrary[page_id].size - FREE_POINTER_SIZE * 2){

				log_warning("pool[%d] page[%d] has not been free(%d)\n", pool->pool_id, page_id,
						pool->page_arrary[page_id].size - (pool->page_arrary[page_id].recycling_pointer->size + 2 * FREE_POINTER_SIZE));
				sr___pointer___t *pointer = (next_pointer(pool->page_arrary[page_id].head));
				while(pointer != pool->page_arrary[page_id].end){
#ifdef NDEBUG
					if (!mergeable(next_pointer(pointer))){
						log_debug("pool[%d] page[%d] location[%s]\n", pool->pool_id, page_id, pointer->msg);
					}
#endif
					pointer = next_pointer(pointer);
				}
			}
		}
	}
}


void sr___memory___dump()
{
#ifdef ___TESTTEST_MEMORY___
	log_debug("enter\n");
	sr___memory___flush_cache();
	sr___memory___debug();
	log_debug("exit\n");
#endif
}


void sr___memory___flush_cache()
{
	log_debug("sr___memory___flush_cache enter\n");
#ifdef ___TESTTEST_MEMORY___
	sr___pointer___t *pointer = NULL;
	sr___memory_pool___t *pool = NULL;
	for (int pool_id = 0; pool_id < mm->pool_number && pool_id < MAX_POOL_NUMBER; ++pool_id){
		pool = &(mm->pool_arrary[pool_id]);
		for (int page_id = 0; page_id < pool->page_number; ++page_id){
			if ( pool->page_arrary[page_id].start_address != NULL){
				SR___ATOM___LOCK(pool->page_arrary[page_id].lock);
				for (int i = 0; i < RECYCLE_POOL_NUMBER; ++i){
					SR___ATOM___LOCK(pool->page_arrary[page_id].recycle_pool[i].lock);
					while(pool->page_arrary[page_id].recycle_pool[i].head){
						pointer = pool->page_arrary[page_id].recycle_pool[i].head;
//						log_debug("pool[%d] page[%d] size[%d]\n", pool->pool_id, page_id, pointer->size);
						pool->page_arrary[page_id].recycle_pool[i].head = pool->page_arrary[page_id].recycle_pool[i].head->next;
						release_pointer(pointer, &(pool->page_arrary[page_id]), &(mm->pool_arrary[pool->pool_id]));
					}
					SR___ATOM___UNLOCK(pool->page_arrary[page_id].recycle_pool[i].lock);
				}
				SR___ATOM___UNLOCK(pool->page_arrary[page_id].lock);
			}
		}
	}
#endif
	log_debug("sr___memory___flush_cache exit\n");
}




void sr___memory___release()
{
#ifdef ___TESTTEST_MEMORY___
	log_debug("enter\n");

	sr___memory___flush_cache();

	sr___memory___debug();

	for (int pool_id = 0; pool_id < mm->pool_number && pool_id < MAX_POOL_NUMBER; ++pool_id){
		sr___memory_pool___t *pool = &(mm->pool_arrary[pool_id]);
		for (int page_id = 0; page_id < pool->page_number; ++page_id){
			if ( pool->page_arrary[page_id].start_address != NULL){
				munmap(pool->page_arrary[page_id].start_address, pool->page_arrary[page_id].size);
			}
		}
	}

	if (pthread_key_delete(mm->key) != 0){
		log_error(ERRSYSCALL);
	}

	memset(mm, 0, sizeof(sr___memory_manager___t));

	SR___ATOM___UNLOCK(memory_lock);

	log_debug("exit\n");
#endif
}
