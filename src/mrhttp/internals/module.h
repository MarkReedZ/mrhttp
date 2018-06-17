
#pragma once
#include "Python.h"
#include "request.h"
#include "protocol.h"
#include "memprotocol.h"
#include "mrqprotocol.h"
#include "mrqclient.h"
#include "memcachedclient.h"
#include "router.h"
#include "app.h"
#include "utils.h"

//PyMethodDef Request_methods[];
//PyGetSetDef Request_getset[];

//static PyMethodDef Matcher_methods[] = {
  //{"match_request", (PyCFunction)matcher_match_request, METH_O, ""},
  //{NULL}
//};
static PyMethodDef mod_methods[] = {
  {"randint", (PyCFunction)myrandint, METH_VARARGS, "Generate a random integer in the interval [0,range]"},
  {NULL}
};

static PyMethodDef MemcachedProtocol_methods[] = {
  {"connection_made", (PyCFunction)MemcachedProtocol_connection_made, METH_O,       ""},
  {"eof_received",    (PyCFunction)MemcachedProtocol_eof_received,    METH_NOARGS,  ""},
  {"connection_lost", (PyCFunction)MemcachedProtocol_connection_lost, METH_VARARGS, ""},
  {"data_received",   (PyCFunction)MemcachedProtocol_data_received,   METH_O,       ""},
  {NULL}
};
static PyMethodDef MrqProtocol_methods[] = {
  {"connection_made", (PyCFunction)MrqProtocol_connection_made, METH_O,       ""},
  {"eof_received",    (PyCFunction)MrqProtocol_eof_received,    METH_NOARGS,  ""},
  {"connection_lost", (PyCFunction)MrqProtocol_connection_lost, METH_VARARGS, ""},
  {"data_received",   (PyCFunction)MrqProtocol_data_received,   METH_O,       ""},
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
static PyGetSetDef Protocol_getset[] = {
  {"pipeline_empty", (getter)Protocol_get_pipeline_empty, NULL, "", NULL},
  {"transport", (getter)Protocol_get_transport, NULL, "", NULL},
  {NULL}
};

static PyMethodDef Router_methods[] = {
  {"setupRoutes", (PyCFunction)Router_setupRoutes, METH_NOARGS,   ""},
  {NULL}
};
static PyMethodDef MrhttpApp_methods[] = {
  {"cinit",      (PyCFunction)MrhttpApp_cinit,       METH_NOARGS,  ""},
  {"updateDate", (PyCFunction)MrhttpApp_updateDate,  METH_O,       ""},
  {"check_idle", (PyCFunction)MrhttpApp_check_idle,  METH_NOARGS,  ""},
  {NULL}
};
static PyMethodDef MrqClient_methods[] = {
  {"cinit", (PyCFunction)MrqClient_cinit, METH_NOARGS,   ""},
  {NULL}
};
static PyMethodDef MemcachedClient_methods[] = {
  {"cinit", (PyCFunction)MemcachedClient_cinit, METH_NOARGS,   ""},
  {"set", (PyCFunction)MemcachedClient_set, METH_VARARGS,   ""},
  {NULL}
};
static PyMethodDef Request_methods[] = {
  //{"Response", (PyCFunction)Request_Response, METH_VARARGS | METH_KEYWORDS, ""},
  {"add_done_callback", (PyCFunction)Request_add_done_callback, METH_O,   ""},
  {"cleanup", (PyCFunction)Request_cleanup, METH_NOARGS,   ""},
  {NULL}
};
static PyGetSetDef Request_getset[] = {
  {"path", (getter)Request_get_path, NULL, "", NULL},
  {"method", (getter)Request_get_method, NULL, "", NULL},
  {"transport", (getter)Request_get_transport, NULL, "", NULL},
  {"headers", (getter)Request_get_headers, NULL, "", NULL},
  {"cookies", (getter)Request_get_cookies, NULL, "", NULL},
  {"body", (getter)Request_get_body, NULL, "", NULL},
  {"query_string", (getter)Request_get_query_string, NULL, "", NULL},
  {"args", (getter)Request_get_query_args, NULL, "", NULL},

  {NULL}
};

static PyMethodDef Response_methods[] = {
  {NULL}
};
static PyGetSetDef Response_getset[] = {
  {"headers", (getter)Response_get_headers, NULL, "", NULL},
  {"cookies", NULL, (setter)Response_set_cookies, "", NULL},
  {NULL}
};

static PyTypeObject MrhttpAppType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  "internals.MrhttpApp",       /* tp_name */
  sizeof(MrhttpApp),          /* tp_basicsize */
  0,                         /* tp_itemsize */
  (destructor)MrhttpApp_dealloc, /* tp_dealloc */
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
  0,
  0,                         /* tp_setattro */
  0,                         /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,        /* tp_flags */
  "MrhttpApp",                /* tp_doc */
  0,                         /* tp_traverse */
  0,                         /* tp_clear */
  0,                         /* tp_richcompare */
  0,                         /* tp_weaklistoffset */
  0,                         /* tp_iter */
  0,                         /* tp_iternext */
  MrhttpApp_methods,           /* tp_methods */
  0,                         /* tp_members */
  0,            /* tp_getset */
  0,                         /* tp_base */
  0,                         /* tp_dict */
  0,                         /* tp_descr_get */
  0,                         /* tp_descr_set */
  0,                         /* tp_dictoffset */
  (initproc)MrhttpApp_init,    /* tp_init */
  0,                         /* tp_alloc */
  MrhttpApp_new,              /* tp_new */
};

