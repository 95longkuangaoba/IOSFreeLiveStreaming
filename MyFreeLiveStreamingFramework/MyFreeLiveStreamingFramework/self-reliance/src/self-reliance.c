#include "self-reliance.h"


#include <sys/time.h>
#include <pthread.h>


///////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////


const char* sr___error_to_message(int errorcode)
{
	switch(errorcode){
	case ERRSYSCALL:
		return "System calls failed !\n";
	case ERRMEMORY:
		return "Out of memory !\n";
	case ERRPARAM:
		return "Invalid parameter !\n";
	case ERRTIMEOUT:
		return "Has timed out !\n";
	case ERRTRYAGAIN:
		return "Try again later !\n";
	case ERRTERMINATION:
		return "termination !\n";
	case ERRUNKONWN:
		return "Undefined error ! ! !\n";
	default:
		break;
	}
	return "Nothing ! ! !\n";
}


///////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////





///////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////


int64_t sr___start_timing()
{
	struct timeval tv = {0};

	if (gettimeofday(&tv, NULL) != 0){
		log_error(ERRSYSCALL);
		return ERRSYSCALL;
	}

	return 1000000LL * tv.tv_sec + tv.tv_usec;
}


int64_t sr___complete_timing(int64_t start_microsecond)
{
	struct timeval tv = {0};

	if (gettimeofday(&tv, NULL) != 0){
		log_error(ERRSYSCALL);
		return ERRSYSCALL;
	}

	return (1000000LL * tv.tv_sec + tv.tv_usec) - start_microsecond;
}


///////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////


typedef struct sr___atom___t
{
	pthread_cond_t cond[1];
	pthread_mutex_t mutex[1];

}sr___atom___t;


int sr___atom___create(sr___atom___t **pp_atom)
{
	sr___atom___t *atom = NULL;

	if (pp_atom == NULL){
		log_error(ERRPARAM);
		return ERRPARAM;
	}

	if ((atom = srmalloc(sizeof(sr___atom___t))) == NULL){
		log_error(ERRMEMORY);
		return ERRMEMORY;
	}

	if (pthread_cond_init(atom->cond, NULL) != 0){
		sr___atom___release(&atom);
		log_error(ERRSYSCALL);
		return ERRSYSCALL;
	}

	if (pthread_mutex_init(atom->mutex, NULL) != 0){
		sr___atom___release(&atom);
		log_error(ERRSYSCALL);
		return ERRSYSCALL;
	}

	*pp_atom = atom;

	return 0;
}


void sr___atom___release(sr___atom___t **pp_atom)
{
	if (pp_atom && *pp_atom){
		sr___atom___t *atom = *pp_atom;
		*pp_atom = NULL;
		pthread_cond_destroy(atom->cond);
		pthread_mutex_destroy(atom->mutex);
		srfree(atom);
	}
}


inline void sr___atom___lock(sr___atom___t *atom)
{
	if (atom){
		if (pthread_mutex_lock(atom->mutex) != 0){
			log_error(ERRSYSCALL);
		}
	}
}


inline void sr___atom___unlock(sr___atom___t *atom)
{
	if (atom){
		if (pthread_mutex_unlock(atom->mutex) != 0){
			log_error(ERRSYSCALL);
		}
	}
}


inline void sr___atom___wait(sr___atom___t *atom)
{
	if (atom){
		if (pthread_cond_wait(atom->cond, atom->mutex) != 0){
			log_error(ERRSYSCALL);
		}
	}
}


inline void sr___atom___signal(sr___atom___t *atom)
{
	if (atom){
		if (pthread_cond_signal(atom->cond) != 0){
			log_error(ERRSYSCALL);
		}
	}
}


inline void sr___atom___broadcast(sr___atom___t *atom)
{
	if (atom){
		if (pthread_cond_broadcast(atom->cond) != 0){
			log_error(ERRSYSCALL);
		}
	}
}


///////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////
