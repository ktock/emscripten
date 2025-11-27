#include <sys/select.h>
#include <time.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <sys/time.h>

// Check if timeout works without fds when the pthread flag is enabled
void test_timeout_without_fds()
{
  struct timeval tv, begin, end;

  tv.tv_sec = 1;
  tv.tv_usec = 0;
  gettimeofday(&begin, NULL);
  assert(select(0, NULL, NULL, NULL, &tv) == 0);
  gettimeofday(&end, NULL);
  assert((end.tv_sec - begin.tv_sec) * 1000000 + end.tv_usec - begin.tv_usec >= 1000000);
}

int pipe_shared[2];

void *wakeup_after_2s(void * arg)
{
  const char *t = "test\n";

  sleep(2);
  write(pipe_shared[1], t, strlen(t));

  return NULL;
}

// Check if select can unblock on an event on another thread
void test_unblock_select_with_thread()
{
  struct timeval begin, end;
  fd_set readfds;
  int maxfd;
  pthread_t tid;
  int pipe_a[2];

  assert(pipe(pipe_a) == 0);
  assert(pipe(pipe_shared) == 0);

  FD_ZERO(&readfds);
  FD_SET(pipe_a[0], &readfds);
  FD_SET(pipe_shared[0], &readfds);
  maxfd = (pipe_a[0] > pipe_shared[0] ? pipe_a[0] : pipe_shared[0]);
  assert(pthread_create(&tid, NULL, wakeup_after_2s, NULL) == 0);
  gettimeofday(&begin, NULL);
  assert(select(maxfd + 1, &readfds, NULL, NULL, NULL) == 1);
  gettimeofday(&end, NULL);
  assert(FD_ISSET(pipe_shared[0], &readfds));
  assert((end.tv_sec - begin.tv_sec) * 1000000 + end.tv_usec - begin.tv_usec >= 1000000);

  pthread_join(tid, NULL);

  close(pipe_a[0]); close(pipe_a[1]);
  close(pipe_shared[0]); close(pipe_shared[1]);
}

int main()
{
  test_timeout_without_fds();
  test_unblock_select_with_thread();
  return 0;
}
