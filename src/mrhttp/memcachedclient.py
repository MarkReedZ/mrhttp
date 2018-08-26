
import asyncio
import socket
import os
from mrhttp import MemcachedProtocol
import mrhttp

class MemcachedServer():
  def __init__(self, host, port, pool_size=2):
    self.host = host
    self.port = port
    self.pool_size = 2
    self.num_connections = 0
    self.reconnecting = False
    self.reconnect_attempts = 0

class MemcachedClient(mrhttp.CMemcachedClient):
  def __init__(self, servers, loop, pool_size=2):

    if not isinstance(servers, list):
      raise ValueError("Memcached client takes a list of (host, port) servers")

    super().__init__(len(servers))
    self.loop = loop
    self.servers = []
    for s in servers:
      self.servers.append( MemcachedServer(s[0], s[1], pool_size) )

  # TODO conn timeout?
 #fut = asyncio.open_connection(host[1], 443, ssl=True)
        #try:
            ## Wait for 3 seconds, then raise TimeoutError
            #reader, writer = yield from asyncio.wait_for(fut, timeout=3)
        #except asyncio.TimeoutError:
            #print("Timeout, skipping {}".format(host[1]))
            #continue

    try:
      snum = 0
      for s in self.servers:
        for c in range(pool_size):
          coro = loop.create_connection(lambda: MemcachedProtocol(self,snum), s.host, s.port)
          loop.run_until_complete(coro)
        snum += 1
    except ConnectionRefusedError:
      print("Could not connect to the memcached server(s)")
      exit(1)
    except Exception as e:
      print(e)
      exit(1)

  async def reconnect(self, srv):
    s = self.servers[srv]
    while True:
      try:
        await self.loop.create_connection(lambda: MemcachedProtocol(self,srv), s.host, s.port)
        s.num_connections += 1
        s.reconnect_attempts = 0
        s.reconnecting = False  #TODO make sure open pool size number of conns
        return
      except ConnectionRefusedError:
        print("Reconnect failed to", s.host, "port", s.port)
        await asyncio.sleep(10)
      except Exception as e:
        print(e)
        await asyncio.sleep(30)

  def lost_connection(self, srv):
    s = self.servers[srv]
    s.num_connections -= 1
    print( "    Lost connection to",s.host, "port",s.port )
    if not s.reconnecting:
      s.reconnecting = True
      asyncio.ensure_future( self.reconnect(srv) )


