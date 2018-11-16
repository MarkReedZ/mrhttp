

# TODO
#   Add state callbacks - Try to attach to post fork loop
#import gc

import signal
import asyncio
import traceback
import socket
import os 
import sys
import multiprocessing
import faulthandler
import functools
from wsgiref.handlers import format_date_time
import inspect, copy
#from inspect import signature #getmodulename, isawaitable, signature, stack
#from prof import profiler_start,profiler_stop
import uuid, http.cookies


import mrhttp
from mrhttp import Protocol
from mrhttp.request import Request
from mrhttp import Response
from mrhttp import router

from mrhttp import MemcachedClient
from mrhttp import MrqClient
try:
  import mrjson as json
except ImportError:
  try:
    import ujson as json
  except:
    pass

#import mrmemcache

#import uvloop
#asyncio.set_event_loop_policy(uvloop.EventLoopPolicy())

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
    self.listeners = { "at_start":[], "at_end":[]}
    self._mc = None
    self._mrq = None
    self.uses_session = False
    self.uses_mrq = False
    self.err404 = "<html><head><title>404 Not Found</title></head><body><h1>Not Found</h1><p>The requested page was not found</p></body></html>"
   
  def setup(self, log_request=None, protocol_factory=None, debug=False):
    self._log_request = log_request
    self._protocol_factory = protocol_factory or Protocol
    self._debug = debug

  @property
  def loop(self):
    if not self._loop:
      self._loop = asyncio.new_event_loop()
    return self._loop

  def expand_requests(self):
    for x in range(len(self.requests)):
      self.requests.append(Request())

  def prehandler(self):
    pass

  # Decorator
  def on(self, event):
    """Call the decorated function on one of the following events:
       ["at_start","at_end"]
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
  def route(self, uri, methods=["GET"], options=[], type="html"):
    if "session" in options:
      self.uses_session = True
    if "mrq" in options:
      self.uses_mrq = True
    if not uri.startswith('/'): uri = '/' + uri
    def response(func): 
      self.router.add_route( func, uri, methods, options, type )
      return func
    return response

  def add_routes(self, rs):
    for r in rs:
      self.router.add_route( r[0], r[1], r[2], r[3], r[4] )


  def _get_idle_and_busy_connections(self):
    return \
      [c for c in self._connections if c.pipeline_empty], \
      [c for c in self._connections if not c.pipeline_empty]

  async def drain(self):
    await asyncio.sleep(0.1)
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


  def extend_request(self, handler, *, name=None, property=False):
      if not name:
          name = handler.__name__

      self._request_extensions[name] = (handler, property)

  def serve(self, *, sock, host, port):
      faulthandler.enable()

      #pr = cProfile.Profile()
      #pr.enable()
      #cProfile.runctx('test(num)', globals(), locals(), 'prof%d.prof' %num)

      #sock.setsockopt(socket.SOL_SOCKET, socket.SO_OOBINLINE, 0) #TODO uvloop .9.1 sets this

      #profiler_start(b"mrhttp.log")
      loop = self.loop
      asyncio.set_event_loop(loop)
      server_coro = loop.create_server( lambda: self._protocol_factory(self), sock=sock)
      server = loop.run_until_complete(server_coro)

      self.requests = [Request() for x in range(128)]
      self.bytes404 = self.err404.encode("utf-8")
      self.cinit()
      self.router.finalize_routes()
      self.router.setupRoutes()
      self._appStart() 
      self.trigger_event("at_start")

      if self.uses_mrq:
        #mrqconf = self.config.get("mrq", None)
        #if not mrqconf:
          #print("When using MrQ app.config['mrq'] must be set. Exiting")
          #exit(1)
        srvs = self.config.get("mrq", None)
        if type(srvs) != list or len(srvs) == 0 or type(srvs[0]) != tuple:
          print("When using MrQ app.config['mrq'] must be set to a list of (host,port) tuple pairs. Exiting")
          exit(1)
        self._mrq = MrqClient( srvs, self.loop) 

      if self.uses_session:
        srvs = self.config.get("memcache", None)
        if type(srvs) != list or len(srvs) == 0 or type(srvs[0]) != tuple:
          print("When using sessions app.config['memcache'] must be set to a list of (host,port) tuple pairs. Exiting")
          exit(1)
        self._mc = MemcachedClient( srvs, self.loop) 

      loop.add_signal_handler(signal.SIGTERM, loop.stop)

      # TODO Setup a reloader to reload if the python files change for ease of development?

      print('Accepting connections on http://{}:{}'.format(host, port))

      try:
        loop.run_forever()
      except KeyboardInterrupt:
        pass
      finally:
        #loop.run_until_complete(loop.shutdown_asyncgens())
        server.close()
        loop = asyncio.get_event_loop()
        loop.run_until_complete(server.wait_closed())
        loop.run_until_complete(self.drain())
        self.trigger_event("at_end")
        loop.close()
        for r in self.requests:
          r.cleanup()
        self.requests = None

    
        #for ref in gc.get_referrers(self.requests[0]):
          #if type(ref) == list:
            #print("list")
          #else:
            #print(ref)
        #print("DELME refcnt ", sys.getrefcount(self.requests[0]))
        #r = self.requests[0]
        #print("id requests ", id(self.requests))
        #rs = self.requests
        #self.requests = None
        #gc.collect()
        #print (gc.get_referrers(rs))
        #print("DELME refcnt ", sys.getrefcount(r))
        #for ref in gc.get_referrers(r):
          #if type(ref) == list:
            #print("list")
            #print("id ref ", id(ref))
          #else:
            #print(ref)


  # Update the response date string every few seconds
  def updateDateString(self):
    self.updateDate( format_date_time(None) )
    self.loop.call_later(5, self.updateDateString)


  def _appStart(self):
    self.loop.call_soon(self.updateDateString)


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

      signal.signal(signal.SIGINT, stop)
      signal.signal(signal.SIGTERM, stop)
      #signal.signal(signal.SIGHUP, stop)

      for _ in range(num_workers or 1):
          worker = multiprocessing.Process( target=self.serve, kwargs=dict(sock=sock, host=host, port=port))
          worker.daemon = True
          worker.start()
          workers.add(worker)

      sock.close() # Only the kids access the socket

      for worker in workers:
          try:
            worker.join()

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


  def setUserSessionAndCookies(self, request, userj, cookies={}, backend="memcached" ):
    if self._mc == None:
      raise ValueError("setUserSession called without memcached being setup")

    skey = uuid.uuid4().hex

    # Send the session cookie back to the user  
    c = http.cookies.SimpleCookie()
    c['mrsession'] = skey
    c['mrsession']['path'] = '/' #TODO
    c['mrsession']['expires'] = 12 * 30 * 24 * 60 * 60 # 1 year TODO arg
    for k in cookies.keys():
      c[k] = cookies[k]
      c[k]['path'] = '/' 
      c[k]['expires'] = 12 * 30 * 24 * 60 * 60 
    
    request.response.cookies = c

    self._mc.set( skey, userj )

app = Application()

