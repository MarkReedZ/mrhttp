

# TODO
#   Add state callbacks - Try to attach to post fork loop

import signal
import asyncio
import traceback
import socket #, cProfile
import os #, io, pstats
import sys
import multiprocessing
import faulthandler
from functools import partial
from wsgiref.handlers import format_date_time
from functools import wraps
import inspect, copy
from inspect import signature
#from inspect import signature #getmodulename, isawaitable, signature, stack
#from prof import profiler_start,profiler_stop

import mrhttp
from mrhttp import Protocol
from mrhttp.request import Request
from mrhttp import Response
from mrhttp import router

from mrhttp import Client
import aiomcache
import mrjson
#import mrmemcache

import uvloop
asyncio.set_event_loop_policy(uvloop.EventLoopPolicy())

signames = {
    int(v): v.name for k, v in signal.__dict__.items()
    if isinstance(v, signal.Signals)}

class Application:
    def __init__(self, *, reaper_settings=None, log_request=None,
                 protocol_factory=None, debug=False):
      self._loop = None
      self._connections = set()
      self._reaper_settings = reaper_settings or {}
      self._error_handlers = []
      self._log_request = log_request
      self._request_extensions = {}
      self._protocol_factory = protocol_factory or Protocol
      self._debug = debug
      self.request = Request()
      self.response = Response()
      self.tasks = []
      self.static_routes = []
      self.routes = []
      self.config = {}
      self.mc = None
      

    @property
    def loop(self):
      if not self._loop:
        self._loop = asyncio.new_event_loop()
      return self._loop

    def prehandler(self):
      pass

    def add_route(self, handler, uri, methods=['GET'], tools=[],type="html"):
      if asyncio.iscoroutinefunction(handler) and router.is_pointless_coroutine(handler):
        handler = router.coroutine_to_func(handler)

      numArgs = uri.count("{}")
      if numArgs != len(signature(handler).parameters):
        print( "ERROR: Number of function args {} not equal to route args {}".format( numArgs, len(signature(handler).parameters)) )
        print( signature(handler), " vs ", uri )
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
      print (r["type"])
      if "session" in tools: r["session"] = True
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
        #if uri.startswith("/{"):
        self.routes.append( r )
        #else:
          #r["prefix"] = uri.split("{")[0]
          #r["prefix_len"] = len(r["prefix"])
          #self.prefix_routes.append( r )


    # Decorator
    def route(self, uri, methods=["GET"], tools=[], type="html"):
      if not uri.startswith('/'): uri = '/' + uri
      def response(func): 
        if "session" in tools:
          @wraps(func)
          async def wrapper(*args, **kwds):
            try:
              uj = await self.mc.get(b"session_key")
              self.request.user = mrjson.loads(uj)
            except Exception as e:
              print(e)
            return func(*args, **kwds)
          self.add_route( wrapper, uri, methods, tools, type )
          return wrapper
        else:
          self.add_route( func, uri, methods, tools, type )
          return func
      return response


    def bad_request(self, error):
        error = error.encode('utf-8')

        response = [
            'HTTP/1.0 400 Bad Request\r\n',
            'Content-Type: text/plain; charset=utf-8\r\n',
            'Content-Length: {}\r\n\r\n'.format(len(error))]

        return ''.join(response).encode('utf-8') + error

    def default_request_logger(self, request):
        print(request.remote_addr, request.method, request.path)

    def add_error_handler(self, typ, handler):
        self._error_handlers.append((typ, handler))

    def default_error_handler(self, request, exception):
        #if isinstance(exception, RouteNotFoundException):
            #return request.Response(code=404, text='Not Found')
        #if isinstance(exception, asyncio.CancelledError):
            #return request.Response(code=503, text='Service unavailable')

        tb = traceback.format_exception(
            None, exception, exception.__traceback__)
        tb = ''.join(tb)
        print(tb, file=sys.stderr, end='')
        return request.Response(
            code=500,
            text=tb if self._debug else 'Internal Server Error')

    def error_handler(self, request, exception):
        for typ, handler in self._error_handlers:
            if typ is not None and not isinstance(exception, typ):
                continue

            try:
                return handler(request, exception)
            except:
                print('-- Exception in error_handler occured:')
                traceback.print_exc()

            print('-- while handling:')
            traceback.print_exception(None, exception, exception.__traceback__)
            return request.Response(
                code=500, text='Internal Server Error')

        return self.default_error_handler(request, exception)

    def _get_idle_and_busy_connections(self):
        return []
            #[c for c in self._connections if c.pipeline_empty], \
            #[c for c in self._connections if not c.pipeline_empty]

    async def drain(self):
      for c in self._connections:
        if c.transport:
          c.transport.close()
      # TODO idle / busy

    def extend_request(self, handler, *, name=None, property=False):
        if not name:
            name = handler.__name__

        self._request_extensions[name] = (handler, property)

    def serve(self, *, sock, host, port, reloader_pid):
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

        self._appStart()
        #self.mc = Client("127.0.0.1",7000, self.loop)
        #z = loop.getLoopPtr()
        #print(hex(z))
        #mrhttp.init(z)
        #self.request.add_done_callback(self)

        loop.add_signal_handler(signal.SIGTERM, loop.stop)
        loop.add_signal_handler(signal.SIGINT, loop.stop)

        #if reloader_pid:
            #from japronto.reloader import ChangeDetector
            #detector = ChangeDetector(loop)
            #detector.start()

        print('Accepting connections on http://{}:{}'.format(host, port))

        try:
          loop.run_forever()
        finally:
          print("loop done")          
          server.close()
          #pr.disable()
          #s = io.StringIO()
          #sortby = 'cumulative'
          #ps = pstats.Stats(pr, stream=s).sort_stats(sortby)
          #ps.print_stats()
          #print(s.getvalue())
          #profiler_stop()
          loop.run_until_complete(server.wait_closed())
          loop.run_until_complete(self.drain())
          #self._reaper.stop()
          loop.close()


    # Update the response date string every second
    def updateDateString(self):
      self.response.updateDate( format_date_time(None) )
      self.loop.call_later(1, self.updateDateString)


    def appStart(self):
      pass

    def _appStart(self):
      #self.loop.call_soon(self.updateDateString)
      if "memcache" in self.config:
        self.mc = aiomcache.Client(self.config["memcache"][0], self.config["memcache"][1], loop=self.loop, pool_size=4)
      self.appStart()


    def _run(self, *, host, port, num_workers=None, reloader_pid=None, debug=None):
        self._debug = debug or self._debug
        if self._debug and not self._log_request:
            self._log_request = self._debug

        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock.bind((host, port))
        os.set_inheritable(sock.fileno(), True)

        self.routes.sort(key=lambda x: x["sortlen"],reverse=True)

        workers = set()

        terminating = False

        def stop(sig, frame):
            nonlocal terminating
            if reloader_pid and sig == signal.SIGHUP:
                print('Reload request received')
            elif not terminating:
                terminating = True
                print('Termination request received')
            for worker in workers:
                worker.terminate()

        signal.signal(signal.SIGINT, stop)
        signal.signal(signal.SIGTERM, stop)
        signal.signal(signal.SIGHUP, stop)

        for _ in range(num_workers or 1):
            worker = multiprocessing.Process( target=self.serve, kwargs=dict(sock=sock, host=host, port=port, reloader_pid=reloader_pid))
            worker.daemon = True
            worker.start()
            workers.add(worker)

        sock.close() # Only the kids access the socket

        for worker in workers:
            worker.join()

            if worker.exitcode > 0:
                print('Worker exited with code {}'.format(worker.exitcode))
            elif worker.exitcode < 0:
                try:
                    signame = signames[-worker.exitcode]
                except KeyError:
                    print(
                        'Worker crashed with unknown code {}!'
                        .format(worker.exitcode))
                else:
                    print('Worker crashed on signal {}!'.format(signame))

    def run(self, host='0.0.0.0', port=8080, *, cores=None, reload=False, debug=False):

        reloader_pid = None
        if reload:
            #if '_JAPR_RELOADER' not in os.environ:
                #from japronto.reloader import exec_reloader
                #exec_reloader(host=host, port=port, worker_num=cores)
            #else:
            reloader_pid = int(os.environ['_JAPR_RELOADER'])

        self._run( host=host, port=port, num_workers=cores, reloader_pid=reloader_pid, debug=debug)


