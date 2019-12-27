
import asyncio
import os
import mrhttp, mrpacker


#TODO
# sighup reload server list from config..
# When reconnecting after establishing a conn bring up pool_size conns. Right now its just 1
# When coming back wait 10 seconds before using that server.  Add to the conn map, but return error to any request.
#   This prevents two servers from working on the same partition at the same time

class MrqServer():
  def __init__(self, host, port, loop, pool_size=2):
    self.host = host
    self.port = port
    self.pool_size = 2
    self.num_connections = 0
    self.reconnecting = False
    self.reconnect_attempts = 0
    self.q = asyncio.Queue( loop=loop )
    
       
class MrqClient(mrhttp.CMrqClient):
  def __init__(self, servers, loop, pool_size=2):

    if not isinstance(servers, list):
      raise ValueError("Mrq client takes a list of (host, port) servers")

    super().__init__(len(servers))
    self.loop = loop
    self.pool_size = 2
    self.servers = []
    for s in servers:
      self.servers.append( MrqServer(s[0], s[1], loop, pool_size) )

    try:
      snum = 0
      for s in self.servers:
        for c in range(pool_size):
          coro = loop.create_connection(lambda: mrhttp.MrqProtocol(self,snum,s.q), s.host, s.port)
          loop.run_until_complete(coro)
          s.num_connections += 1
        snum += 1

    except ConnectionRefusedError:
      print("Could not connect to the MrQ server(s)")
      exit(1)
    except Exception as e:
      print(e)
      exit(1)

  async def create_connection(self, srv):
    s = self.servers[srv]
    #for c in range(pool_size):
    try:
      await loop.create_connection(lambda: mrhttp.MrqProtocol(self,srv,s.q), s.host, s.port)
    except ConnectionRefusedError:
      print("Could not connect to the MrQ server(s)")
      return False
    except Exception as e:
      print(e)
      return False

    return True

  async def reconnect(self, srv):
    s = self.servers[srv]
    while True:
      try:
        await self.loop.create_connection(lambda: mrhttp.MrqProtocol(self,srv,s.q), s.host, s.port)
        s.num_connections += 1
        s.reconnect_attempts = 0
        s.reconnecting = False  #TODO make sure open pool size number of conns
        print( "MrQ: Reconnected to",s.host, "port",s.port )
        if s.num_connections >= self.pool_size:
          return
      except ConnectionRefusedError:
        print("Reconnect failed to", s.host, "port", s.port)
        await asyncio.sleep(10)
      except Exception as e:
        print(e)
        await asyncio.sleep(30)


  async def get(self, slot, o):
    b = mrpacker.pack(o)
    srv = self._get(slot, b)
    return await self.servers[srv].q.get()
  
  def lost_connection(self, srv):
    s = self.servers[srv]
    s.num_connections -= 1
    print( "MrQ: Lost connection to",s.host, "port",s.port )
    if not s.reconnecting:
      s.reconnecting = True
      asyncio.ensure_future( self.reconnect(srv) )
      
    



          
