#pragma once

#include <Python.h>
#include <request.h>

#include "mrhttpparser.h"

enum connection_state {
  UNSET,
  CONN_KEEP_ALIVE,
  CONN_CLOSE
};

typedef struct {

  enum connection_state conn_state;
  
  int parsed_headers;
  long body_length;

  struct mr_chunked_decoder chunked_decoder;
  size_t chunked_offset;

  char *buf, *buf_end, *start, *end;
  int buf_size;

  void* protocol;

} Parser;

int  parser_init          (Parser* self, void* protocol);
int  parser_data_received (Parser* self, PyObject *py_data, Request *request );
void parser_dealloc       (Parser* self);

int parse_headers(Parser* self);
int parse_body   (Parser* self);

Parser* parser_disconnect(Parser* self);

