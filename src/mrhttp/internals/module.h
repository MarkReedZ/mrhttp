
#pragma once

#include "Python.h"
#include "structmember.h"
#include "stddef.h"
#include "app.h"
#include "request.h"
#include "protocol.h"
#include "memprotocol.h"
#include "mrqprotocol.h"
#include "mrcacheprotocol.h"
#include "mrqclient.h"
#include "mrcacheclient.h"
#include "memcachedclient.h"
#include "router.h"
#include "utils.h"

//PyMethodDef Request_methods[];
//PyGetSetDef Request_getset[];

//static PyMethodDef Matcher_methods[] = {
  //{"match_request", (PyCFunction)matcher_match_request, METH_O, ""},
  //{NULL}
//};
static PyMethodDef mod_methods[] = {
  {"randint",   (PyCFunction)myrandint, METH_VARARGS,   "Generate a random integer in the interval [0,range]"},
  {"escape",    (PyCFunction)escape_html, METH_O,       "Escape <>&\" from a string" },
  {"to64",      (PyCFunction)to64,        METH_O,       "Number to base64 string"    },
  {"from64",    (PyCFunction)from64,      METH_O,       "Base64 string to number"    },
  {"timesince", (PyCFunction)timesince,   METH_O,       "Timestamp difference as string"},
  {"hot",       (PyCFunction)hot,         METH_VARARGS, "Chatt1r's hot algorithm"    },
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
static PyMethodDef MrcacheProtocol_methods[] = {
  {"connection_made", (PyCFunction)MrcacheProtocol_connection_made, METH_O,       ""},
  {"eof_received",    (PyCFunction)MrcacheProtocol_eof_received,    METH_NOARGS,  ""},
  {"connection_lost", (PyCFunction)MrcacheProtocol_connection_lost, METH_VARARGS, ""},
  {"data_received",   (PyCFunction)MrcacheProtocol_data_received,   METH_O,       ""},
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
static PyMemberDef MrhttpApp_members[] = {
    {"_mc", T_OBJECT, offsetof(MrhttpApp, py_mc), 0, NULL},
    {"_mrq", T_OBJECT, offsetof(MrhttpApp, py_mrq), 0, NULL},
    {"_redis", T_OBJECT, offsetof(MrhttpApp, py_redis), 0, NULL},
    {"_session_client", T_OBJECT, offsetof(MrhttpApp, py_session), 0, NULL},
    {"session_backend_type", T_OBJECT, offsetof(MrhttpApp, py_session_backend_type), 0, NULL},
    {NULL},
};
static PyMethodDef MrqClient_methods[] = {
  {"cinit", (PyCFunction)MrqClient_cinit, METH_NOARGS,   ""},
  {"_get",  (PyCFunction)MrqClient_get,   METH_VARARGS,  ""},
  {"set",   (PyCFunction)MrqClient_set,   METH_VARARGS,  ""},
  {NULL}
};
static PyMethodDef MrcacheClient_methods[] = {
  {"cinit", (PyCFunction)MrcacheClient_cinit, METH_NOARGS,   ""},
  {"_get",  (PyCFunction)MrcacheClient_get,   METH_VARARGS,  ""},
  {"set",   (PyCFunction)MrcacheClient_set,   METH_VARARGS,  ""},
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
  {"NotFound", (PyCFunction)Request_notfound, METH_NOARGS,   ""},
  {"cleanup", (PyCFunction)Request_cleanup,   METH_NOARGS,   ""},
  {"parse_mp_form", (PyCFunction)Request_parse_mp_form,   METH_NOARGS,   ""},
  {"parse_urlencoded_form", (PyCFunction)Request_parse_urlencoded_form,   METH_NOARGS,   ""},
  {NULL}
};

static PyMemberDef Request_members[] = {
    {"_json",  T_OBJECT, offsetof(Request, py_json),   0, NULL},
    {"mrpack", T_OBJECT, offsetof(Request, py_mrpack), 0, NULL},
    {"_form",  T_OBJECT, offsetof(Request, py_form),   0, NULL},
    {"_file",  T_OBJECT, offsetof(Request, py_file),   0, NULL},
    {"_files", T_OBJECT, offsetof(Request, py_files),  0, NULL},
    {"servers_down",T_OBJECT, offsetof(Request, py_mrq_servers_down),0, NULL},
    {"user",   T_OBJECT, offsetof(Request, py_user),   0, NULL},
    {"ip",     T_OBJECT, offsetof(Request, py_ip),     0, NULL},
    {NULL},
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
  MrhttpApp_methods,         /* tp_methods */
  MrhttpApp_members,         /* tp_members */
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
  Request_members,           /* tp_members */
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
static PyTypeObject MrcacheProtocolType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  "internals.MrcacheProtocol",      /* tp_name */
  sizeof(MrcacheProtocol),          /* tp_basicsize */
  0,                         /* tp_itemsize */
  (destructor)MrcacheProtocol_dealloc, /* tp_dealloc */
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
  MrcacheProtocol_methods,          /* tp_methods */
  0,                         /* tp_members */
  0,//Protocol_getset,           /* tp_getset */
  0,                         /* tp_base */
  0,                         /* tp_dict */
  0,                         /* tp_descr_get */
  0,                         /* tp_descr_set */
  0,                         /* tp_dictoffset */
  (initproc)MrcacheProtocol_init,   /* tp_init */
  0,                         /* tp_alloc */
  MrcacheProtocol_new,              /* tp_new */
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
static PyTypeObject MrcacheClientType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  "internals.MrcacheClient",       /* tp_name */
  sizeof(MrcacheClient),          /* tp_basicsize */
  0,                         /* tp_itemsize */
  (destructor)MrcacheClient_dealloc, /* tp_dealloc */
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
  "MrcacheClient",                /* tp_doc */
  0,                         /* tp_traverse */
  0,                         /* tp_clear */
  0,                         /* tp_richcompare */
  0,                         /* tp_weaklistoffset */
  0,                         /* tp_iter */
  0,                         /* tp_iternext */
  MrcacheClient_methods,           /* tp_methods */
  0,                         /* tp_members */
  0,            /* tp_getset */
  0,                         /* tp_base */
  0,                         /* tp_dict */
  0,                         /* tp_descr_get */
  0,                         /* tp_descr_set */
  0,                         /* tp_dictoffset */
  (initproc)MrcacheClient_init,    /* tp_init */
  0,                         /* tp_alloc */
  MrcacheClient_new,              /* tp_new */
};


