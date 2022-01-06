#include <time.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#define NUM_CLIENTS 5
#define BUF_SIZE 16

static int run_client(int id, int write_fd) {
  for (int cnt = 0;; cnt++) {
    char buf[BUF_SIZE] = {0};
    int nbytes = snprintf(buf, sizeof(buf), "%d", cnt);
#ifdef DEBUG
    fprintf(stdout, "[%lu] [Client %d] Write %s\n", time(NULL), id, buf);
#endif
    write(write_fd, buf, nbytes);
    sleep(id + 1);
  }
  return 0;
}

static int run_server(int client_fds[], int num_clients, int max_client_fd) {
  fd_set client_fd_set;

  while (1) {
    FD_ZERO(&client_fd_set);
    for (int i = 0; i < num_clients; i++) {
      FD_SET(client_fds[i], &client_fd_set);
    }
    int ready_cnt =
        select(max_client_fd + 1, &client_fd_set,
               /*writefds=*/NULL, /*exceptfds=*/NULL, /*timeout=*/NULL);
    if (ready_cnt < 0) {
      perror("select");
      goto fail;
    }
    int cnt = 0;
    for (int i = 0; i < num_clients; i++) {
      if (FD_ISSET(client_fds[i], &client_fd_set)) {
        char buf[BUF_SIZE] = {0};
        int nbytes = read(client_fds[i], buf, sizeof(buf));

        if (nbytes < 0) {
          perror("read");
          goto fail;
        } else if (nbytes > 0) {
          fprintf(stdout, "[%lu] [Server] Client %d: %s\n",
                  time(NULL), i, buf);
        } else {
          fprintf(stdout, "[%lu] [Server] Client %d closed\n", time(NULL), i);
        }
        cnt++;
      }
    }
    if (cnt != ready_cnt) {
      fprintf(stderr,
              "[%lu] [Server] Number of set file descriptors (%d) != number "
              "returned from "
              "select (%d)\n",
              time(NULL), cnt, ready_cnt);
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
  int client_fds[NUM_CLIENTS] = {0};
  int max_client_fd = 0;

  for (int i = 0; i < NUM_CLIENTS; i++) {
    int pipefd[2] = {0};
    if (pipe(pipefd) != 0) {
      perror("pipe");
      return -1;
    }
    int read_fd = pipefd[0];
    int write_fd = pipefd[1];
    client_fds[i] = read_fd;
    if (read_fd > max_client_fd) {
      max_client_fd = read_fd;
    }
    pid_t child_pid = fork();

    if (child_pid < 0) {
      perror("fork");
     return child_pid;
    } else if (child_pid == 0) {
      close(read_fd);
      return run_client(i, write_fd);
    }
    close(write_fd);
  }
  return run_server(client_fds, NUM_CLIENTS, max_client_fd);
}
