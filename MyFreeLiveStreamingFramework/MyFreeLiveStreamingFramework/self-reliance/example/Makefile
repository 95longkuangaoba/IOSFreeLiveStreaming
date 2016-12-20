CC = gcc
CFLAGS = -DNDEBUG -D___TESTTEST_MEMORY___ -I../src -I../src/io
LDFLAGS = -lpthread

all:
	$(CC) -o example example.c \
		../src/self-reliance.c \
		../src/self-reliance-memory.c \
		../src/self-reliance-buffer.c \
		../src/self-reliance-log.c \
		../src/self-reliance-media.c \
		../src/self-reliance-media-muxer.c \
		../src/self-reliance-media-demux.c \
		../src/self-reliance-media-io.c \
		../src/io/file.c \
		$(CFLAGS) $(LDFLAGS)

clean:
	rm -rfv example
