#pragma once

#include <Python.h>
#include <stdbool.h>

#include "cdict.h"

typedef struct {
  PyObject* route;
  PyObject* handler;
  bool coro_func;
  bool simple;
  size_t pattern_len;
  size_t methods_len;
  size_t placeholder_cnt;
  char buffer[];
} MatcherEntry;

typedef struct _Matcher Matcher;

MatcherEntry* *matcher_match( Matcher* matcher, PyObject* request, CDictEntry** entries, size_t* entries_length);
