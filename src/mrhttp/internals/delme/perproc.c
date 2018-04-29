
#include "Python.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include "uv.h"


void pp_alloc_cb(uv_handle_t* handle, size_t size, uv_buf_t* buf) {
    buf->base = malloc(size);
    buf->len = size;
}

void pp_on_close(uv_handle_t* handle) {
  printf("closed\n");
}

void pp_on_read(uv_stream_t* tcp, ssize_t nread, const uv_buf_t *buf)
{
  if(nread >= 0) {
    printf("read: %.*s\n", (int)nread, buf->base);
  }
  else {
    //we got an EOF
    uv_close((uv_handle_t*)tcp, pp_on_close);
  }
  free(buf->base);
}
void pp_on_write(uv_write_t* req, int status)
{
  if (status) {
    //uv_err_t err = uv_last_error(loop);
    fprintf(stderr, "uv_write error: %s\n", uv_strerror(status));
    return;
  }
  printf("wrote.\n");
  //uv_close((uv_handle_t*)req->handle, on_close);
}

void pp_on_connect(uv_connect_t *conn, int status) {
  printf("connect status %d\n",status);
  if (status < 0) {
    fprintf(stderr, "connect failed error %s\n", uv_err_name(status));
    //free(req);
    return;
  }

  printf("connected.\n");

  uv_stream_t* stream = conn->handle;

  uv_buf_t buffer[] = {
    {.base = "12345678901234567890", .len = 20},
    {.base = "12345678901234567890", .len = 20}
  };
  uv_write_t request;

  uv_write(&request, stream, buffer, 2, pp_on_write);
  //uv_write((uv_write_t*) req, client, &req->buf, 1, echo_write);
  uv_read_start(stream, pp_alloc_cb, pp_on_read);
}
 

void PerProc_init(PyObject* ignore, PyObject *loop_ptr) {
  printf("per proc init\n");
/*
  //char *p = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0);
  unsigned long x = PyLong_AsUnsignedLong(loop_ptr);
  printf("DELME loop ptr ul %lx", x);
  uv_loop_t *loop = (uv_loop_t*)PyLong_AsVoidPtr(loop_ptr);
  printf("uv loop %p\n", loop);
  loop = (uv_loop_t*)x;
  printf("uv loop %p\n", loop);
  uv_tcp_t socket;
  uv_tcp_init(loop, &socket);
  uv_tcp_keepalive(&socket, 1, 60);

  struct sockaddr_in addr;
  uv_ip4_addr("127.0.0.1", 7000, &addr);
  uv_connect_t connect;
  uv_tcp_connect(&connect, &socket,(const struct sockaddr*)&addr, pp_on_connect);
*/

/*
  int fd = shm_open("test", O_RDWR|O_CREAT, 0600);
  printf("fd %d\n",fd);

  ftruncate(fd, 2*1024*1024*1024);
  //char *p = mmap(NULL, 4096, PROT_WRITE|PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, fd, 0);
  char *p = mmap(NULL, 4096, PROT_WRITE|PROT_WRITE, MAP_SHARED, fd, 0);
  if (p) {
    printf("was >%.*s<\n", 8, p);
    p[0] = 'H'; p[1] = 'e'; p[2] = 'l'; p[3] = 'l'; p[4] = 'o';
    printf("now >%.*s<\n", 8, p);
  }
  //shm_unlink("test");
*/

error:
  printf("end\n");
}

  //mrhttp_libuv_loop = uv_default_loop();
  //printf("uv default loop %p\n");
  //uv_loop_init(mrhttp_libuv_loop);
/*
  uv_tcp_t socket;
  uv_tcp_init(mrhttp_libuv_loop, &socket);
  uv_tcp_keepalive(&socket, 1, 60);

  struct sockaddr_in addr;
  uv_ip4_addr("127.0.0.1", 7000, &addr);
  uv_connect_t connect;
  uv_tcp_connect(&connect, &socket,(const struct sockaddr*)&addr, pp_on_connect);
  uv_run(mrhttp_libuv_loop, UV_RUN_DEFAULT);
*/
