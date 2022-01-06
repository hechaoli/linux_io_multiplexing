#include <poll.h>
#include <stdio.h>
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
  struct pollfd fds[num_clients];

  for (int i = 0; i < num_clients; i++) {
    fds[i].events = POLLIN;
    fds[i].fd = client_fds[i];
  }

  while (1) {
    int retval = poll(fds, num_clients, /*timeout=*/-1);
    if (retval < 0) {
      perror("epoll_wait");
      goto fail;
    }
    int cnt = 0;
    for (int i = 0; i < num_clients; i++) {
      if (fds[i].revents & POLLIN) {
        char buf[BUF_SIZE] = {0};
        int nbytes = read(client_fds[i], buf, sizeof(buf));

        if (nbytes < 0) {
          perror("read");
          goto fail;
        } else if (nbytes > 0) {
          fprintf(stdout, "[%lu] [Server] Client %d: %s\n", time(NULL), i, buf);
        }
        cnt++;
      }
      if (fds[i].revents & POLLHUP) {
        fprintf(stdout, "[%lu] [Server] Client %d closed\n", time(NULL), i);
        cnt++;
      }
    }
    if (cnt != retval) {
      fprintf(stderr,
              "Number of set file descriptors (%d) != number returned from "
              "epoll_wait (%d)\n",
              cnt, retval);
      goto fail;
    }
  }
fail:
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
