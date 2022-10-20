import dis
import functools
import types
import mrhttp, asyncio, inspect

class Router(mrhttp.CRouter):

  def __init__(self):
    self.routes = []
    self.static_routes = []
    self.func_namemap= {}
    self.funcs = []

    super().__init__()

  def finalize_routes(self):
    self.routes.sort(key=lambda x: x["sortlen"],reverse=True)

  def add_route(self, handler, uri, methods=['GET'], options=[],_type="html"):

    if handler.__name__ in self.func_namemap:
      raise ValueError("You have page handlers with the same name - use unique function names")
    self.func_namemap[ handler.__name__ ] = 1

    self.funcs.append(handler) # Store the handler to increment the ref count as we save the memory location below

    numArgs = uri.count("{}") + uri.count("{num}")
    if numArgs == 0 and len(inspect.signature(handler).parameters) == 0:
      raise ValueError("Page handler must take a request object as an argument")
    if numArgs != (len(inspect.signature(handler).parameters)-1):
      print( "ERROR: Number of function args {} not equal to route args {}".format( len(inspect.signature(handler).parameters), numArgs) )
      print( "  ", inspect.signature(handler), " vs ", uri )
      raise ValueError("Number of route arguments not equal to function arguments")
   
    # TODO Add {num}  change sortlen 
     
    r = {}
    r["handler"] = id(handler)
    r["iscoro"]  = asyncio.iscoroutinefunction(handler)
    r["path"]    = uri
    r["methods"] = methods
    r["sortlen"] = len(uri.replace("{}","").replace("{num}",""))
    r["type"] = 0
    if _type == "text": r["type"] = 1
    if _type == "json": r["type"] = 2
    if _type == "mrp": r["type"] = 3
    #r["user_key"] = optiondict.get("append_user_key",None)
    if "session" in options: r["session"] = True
    if "mrq" in options: r["mrq"] = True
    if "append_user" in options: r["append_user"] = True
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


