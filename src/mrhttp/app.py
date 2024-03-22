

# TODO
#   Add state callbacks - Try to attach to post fork loop
#import gc

import signal
import asyncio
import traceback
import socket
import os, sys, random, mrpacker, time
from glob import glob
import multiprocessing
import faulthandler
import functools
from wsgiref.handlers import format_date_time
import inspect, copy
#from inspect import signature #getmodulename, isawaitable, signature, stack
#from prof import profiler_start,profiler_stop
import http.cookies

import mrhttp
from mrhttp import Protocol
from mrhttp.request import Request
from mrhttp import Response
from mrhttp import router

from mrhttp import MemcachedClient
from mrhttp import MrqClient
from mrhttp import MrcacheClient
try:
  import mrjson as json
except ImportError:
  try:
    import ujson as json
  except:
    pass

#import mrmemcache

import uvloop
asyncio.set_event_loop_policy(uvloop.EventLoopPolicy())

signames = {
    int(v): v.name for k, v in signal.__dict__.items()
    if isinstance(v, signal.Signals)}

class Application(mrhttp.CApp):
  _instance = None
  def __new__(cls, *args, **kwargs):
    if not cls._instance:
      cls._instance = super(Application, cls).__new__(cls, *args, **kwargs)
    return cls._instance

  def __init__(self):
    self._loop = None
    self._connections = set()
    self._error_handlers = []
    self._request_extensions = {}
    self._log_request = None
    self._protocol_factory = Protocol
    self._debug = False
    self.request = Request()
    self.requests = None
    self.response = Response()
    self.router = router.Router()
    self.tasks = []
    self.config = {}
    self.listeners = { "at_start":[], "at_end":[], "after_start":[]}
    self._mc = None
    self._mrq = None
    self._mrq2 = None
    self._mrc = None
    self.static_cached_files = []
    self.static_cached_update_time = 10
    self.session_backend = "memcached"
    self.uses_session = False
    self.uses_mrq = False
    self.uses_mrq2 = False
    self.err404 = "<html><head><title>404 Not Found</title></head><body><h1>Not Found</h1><p>The requested page was not found</p></body></html>"
    self.err400 = "<html><head><title>400 Bad Request</title></head><body><p>Invalid Request</p></body></html>"
   
  def setup(self, log_request=None, protocol_factory=None, debug=False):
    self._log_request = log_request
    self._protocol_factory = protocol_factory or Protocol
    self._debug = debug

  @property
  def loop(self):
    if not self._loop:
      self._loop = asyncio.get_event_loop()
    return self._loop

  def expand_requests(self):
    for x in range(len(self.requests)):
      self.requests.append(Request())

  def prehandler(self):
    pass

  # Decorator
  def on(self, event):
    """Call the decorated function on one of the following events:
       ["at_start","at_end", "after_start"]
    """
    def decorator(func):
      self.listeners[event].append(func)
      return func
    return decorator

  def trigger_event(self, event):
    for func in self.listeners[event]:
      result = func()
      if inspect.isawaitable(result):
        self.loop.run_until_complete(result)

  # Decorator
  def route(self, uri, methods=["GET"], options=[], _type="html"):
    if "session" in options:
      self.uses_session = True
    if "mrq" in options:
      self.uses_mrq = True
    if "mrq2" in options:
      self.uses_mrq2 = True

    if not uri.startswith('/'): uri = '/' + uri

    def response(func): 
      self.router.add_route( func, uri, methods, options, _type )
      #self.router.add_route( func, uri, params )
      return func
    return response

  def add_routes(self, rs):
    for r in rs:
      params = {}
      params["methods"] = r[2]
      params["options"] = r[3]
      params["type"]    = r[4]
      self.router.add_route( r[0], r[1], params )

  #TODO Size limit on files?
  def static_cached_timer(self):
    ts = time.time() # Avoid race 
    for item in self.static_cached_files:
      fn = item[1]
      if os.path.getmtime(fn) > self.static_cached_timestamp:
        with open(fn, 'rb') as f:
          b = f.read()
          self.router.update_cached_route( [item[0], b] )
    self.static_cached_timestamp = ts
    self.loop.call_later(self.static_cached_update_time, self.static_cached_timer)  # Default 10 seconds
        
       
  #TODO Use brotli'd files - if [path].br exists use that instead
  def static_cached(self, root, directory):
    def removeprefix( prefix, text ):
      if text.startswith(prefix):
          return text[len(prefix):]

    if not os.path.isdir(directory):
      print("WARNING: app.static_cached root dir does not exist")
      return
    files = glob(os.path.join(directory, '**', '*'), recursive=True)
    self.static_cached_timestamp = time.time()
    for fn in files:
      if os.path.isdir(fn): continue
      with open(fn, 'rb') as f:
        b = f.read()
        print("DELME loaded",fn," bytes ",len(b))
      

      if not root.startswith('/'): root = '/'+root
      uri = root+removeprefix(directory, fn)
      self.static_cached_files.append( [uri,fn] )
      self.router.add_cached_route( uri, b )

  def _get_idle_and_busy_connections(self):
    return \
      [c for c in self._connections if c.pipeline_empty], \
      [c for c in self._connections if not c.pipeline_empty]

  async def drain(self):
    #await asyncio.sleep(0.1)
    idle, busy = self._get_idle_and_busy_connections()

    for c in idle:
      c.transport.close()

    if idle or busy:
      print('Draining connections...')
    else:
      return

    if idle:
      print('{} idle connections closed immediately'.format(len(idle)))
    if busy:
      print('{} connections busy, read-end closed'.format(len(busy)))

    for x in range(5, 0, -1):
      await asyncio.sleep(1)
      idle, busy = self._get_idle_and_busy_connections()
      for c in idle:
        c.transport.close()
      if not busy:
        break
      else:
        print( "{} seconds remaining, {} connections still busy" .format(x, len(busy)))

    _, busy = self._get_idle_and_busy_connections()
    if busy:
      print('Forcefully killing {} connections'.format(len(busy)))
    for c in busy:
      c.pipeline_cancel()
    #await asyncio.sleep(2.3)


  def extend_request(self, handler, *, name=None, property=False):
      if not name:
          name = handler.__name__

      self._request_extensions[name] = (handler, property)

  def serve(self, *, sock, host, port, loop, run_async=False):
      faulthandler.enable()

      #pr = cProfile.Profile()
      #pr.enable()
      #cProfile.runctx('test(num)', globals(), locals(), 'prof%d.prof' %num)

      #sock.setsockopt(socket.SOL_SOCKET, socket.SO_OOBINLINE, 0) #TODO uvloop .9.1 sets this

      #profiler_start(b"mrhttp.log")

      if not loop:
        loop = self.loop
        asyncio.set_event_loop(loop)
      else:
        self._loop = loop

      self.session_backend_type = 1
      if self.session_backend == "mrworkserver":
        self.session_backend_type = 2
      elif self.session_backend == "mrcache":
        self.session_backend_type = 3

      self.requests = [Request() for x in range(128)] # TODO multiple connections each have 128 req objects?
      self.cinit()
      self.router.finalize_routes()
      self.router.setupRoutes()
      self._appStart() 

      if self.uses_mrq:
        srvs = self.config.get("mrq", None)
        if not srvs:
          print("When using MrQ app.config['mrq'] must be set. Exiting")
          exit(1)
        if type(srvs) != list or len(srvs) == 0 or type(srvs[0]) != tuple:
          print("When using MrQ app.config['mrq'] must be set to a list of (host,port) tuple pairs. Exiting")
          exit(1)
        self._mrq = MrqClient( srvs, self.loop) 
      if self.uses_mrq2:
        srvs = self.config.get("mrq2", None)
        if not srvs:
          print("When using mrq2 app.config['mrq2'] must be set. Exiting")
          exit(1)
        if type(srvs) != list or len(srvs) == 0 or type(srvs[0]) != tuple:
          print("When using MrQ app.config['mrq'] must be set to a list of (host,port) tuple pairs. Exiting")
          exit(1)
        self._mrq2 = MrqClient( srvs, self.loop) 

      if self.uses_session:

        self.setupSessionClient()


      self.trigger_event("at_start")

      server_coro = loop.create_server( lambda: self._protocol_factory(self), sock=sock)
      if run_async: 
        return server_coro

      # Try except here?
      server = loop.run_until_complete(server_coro)

      print('Accepting connections on http://{}:{}'.format(host, port))

      self.trigger_event("after_start")

      loop.add_signal_handler(signal.SIGTERM, loop.stop)
      #loop.add_signal_handler(signal.SIGINT,  loop.stop)

      try:
        loop.run_forever()
      except KeyboardInterrupt:
        pass
      finally:
        loop.run_until_complete(loop.shutdown_asyncgens())
        loop.run_until_complete(self.drain())
        self._connections.clear()
        server.close()
        loop = asyncio.get_event_loop()
        loop.run_until_complete(server.wait_closed())
        self.trigger_event("at_end")
        loop.close()
        for r in self.requests:
          r.cleanup()
        self.requests = None
    
  # Update the response date string every few seconds
  def updateDateString(self):
    self.updateDate( format_date_time(None) )
    self.loop.call_later(1, self.updateDateString)


  def _appStart(self):
    self.loop.call_soon(self.updateDateString)
    self.loop.call_soon(self.static_cached_timer)


  def _run(self, *, host, port, num_workers=None, debug=None):
      self._debug = debug or self._debug
      if self._debug and not self._log_request:
          self._log_request = self._debug

      sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
      sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
      sock.bind((host, port))
      os.set_inheritable(sock.fileno(), True)

      workers = set()

      terminating = False

      def stop(sig, frame):
        nonlocal terminating
        #if reloader_pid and sig == signal.SIGHUP:
          #print('Reload request received')
        if not terminating:
          terminating = True
          print('Termination request received')
        for worker in workers:
          worker.terminate()

      def appstop():
        nonlocal terminating
        terminating = True
        for worker in workers:
          worker.terminate()

      self.stop = appstop
      signal.signal(signal.SIGTERM, stop)
      #signal.signal(signal.SIGHUP, stop)

      for _ in range(num_workers or 1):
          worker = multiprocessing.Process( target=self.serve, kwargs=dict(sock=sock, host=host, port=port, loop=None))
          worker.daemon = True
          worker.start()
          workers.add(worker)

      sock.close() # Only the kids access the socket

      for worker in workers:
          try:
            worker.join()
            print("worker stopped")
            if worker.exitcode > 0:
              print('Worker exited with code {}'.format(worker.exitcode))
            elif worker.exitcode < 0:
              try:
                signame = signames[-worker.exitcode]
              except KeyError:
                print( 'Worker crashed with unknown code {}!' .format(worker.exitcode))
              else:
                print('Worker crashed on signal {}!'.format(signame))
          except KeyboardInterrupt:
            pass
      

  def run(self, host='0.0.0.0', port=8080, *, cores=None, debug=False):

    # TODO reloader?
    self._run( host=host, port=port, num_workers=cores, debug=debug)


  def start_server(self, *, host='0.0.0.0', port=8080, loop=None):

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind((host, port))
    os.set_inheritable(sock.fileno(), True)

    if not loop:
      loop = self.loop
    
    return self.serve(sock=sock, host=host, port=port, loop=loop, run_async=True)


  def logoutUser(self, request):
    c = http.cookies.SimpleCookie()
    c['mrsession'] = ""
    c['mrsession']['max-age'] = 0
    request.response.cookies = c

  # TODO LOL
  def setUserSessionAndCookies(self, request, user_id, user, cookies=http.cookies.SimpleCookie(), 
                               expiration=12*30*24*60*60, json=False ):
    if self.session_backend_type == 1 and self._mc == None:
      raise ValueError("setUserSession called without memcached being setup")
    if self.session_backend_type == 2 and self._mrq== None:
      raise ValueError("setUserSession called without mrworkserver being setup")
    if self.session_backend_type == 3 and self._mrc== None:
      raise ValueError("setUserSession called without mrcache being setup")

    a = random.getrandbits(64)
    b = random.getrandbits(64)
    c = random.getrandbits(64)
    k = mrhttp.to64(a) + mrhttp.to64(b) + mrhttp.to64(c)
    k = k[:32]
    while len(k) < 32:
      k += mrhttp.to64( random.getrandbits(6) )

    userk = ""
    numbits = user_id.bit_length() 
    if numbits == 0:
      numbits += 1
    while numbits > 0:
      userk = mrhttp.to64( user_id & 0x1F ) + userk
      user_id >>= 5
      numbits -= 5
    userk = userk + mrhttp.to64( 0x20 | random.getrandbits(5) ) 

    skey = userk + k[len(userk):]

    # Send the session cookie back to the user  
    c = cookies
    c['mrsession'] = skey
    c['mrsession']['path'] = '/' 
    c['mrsession']['expires'] = expiration
    
    request.response.cookies = c

    if self.session_backend_type == 1: # Memcached
      if json:
        self._mc.set( skey, json.dumpb(user) )
      else:
        self._mc.set( skey, mrpacker.pack(user) )
    elif self.session_backend_type == 2: # MrWorkServer 
      self._mrq.set( user_id, mrpacker.pack( [skey, user]) )
    elif self.session_backend_type == 3: # MrCache
      self._mrc.set( skey, mrpacker.pack( user ) )

    return skey

  def setupSessionClient(self):
    if self.session_backend == "memcached":
      srvs = self.config.get("memcache", None)
      if type(srvs) != list or len(srvs) == 0 or type(srvs[0]) != tuple:
        print("When using sessions app.config['memcache'] must be set to a list of (host,port) tuple pairs. Exiting")
        exit(1)
      self._mc = MemcachedClient( srvs, self.loop) 
      self._session_client = self._mc
    elif self.session_backend == "mrworkserver":
      if not self._mrq:
        mrqconf = self.config.get("mrq", None)
        if not mrqconf:
          print("When using mrworkserver as a session backend app.config['mrq'] must be set. Exiting")
          exit(1)
        srvs = self.config.get("mrq", None)
        if type(srvs) != list or len(srvs) == 0 or type(srvs[0]) != tuple:
          print("When using mrworkserver app.config['mrq'] must be set to a list of (host,port) tuple pairs. Exiting")
          exit(1)
        self._mrq = MrqClient( srvs, self.loop) 
      self._session_client = self._mrq
    elif self.session_backend == "mrcache":
      if not self._mrc:
        conf = self.config.get("mrcache", None)
        if not conf:
          print("When using mrcache as a session backend app.config['mrcache'] must be set. Exiting")
          exit(1)
        srvs = self.config.get("mrcache", None)
        if type(srvs) != list or len(srvs) == 0 or type(srvs[0]) != tuple:
          print("When using mrcache app.config['mrcache'] must be set to a list of (host,port) tuple pairs. Exiting")
          exit(1)
        self._mrc = MrcacheClient( srvs, self.loop) 
      self._session_client = self._mrc

app = Application()

