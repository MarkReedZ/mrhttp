#pragma once
#include "Python.h"
#include "request.h"
#include "protocol.h"
#include "memprotocol.h"

//PyMethodDef Request_methods[];
//PyGetSetDef Request_getset[];

//static PyMethodDef Matcher_methods[] = {
  //{"match_request", (PyCFunction)matcher_match_request, METH_O, ""},
  //{NULL}
//};

static PyMethodDef MemcachedProtocol_methods[] = {
  {"connection_made", (PyCFunction)MemcachedProtocol_connection_made, METH_O,       ""},
  {"eof_received",    (PyCFunction)MemcachedProtocol_eof_received,    METH_NOARGS,  ""},
  {"connection_lost", (PyCFunction)MemcachedProtocol_connection_lost, METH_VARARGS, ""},
  {"data_received",   (PyCFunction)MemcachedProtocol_data_received,   METH_O,       ""},
  {NULL}
};

static PyMethodDef Protocol_methods[] = {
  {"connection_made", (PyCFunction)Protocol_connection_made, METH_O,       ""},
  {"connection_lost", (PyCFunction)Protocol_connection_lost, METH_VARARGS, ""},
  {"eof_received",    (PyCFunction)Protocol_eof_received,    METH_NOARGS,  ""},
  {"data_received",   (PyCFunction)Protocol_data_received,   METH_O,       ""},
  {"pipeline_cancel", (PyCFunction)Protocol_pipeline_cancel, METH_NOARGS,  ""},
  {"task_done",       (PyCFunction)protocol_task_done,       METH_O,       ""},
  {NULL}
};

static PyMethodDef Request_methods[] = {
  //{"Response", (PyCFunction)Request_Response, METH_VARARGS | METH_KEYWORDS, ""},
  {"add_done_callback", (PyCFunction)Request_add_done_callback, METH_O,   ""},
  {NULL}
};
static PyGetSetDef Request_getset[] = {
  {"method", (getter)Request_get_method, NULL, "", NULL},
  {"transport", (getter)Request_get_transport, NULL, "", NULL},
  {"headers", (getter)Request_get_headers, NULL, "", NULL},
  {"cookies", (getter)Request_get_cookies, NULL, "", NULL},
  {"body", (getter)Request_get_body, NULL, "", NULL},
  {NULL}
};

static PyMethodDef Response_methods[] = {
  {"updateDate",        (PyCFunction)response_updateDate,        METH_O,   ""},
  {NULL}
};
static PyGetSetDef Response_getset[] = {
  {"headers", (getter)Response_get_headers, NULL, "", NULL},
  {"cookies", (getter)Response_get_cookies, NULL, "", NULL},
  {NULL}
};

static PyTypeObject RequestType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  "internals.Request",       /* tp_name */
  sizeof(Request),          /* tp_basicsize */
  0,                         /* tp_itemsize */
  (destructor)Request_dealloc, /* tp_dealloc */
  0,                         /* tp_print */
  0,                         /* tp_getattr */
  0,                         /* tp_setattr */
  0,                         /* tp_reserved */
  0,                         /* tp_repr */
  0,                         /* tp_as_number */
  0,                         /* tp_as_sequence */
  0,                         /* tp_as_mapping */
  0,                         /* tp_hash  */
  0,                         /* tp_call */
  0,                         /* tp_str */
  (getattrofunc)Request_getattro, /* tp_getattro */
  0,                         /* tp_setattro */
  0,                         /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,        /* tp_flags */
  "Request",                /* tp_doc */
  0,                         /* tp_traverse */
  0,                         /* tp_clear */
  0,                         /* tp_richcompare */
  0,                         /* tp_weaklistoffset */
  0,                         /* tp_iter */
  0,                         /* tp_iternext */
  Request_methods,           /* tp_methods */
  0,                         /* tp_members */
  Request_getset,            /* tp_getset */
  0,                         /* tp_base */
  0,                         /* tp_dict */
  0,                         /* tp_descr_get */
  0,                         /* tp_descr_set */
  0,                         /* tp_dictoffset */
  (initproc)Request_init,    /* tp_init */
  0,                         /* tp_alloc */
  Request_new,              /* tp_new */
};
static PyTypeObject ResponseType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  "internals.Response",       /* tp_name */
  sizeof(Response),          /* tp_basicsize */
  0,                         /* tp_itemsize */
  (destructor)Response_dealloc, /* tp_dealloc */
  0,                         /* tp_print */
  0,                         /* tp_getattr */
  0,                         /* tp_setattr */
  0,                         /* tp_reserved */
  0,                         /* tp_repr */
  0,                         /* tp_as_number */
  0,                         /* tp_as_sequence */
  0,                         /* tp_as_mapping */
  0,                         /* tp_hash  */
  0,                         /* tp_call */
  0,                         /* tp_str */
  0,                         /* tp_getattro */
  0,                         /* tp_setattro */
  0,                         /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT,        /* tp_flags */
  "Response",                /* tp_doc */
  0,                         /* tp_traverse */
  0,                         /* tp_clear */
  0,                         /* tp_richcompare */
  0,                         /* tp_weaklistoffset */
  0,                         /* tp_iter */
  0,                         /* tp_iternext */
  Response_methods,           /* tp_methods */
  0,                         /* tp_members */
  Response_getset,            /* tp_getset */
  0,                         /* tp_base */
  0,                         /* tp_dict */
  0,                         /* tp_descr_get */
  0,                         /* tp_descr_set */
  0,                         /* tp_dictoffset */
  (initproc)Response_init,    /* tp_init */
  0,                         /* tp_alloc */
  Response_new,              /* tp_new */
};

