# Linux I/O multiplexing examples

## Overview
This repo contains 3 examples of using system call `select`, `poll` and `epoll`
to achieve I/O multiplexing on Linux. Each of the example does the following:

* The main process acts as the server.
* Fork several child processes as clients.
* For each child process/client, create a pipe to communicate with it.
* The client constantly writes to the write side of the pipe.
* The server monitors the read side of each pipe. If there is anything to read,
  print the client id and the message.

See [this post](https://hechao.li/2022/01/04/select-vs-poll-vs-epoll/) about
select v.s. poll v.s. epoll.

# Build & Run
```
# Example of select
$ make select
$ ./select

# Example of poll
$ make poll
$ ./poll

# Example of epoll
$ make epoll
$ ./epoll

# All examples
$ make all

# Clean all generated files
$ make clean
```
