
#include "Python.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include "perproc.h"

//Assoc_t *mrhttp_sess_store;
  //mrhttp_session_store = assoc_create();

void PerProc_init(PyObject* ignore, PyObject *loop_ptr) {
  printf("per proc init\n");


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

}

