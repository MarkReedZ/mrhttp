
import asyncio
import os
import mrhttp

#TODO sighup reload server list from config..
#TODO cluster
#       
class MrqClient(mrhttp.CMrqClient):
  def __init__(self, servers, loop, queue_number=0, pool_size=2):
    self.conns = []
    self.next_conn = 0
    self.num_conns = 0  
    self.queue_number = queue_number

    if not isinstance(servers, list):
      raise ValueError("Mrq client takes a list of (host, port) servers")

    try:
      for s in servers:
        for c in range(pool_size):
          coro = loop.create_connection(lambda: mrhttp.MrqProtocol(self), s[0], s[1])
          loop.run_until_complete(coro)
    except ConnectionRefusedError:
      print("Could not connect to the mrq server(s)")
      exit(1)
    except Exception as e:
      print(e)
      exit(1)
      
  def setConnection(self, conn):
    self.conns.append(conn)
    self.num_conns += 1

  def getConnection(self):
    ret = self.conns[self.next_conn]
    self.next_conn = (self.next_conn+1) % self.num_conns
    return ret

