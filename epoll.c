#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define NUM_CLIENTS 5
#define BUF_SIZE 16

static void run_client(int id, int write_fd) {
  for (int cnt = 0;; cnt++) {
    char buf[BUF_SIZE] = {0};
    int nbytes = snprintf(buf, sizeof(buf), "%d", cnt);
#ifdef DEBUG
    fprintf(stdout, "[%lu] [Client %d] Write %s\n", (unsigned long)time(NULL),
            id, buf);
#endif
    write(write_fd, buf, nbytes);
    sleep(id + 1);
  }
}

static int run_server(int client_fds[], int num_clients) {
  struct epoll_event epoll_events[num_clients];
  int epoll_fd = epoll_create1(/*flags=*/0);

  if (epoll_fd < 0) {
    perror("epoll_create1");
    goto fail;
  }
  for (uint32_t i = 0; i < num_clients; i++) {
    struct epoll_event event;
    event.events = EPOLLIN;
#ifdef EDGE_TRIGGER
    event.events |= EPOLLET;
    fcntl(client_fds[i], F_SETFL, O_NONBLOCK);
#endif
    event.data.u32 = i;
    int retval =
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fds[i], &event);
    if (retval < 0) {
      perror("epoll_ctl");
      goto fail;
    }
  }


  while (1) {
    int ready_cnt = epoll_wait(epoll_fd, epoll_events, num_clients, /*timeout=*/-1);
    if (ready_cnt < 0) {
      perror("epoll_wait");
      goto fail;
    }
    for (int i = 0; i < ready_cnt; i++) {
      uint32_t id = epoll_events[i].data.u32;
      if (epoll_events[i].events & EPOLLIN) {
#ifdef EDGE_TRIGGER
        // In case of edge-triggered epoll, need to read until no more to read.
        while (1) {
          char buf[BUF_SIZE] = {0};
          int nbytes = read(client_fds[id], buf, sizeof(buf));

          if (nbytes < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
              break;
            }
            perror("read");
            goto fail;
          } else if (nbytes > 0) {
            fprintf(stdout, "[%lu] [Server] Client %d: %s\n", time(NULL), id, buf);
          }
        }
#else
        // In case of level-triggered epoll, we can read more next time if the
        // buffer is not big enough.
        char buf[BUF_SIZE] = {0};
        int nbytes = read(client_fds[id], buf, sizeof(buf));

        if (nbytes < 0) {
          perror("read");
          goto fail;
        } else if (nbytes > 0) {
          fprintf(stdout, "[%lu] [Server] Client %d: %s\n", time(NULL), id, buf);
        }
#endif
      }
      if (epoll_events[i].events & EPOLLHUP) {
        fprintf(stdout, "[%lu] [Server] Client %d closed\n", time(NULL), id);
      }
    }
  }
fail:
  if (epoll_fd > 0) {
    close(epoll_fd);
  }
  for (int i = 0; i < num_clients; i++) {
    close(client_fds[i]);
  }
  return -1;
}

int main() {
  int read_fds[NUM_CLIENTS] = {0};

  for (int i = 0; i < NUM_CLIENTS; i++) {
    int pipefd[2] = {0};
    if (pipe(pipefd) < 0) {
      perror("pipe");
      return -1;
    }
    int read_fd = pipefd[0];
    int write_fd = pipefd[1];
    read_fds[i] = read_fd;
    pid_t child_pid = fork();

    if (child_pid < 0) {
      perror("fork");
      return child_pid;
    } else if (child_pid == 0) {
      close(read_fd);
      run_client(i, write_fd);
      return 0;
    }
    close(write_fd);
  }
  return run_server(read_fds, NUM_CLIENTS);
}
