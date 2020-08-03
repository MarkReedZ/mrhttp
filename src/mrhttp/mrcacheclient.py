
import asyncio
import socket
import os
from mrhttp import MrcacheProtocol
import mrhttp

class MrcacheServer():
  def __init__(self, host, port):
    self.host = host
    self.port = port
    self.num_connections = 0
    self.reconnecting = False
    self.reconnect_attempts = 0

class MrcacheClient(mrhttp.CMrcacheClient):
  def __init__(self, servers, loop, pool_size=1):

    if not isinstance(servers, list):
      raise ValueError("Mrcache client takes a list of (host, port) servers")

    super().__init__(len(servers))
    self.loop = loop
    self.servers = []
    for s in servers:
      self.servers.append( MrcacheServer(s[0], s[1]) )

    try:
      snum = 0
      for s in self.servers:
        for c in range(pool_size):
          coro = loop.create_connection(lambda: MrcacheProtocol(self,snum), s.host, s.port)
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
        await self.loop.create_connection(lambda: MrcacheProtocol(self,srv), s.host, s.port)
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


