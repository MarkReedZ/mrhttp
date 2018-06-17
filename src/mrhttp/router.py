import dis
import functools
import types
import mrhttp, asyncio, inspect

FLAG_COROUTINE = 128

def is_simple(fun):
    """A heuristic to find out if a function is simple enough."""
    seen_load_fast_0 = False
    seen_load_response = False
    seen_call_fun = False

    for instruction in dis.get_instructions(fun):
        if instruction.opname == 'LOAD_FAST' and instruction.arg == 0:
            seen_load_fast_0 = True
            continue

        if instruction.opname == 'LOAD_ATTR' \
           and instruction.argval == 'Response':
            seen_load_response = True
            continue

        if instruction.opname.startswith('CALL_FUNCTION'):
            if seen_call_fun:
                return False

            seen_call_fun = True
            continue

    return seen_call_fun and seen_load_fast_0 and seen_load_response


def is_pointless_coroutine(fun):
    for instruction in dis.get_instructions(fun):
        if instruction.opname in ('GET_AWAITABLE', 'YIELD_FROM'):
            return False

    return True


def coroutine_to_func(f):
    # Based on http://stackoverflow.com/questions/13503079/
    # how-to-create-a-copy-of-a-python-function
    oc = f.__code__
    code = types.CodeType(
        oc.co_argcount, oc.co_kwonlyargcount, oc.co_nlocals, oc.co_stacksize,
        oc.co_flags & ~FLAG_COROUTINE,
        oc.co_code, oc.co_consts, oc.co_names, oc.co_varnames, oc.co_filename,
        oc.co_name, oc.co_firstlineno, oc.co_lnotab, oc.co_freevars,
        oc.co_cellvars)
    g = types.FunctionType(
        code, f.__globals__, name=f.__name__, argdefs=f.__defaults__,
        closure=f.__closure__)
    g = functools.update_wrapper(g, f)
    g.__kwdefaults__ = f.__kwdefaults__

    return g

class Router(mrhttp.CRouter):

  def __init__(self):
    self.routes = []
    self.static_routes = []
    self.func_namemap= {}

    super().__init__()

  def finalize_routes(self):
    self.routes.sort(key=lambda x: x["sortlen"],reverse=True)

  def add_route(self, handler, uri, methods=['GET'], tools=[],type="html"):

    if handler.__name__ in self.func_namemap:
      raise ValueError("You have page handlers with the same name - use unique function names")
    self.func_namemap[ handler.__name__ ] = 1

    #print("DELME add route func ",hex(id(handler)))
    #if asyncio.iscoroutinefunction(handler) and is_pointless_coroutine(handler):
      #handler = coroutine_to_func(handler)
    #print("DELME add route func after coro change ",hex(id(handler)))

    numArgs = uri.count("{}")
    if numArgs == 0 and len(inspect.signature(handler).parameters) != 1:
      raise ValueError("Page handler must take a request object as an argument")
    if numArgs != (len(inspect.signature(handler).parameters)-1):
      print( "ERROR: Number of function args {} not equal to route args {}".format( numArgs, len(inspect.signature(handler).parameters)) )
      print( inspect.signature(handler), " vs ", uri )
      raise ValueError("Number of route arguments not equal to function arguments")
     
    r = {}
    r["handler"] = id(handler)
    r["iscoro"]  = asyncio.iscoroutinefunction(handler)
    r["path"]    = uri
    r["methods"] = methods
    r["sortlen"] = len(uri.replace("{}",""))
    r["type"] = 0
    if type == "text": r["type"] = 1
    if type == "json": r["type"] = 2
    if "session" in tools: r["session"] = True
    if "mrq" in tools: r["mrq"] = True
    # Static routes
    if not "{" in uri:
      self.static_routes.append( r )
    else:
      spl = uri.split("/")[1:]
      if uri.endswith("/"):
        spl = uri.split("/")[1:-1]
      seglens = []
      segs = []
      for s in spl:
        segs.append( s.encode("utf-8") )
        seglens.append( len(s) )
      r["segs"] = segs
      r["num_segs"] = len(segs)
      self.routes.append( r )
