import pkgutil
import inspect
import types 
import importlib

import tests

package = tests
for importer, modname, ispkg in pkgutil.iter_modules(package.__path__):
  if modname.startswith("test"):
    m = importlib.import_module('tests.'+modname)
    functions = inspect.getmembers(m, inspect.isfunction)
    for f in functions:
      if f[0] == 'setup':
        if f[1]():
          exit()
    for f in functions:
      if f[0].startswith("test_"):
        f[1]()
    for f in functions:
      if f[0] == 'teardown':
        f[1]()
 
    