static PyTypeObject ProtocolType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  "protocol.Protocol",      /* tp_name */
  sizeof(Protocol),          /* tp_basicsize */
  0,                         /* tp_itemsize */
  (destructor)Protocol_dealloc, /* tp_dealloc */
  0,                         /* tp_print */
  0,                         /* tp_getattr */
  0,                         /* tp_setattr */
  0,                         /* tp_reserved */
  0,                         /* tp_repr */
  0,                         /* tp_as_number */
  0,                         /* tp_as_sequence */
  0,                         /* tp_as_mapping */
  0,                         /* tp_hash  */
  0,                         /* tp_call */
  0,                         /* tp_str */
  0,                         /* tp_getattro */
  0,                         /* tp_setattro */
  0,                         /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT,        /* tp_flags */
  "Protocol",                /* tp_doc */
  0,                         /* tp_traverse */
  0,                         /* tp_clear */
  0,                         /* tp_richcompare */
  0,                         /* tp_weaklistoffset */
  0,                         /* tp_iter */
  0,                         /* tp_iternext */
  Protocol_methods,          /* tp_methods */
  0,                         /* tp_members */
  0,//Protocol_getset,           /* tp_getset */
  0,                         /* tp_base */
  0,                         /* tp_dict */
  0,                         /* tp_descr_get */
  0,                         /* tp_descr_set */
  0,                         /* tp_dictoffset */
  (initproc)Protocol_init,   /* tp_init */
  0,                         /* tp_alloc */
  Protocol_new,              /* tp_new */
};

static PyTypeObject MemcachedProtocolType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  "internals.MemcachedProtocol",      /* tp_name */
  sizeof(MemcachedProtocol),          /* tp_basicsize */
  0,                         /* tp_itemsize */
  (destructor)MemcachedProtocol_dealloc, /* tp_dealloc */
  0,                         /* tp_print */
  0,                         /* tp_getattr */
  0,                         /* tp_setattr */
  0,                         /* tp_reserved */
  0,                         /* tp_repr */
  0,                         /* tp_as_number */
  0,                         /* tp_as_sequence */
  0,                         /* tp_as_mapping */
  0,                         /* tp_hash  */
  0,                         /* tp_call */
  0,                         /* tp_str */
  0,                         /* tp_getattro */
  0,                         /* tp_setattro */
  0,                         /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT,        /* tp_flags */
  "Memcached client protocol",                /* tp_doc */
  0,                         /* tp_traverse */
  0,                         /* tp_clear */
  0,                         /* tp_richcompare */
  0,                         /* tp_weaklistoffset */
  0,                         /* tp_iter */
  0,                         /* tp_iternext */
  MemcachedProtocol_methods,          /* tp_methods */
  0,                         /* tp_members */
  0,//Protocol_getset,           /* tp_getset */
  0,                         /* tp_base */
  0,                         /* tp_dict */
  0,                         /* tp_descr_get */
  0,                         /* tp_descr_set */
  0,                         /* tp_dictoffset */
  (initproc)MemcachedProtocol_init,   /* tp_init */
  0,                         /* tp_alloc */
  MemcachedProtocol_new,              /* tp_new */
};

