
import asyncio
import socket
import os
from mrhttp import MemcachedProtocol
import mrhttp

class MemcachedClient(mrhttp.CMemcachedClient):
  def __init__(self, servers, loop, pool_size=2):
    #self.conns = []
    #self.next_conn = []
    #self.num_conns = []
    super().__init__(len(servers))

    if not isinstance(servers, list):
      raise ValueError("Memcached client takes a list of (host, port) servers")

#TODO We need to add timeout to initial conn? and add a function for when we loose conns to re-establish

 #fut = asyncio.open_connection(host[1], 443, ssl=True)
        #try:
            ## Wait for 3 seconds, then raise TimeoutError
            #reader, writer = yield from asyncio.wait_for(fut, timeout=3)
        #except asyncio.TimeoutError:
            #print("Timeout, skipping {}".format(host[1]))
            #continue

    try:
      snum = 0
      for s in servers:
        #self.conns.append([])
        #self.num_conns.append(0)
        #self.next_conn.append(0)
        for c in range(pool_size):
          coro = loop.create_connection(lambda: MemcachedProtocol(self,snum), s[0], s[1])
          loop.run_until_complete(coro)
        snum += 1
    except ConnectionRefusedError:
      print("Could not connect to the memcached server(s)")
      exit(1)
    except Exception as e:
      print(e)
      exit(1)


  #def addConnection(self, conn, server):
    #self.conns[server].append(conn)
    #self.num_conns[server] += 1

  #def getConnection(self, server):
    #ret = self.conns[server][self.next_conn[server]]
    #self.next_conn[server] = (self.next_conn[server]+1) % self.num_conns[server]
    #return ret


