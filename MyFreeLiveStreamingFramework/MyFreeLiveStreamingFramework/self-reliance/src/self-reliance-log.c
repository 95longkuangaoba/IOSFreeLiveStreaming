#include "self-reliance.h"



#include <time.h>
#include <string.h>
#include <stdarg.h>



#define LOG_TIME_SIZE			32
#define LOG_LINE_SIZE			4096


#define PATHCLEAR(FILEPATH) \
	( strrchr( FILEPATH, '/' ) ? strrchr( FILEPATH, '/' ) + 1 : FILEPATH )


typedef struct ___sr___logger___t{
	int level;
	void (*callback) (int level, const char *tag, const char *log);
}___sr___logger___t;



static void sr___log___callback(int level, const char *tag, const char *log)
{
	if (level == SR_LOG_DEBUG){
		fprintf(stderr, "[D]		%-20s %s", tag, log);
	}else if (level == SR_LOG_WARNING){
		fprintf(stderr, "[W]		%-20s %s", tag, log);
	}else if (level == SR_LOG_ERROR){
		fprintf(stderr, "[E]		%-20s %s", tag, log);
	}else{
		fprintf(stderr, "[I]		%-20s %s", tag, log);
	}
}


static ___sr___logger___t global_logger = {SR_LOG_DEBUG, sr___log___callback};


void sr___log___initialize(int level, void (*callback) (int level, const char *tag, const char *log))
{
	if (level >= SR_LOG_DEBUG && level <= SR_LOG_ERROR && callback != NULL){
		global_logger.level = level;
		global_logger.callback = callback;
	}
}


void sr___log___release()
{
	global_logger.callback = sr___log___callback;
	global_logger.level = SR_LOG_DEBUG;
}


void sr___log___info(const char *format, ...)
{
	char log_line[LOG_LINE_SIZE] = {0};

	va_list args;
	va_start (args, format);
	vsnprintf(log_line, LOG_LINE_SIZE, format, args);
	va_end (args);

	if (global_logger.callback){
		global_logger.callback(SR_LOG_INFO, "info", log_line);
	}
}


static char* sr___time_to_string(char *buffer, size_t length)
{
	time_t current_time;
	struct tm *format_time;

	time(&current_time);
	format_time = localtime(&current_time);

	strftime(buffer, length, "%F:%H:%M:%S", format_time);
	return buffer;
}


void sr___log___debug(int level, const char *file, const char *function, int line, const char *format, ...)
{
	int i = 0;
	char log_line[LOG_LINE_SIZE] = {0};

	if (level >= global_logger.level){
		if (level == SR_LOG_DEBUG) {
			i = snprintf(log_line, LOG_LINE_SIZE, "%-20s[%-4d]    ", function, line);
		} else {
			char log_time[LOG_TIME_SIZE] = {0};
			i = snprintf(log_line, LOG_LINE_SIZE, "%s:%s[%-4d]    ", sr___time_to_string(log_time, LOG_TIME_SIZE), function, line);
		}

		va_list args;
		va_start (args, format);
		vsnprintf(log_line + i, LOG_LINE_SIZE - i, format, args);
		va_end (args);

		if (global_logger.callback){
			global_logger.callback(level, PATHCLEAR(file), log_line);
		}
	}
}