static PyTypeObject RouterType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  "internals.Router",       /* tp_name */
  sizeof(Router),          /* tp_basicsize */
  0,                         /* tp_itemsize */
  (destructor)Router_dealloc, /* tp_dealloc */
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
  0,
  0,                         /* tp_setattro */
  0,                         /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,        /* tp_flags */
  "Router",                /* tp_doc */
  0,                         /* tp_traverse */
  0,                         /* tp_clear */
  0,                         /* tp_richcompare */
  0,                         /* tp_weaklistoffset */
  0,                         /* tp_iter */
  0,                         /* tp_iternext */
  Router_methods,           /* tp_methods */
  0,                         /* tp_members */
  0,            /* tp_getset */
  0,                         /* tp_base */
  0,                         /* tp_dict */
  0,                         /* tp_descr_get */
  0,                         /* tp_descr_set */
  0,                         /* tp_dictoffset */
  (initproc)Router_init,    /* tp_init */
  0,                         /* tp_alloc */
  Router_new,              /* tp_new */
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
  Protocol_getset,           /* tp_getset */
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

static PyTypeObject MrqProtocolType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  "internals.MrqProtocol",      /* tp_name */
  sizeof(MrqProtocol),          /* tp_basicsize */
  0,                         /* tp_itemsize */
  (destructor)MrqProtocol_dealloc, /* tp_dealloc */
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
  "Mrq client protocol",                /* tp_doc */
  0,                         /* tp_traverse */
  0,                         /* tp_clear */
  0,                         /* tp_richcompare */
  0,                         /* tp_weaklistoffset */
  0,                         /* tp_iter */
  0,                         /* tp_iternext */
  MrqProtocol_methods,          /* tp_methods */
  0,                         /* tp_members */
  0,//Protocol_getset,           /* tp_getset */
  0,                         /* tp_base */
  0,                         /* tp_dict */
  0,                         /* tp_descr_get */
  0,                         /* tp_descr_set */
  0,                         /* tp_dictoffset */
  (initproc)MrqProtocol_init,   /* tp_init */
  0,                         /* tp_alloc */
  MrqProtocol_new,              /* tp_new */
};
static PyTypeObject MrqClientType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  "internals.MrqClient",       /* tp_name */
  sizeof(MrqClient),          /* tp_basicsize */
  0,                         /* tp_itemsize */
  (destructor)MrqClient_dealloc, /* tp_dealloc */
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
  0,
  0,                         /* tp_setattro */
  0,                         /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,        /* tp_flags */
  "MrqClient",                /* tp_doc */
  0,                         /* tp_traverse */
  0,                         /* tp_clear */
  0,                         /* tp_richcompare */
  0,                         /* tp_weaklistoffset */
  0,                         /* tp_iter */
  0,                         /* tp_iternext */
  MrqClient_methods,           /* tp_methods */
  0,                         /* tp_members */
  0,            /* tp_getset */
  0,                         /* tp_base */
  0,                         /* tp_dict */
  0,                         /* tp_descr_get */
  0,                         /* tp_descr_set */
  0,                         /* tp_dictoffset */
  (initproc)MrqClient_init,    /* tp_init */
  0,                         /* tp_alloc */
  MrqClient_new,              /* tp_new */
};
static PyTypeObject MemcachedClientType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  "internals.MemcachedClient",       /* tp_name */
  sizeof(MemcachedClient),          /* tp_basicsize */
  0,                         /* tp_itemsize */
  (destructor)MemcachedClient_dealloc, /* tp_dealloc */
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
  0,
  0,                         /* tp_setattro */
  0,                         /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,        /* tp_flags */
  "MemcachedClient",                /* tp_doc */
  0,                         /* tp_traverse */
  0,                         /* tp_clear */
  0,                         /* tp_richcompare */
  0,                         /* tp_weaklistoffset */
  0,                         /* tp_iter */
  0,                         /* tp_iternext */
  MemcachedClient_methods,           /* tp_methods */
  0,                         /* tp_members */
  0,            /* tp_getset */
  0,                         /* tp_base */
  0,                         /* tp_dict */
  0,                         /* tp_descr_get */
  0,                         /* tp_descr_set */
  0,                         /* tp_dictoffset */
  (initproc)MemcachedClient_init,    /* tp_init */
  0,                         /* tp_alloc */
  MemcachedClient_new,              /* tp_new */
};

