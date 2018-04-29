#include <Python.h>
#include <stdbool.h>

#include "matcher.h"
#include "request.h"

//TODO
  //if(!(router_route = PyImport_ImportModule("japronto.router.route"))) goto error;
  //if(!(compile_all = PyObject_GetAttrString(router_route, "compile_all"))) goto error;

struct _Matcher {
  PyObject_HEAD

  size_t buffer_len;
  char* buffer;
};


typedef enum {
  SEGMENT_EXACT,
  SEGMENT_PLACEHOLDER
} SegmentType;


typedef struct {
  size_t data_length;
  char data[];
} ExactSegment;


typedef struct {
  size_t name_length;
  char name[];
} PlaceholderSegment;


typedef struct {
  SegmentType type;

  union {
    ExactSegment exact;
    PlaceholderSegment placeholder;
  };
} Segment;


static CDictEntry _entries[10];

static PyObject* compile_all;

static PyObject * matcher_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  Matcher* self = NULL;

  self = (Matcher*)type->tp_alloc(type, 0);
  if(!self) return NULL;

  self->buffer = NULL;
  self->buffer_len = 0;

  return (PyObject*)self;
}


#define ROUNDTO8(v) (((v) + 7) & ~7)


#define ENTRY_LOOP \
char* entry_end = self->buffer + self->buffer_len; \
for(MatcherEntry* entry = (MatcherEntry*)self->buffer; \
    (char*)entry < entry_end; \
    entry = (MatcherEntry*)((char*)entry + sizeof(MatcherEntry) + \
      ROUNDTO8(entry->pattern_len) + ROUNDTO8(entry->methods_len)))

#define SEGMENT_LOOP \
char* segments_end = entry->buffer + entry->pattern_len; \
for(Segment* segment = (Segment*)entry->buffer; \
    (char*)segment < segments_end; \
    segment = (Segment*)((char*)segment + sizeof(Segment) + \
      ROUNDTO8(segment->type == SEGMENT_EXACT ? \
        segment->exact.data_length : segment->placeholder.name_length)))


static void matcher_dealloc(Matcher* self)
{
  if(self->buffer) {
    ENTRY_LOOP {
      Py_DECREF(entry->handler);
      Py_DECREF(entry->route);
    }
    free(self->buffer);
  }

  Py_TYPE(self)->tp_free((PyObject*)self);
}


int matcher_init(Matcher* self, PyObject *args, PyObject *kw)
{
  int result = 0;
  PyObject* compiled = NULL;

  PyObject* routes;
  if(!PyArg_ParseTuple(args, "O", &routes)) goto error;

  if(!(compiled = PyObject_CallFunctionObjArgs(compile_all, routes, NULL))) goto error;

  char* compiled_buffer;
  if(PyBytes_AsStringAndSize(compiled, &compiled_buffer, (Py_ssize_t*)&self->buffer_len) == -1) goto error;

  if(!(self->buffer = malloc(self->buffer_len))) goto error;

  memcpy(self->buffer, compiled_buffer, self->buffer_len);

  ENTRY_LOOP {
    Py_INCREF(entry->handler);
    Py_INCREF(entry->route);
  }

  goto finally;

  error:
  result = -1;
  finally:
  Py_XDECREF(compiled);
  return result;
}

// borrows route and handler in matcher entry
MatcherEntry* matcher_match(Matcher* self, PyObject* request, MatchDictEntry** match_dict_entries, size_t* match_dict_length)
{
  MatcherEntry* result = NULL;
  PyObject* path = NULL;
  PyObject* method = NULL;

  size_t method_len;
  char* method_str;
  size_t path_len;
  char* path_str;

#define REQUEST(r) \
  ((Request*)r)

#define REQUEST_METHOD(r) \
  REQUEST(r)->buffer

#define REQUEST_PATH(r) \
  REQUEST(r)->path

  method_len = ((Request*)request)->method_len;
  method_str = ((Request*)request)->method;
  //path_str = request_capi->Request_get_decoded_path( REQUEST(request), &path_len);

  ENTRY_LOOP {
    char* rest = path_str;
    size_t rest_len = path_len;

    MatchDictEntry* current_mde = _match_dict_entries;
    size_t value_length = 1;

    SEGMENT_LOOP {
      if(segment->type == SEGMENT_EXACT) {
        if(rest_len < segment->exact.data_length)
          break;

        if(memcmp(rest, segment->exact.data, segment->exact.data_length) != 0)
          break;

        rest += segment->exact.data_length;
        rest_len -= segment->exact.data_length;
      } else if(segment->type == SEGMENT_PLACEHOLDER) {
        assert(((size_t)(current_mde - _match_dict_entries)) < sizeof(_match_dict_entries) / sizeof(MatchDictEntry));

        char* slash = memchr(rest, '/', rest_len);
        current_mde->value = rest;
        if(slash) {
          value_length = current_mde->value_length = slash - rest;
          rest_len -= current_mde->value_length;
          rest = slash;
        } else {
          value_length = current_mde->value_length = rest_len;
          rest_len = 0;
        }

        if(!value_length)
          break;

        current_mde->key = segment->placeholder.name;
        current_mde->key_length = segment->placeholder.name_length;

        current_mde++;
      } else {
        assert(0);
      }
    }

    if(rest_len)
      continue;

    if(!value_length)
      continue;

    if((size_t)(current_mde - _match_dict_entries) != entry->placeholder_cnt)
      continue;

    if(!entry->methods_len)
      goto loop_finally;

    char* method_found = memmem(
      entry->buffer + entry->pattern_len, entry->methods_len,
      method_str, (size_t)method_len);
    if(!method_found)
      continue;

    if(*(method_found + (size_t)method_len) != ' ')
      continue;

    loop_finally:
    result = entry;

    if(match_dict_entries)
      *match_dict_entries = _match_dict_entries;
    if(match_dict_length)
      *match_dict_length = current_mde - _match_dict_entries;
    goto finally;
  }

  if(match_dict_length)
    *match_dict_length = 0;

  goto finally;

  error:
  result = NULL;

  finally:

  if(Py_TYPE(request) != request_capi->RequestType) {
    Py_XDECREF(method);
    Py_XDECREF(path);
  }

  return result;
}


PyObject* matcher_match_request(Matcher* self, PyObject* request)
{
  MatcherEntry* entry;
  CDictEntry* entries;
  PyObject* route = NULL;
  size_t length;
  PyObject* match_dict = NULL;
  PyObject* route_dict = NULL;

  if(!(entry = matcher_match( self, request, &entries, &length)))
    Py_RETURN_NONE;

  route = entry->route;

  if(!(match_dict = cdict_to_dict(entries, length))) goto error;

  if(!(route_dict = PyTuple_New(2))) goto error;

  PyTuple_SET_ITEM(route_dict, 0, route);
  PyTuple_SET_ITEM(route_dict, 1, match_dict);

  goto finally;

  error:
  Py_XDECREF(match_dict);
  route = NULL;

  finally:
  if(route) Py_INCREF(route);
  return route_dict;
}


