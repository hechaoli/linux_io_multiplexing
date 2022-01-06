CC=gcc

all: select poll epoll

select: select.c
	 $(CC) -o select select.c

poll: poll.c
	 $(CC) -o poll poll.c

epoll: epoll.c
	 $(CC) -o epoll epoll.c

clean:
	rm -f select poll epoll
