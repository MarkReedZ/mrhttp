#include "module.h"
#include "protocol.h"
#include "request.h"


#define PROXY(attr) \
  {#attr, (getter)Request_get_proxy, NULL, "", #attr}
/*
static PyGetSetDef Request_getset[] = {
  {"method", (getter)Request_get_method, NULL, "", NULL},
  {"path", (getter)Request_get_path, NULL, "", NULL},
  {"query_string", (getter)Request_get_qs, NULL, "", NULL},
  {"version", (getter)Request_get_version, NULL, "", NULL},
  {"headers", (getter)Request_get_headers, NULL, "", NULL},
  //{"match_dict", (getter)Request_get_match_dict, NULL, "", NULL},
  {"body", (getter)Request_get_body, NULL, "", NULL},
  {"transport", (getter)Request_get_transport, NULL, "", NULL},
  //{"keep_alive", (getter)Request_get_keep_alive, NULL, "", NULL},
  //{"route", (getter)Request_get_route, NULL, "", NULL},
  {"extra", (getter)Request_get_extra, NULL, "", NULL},
  {"app", (getter)Request_get_app, NULL, "", NULL},
  PROXY(text),
  PROXY(json),
  PROXY(query),
  PROXY(mime_type),
  PROXY(encoding),
  PROXY(form),
  PROXY(files),
  PROXY(remote_addr),
  PROXY(hostname),
  PROXY(port),
  PROXY(cookies),
  {NULL}
};
*/
#undef PROXY

static PyModuleDef internals_module = {
  PyModuleDef_HEAD_INIT,
  "internals",
  "C internals",
  -1,
  mod_methods,
  NULL, NULL, NULL, NULL
};

PyMODINIT_FUNC
PyInit_internals(void)
{

  PyObject* m = NULL;
  if (PyType_Ready(&ProtocolType) < 0) return NULL;
  if (PyType_Ready(&RequestType) < 0) return NULL;
  if (PyType_Ready(&RouterType) < 0) return NULL;
  if (PyType_Ready(&MrhttpAppType) < 0) return NULL;
  if (PyType_Ready(&MemcachedClientType) < 0) return NULL;
  if (PyType_Ready(&MrqClientType) < 0) return NULL;
  if (PyType_Ready(&ResponseType) < 0) return NULL;
  if (PyType_Ready(&MemcachedProtocolType) < 0) return NULL;
  if (PyType_Ready(&MrqProtocolType) < 0) return NULL;

  m = PyModule_Create(&internals_module);
  if(!m) return NULL;

  Py_INCREF(&ProtocolType);
  PyModule_AddObject(m, "Protocol", (PyObject*)&ProtocolType);
  Py_INCREF(&RequestType);
  PyModule_AddObject(m, "Request", (PyObject*)&RequestType);
  Py_INCREF(&RouterType);
  PyModule_AddObject(m, "Router", (PyObject*)&RouterType);
  Py_INCREF(&MrhttpAppType);
  PyModule_AddObject(m, "MrhttpApp", (PyObject*)&MrhttpAppType);
  Py_INCREF(&ResponseType);
  PyModule_AddObject(m, "Response", (PyObject*)&ResponseType);
  Py_INCREF(&MemcachedClientType);
  PyModule_AddObject(m, "MemcachedClient", (PyObject*)&MemcachedClientType);
  Py_INCREF(&MrqClientType);
  PyModule_AddObject(m, "MrqClient", (PyObject*)&MrqClientType);

  Py_INCREF(&MemcachedProtocolType);
  PyModule_AddObject(m, "MemcachedProtocol", (PyObject*)&MemcachedProtocolType);
  Py_INCREF(&MrqProtocolType);
  PyModule_AddObject(m, "MrqProtocol", (PyObject*)&MrqProtocolType);

  return m;
}

