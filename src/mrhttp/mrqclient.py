
import asyncio
import os
import mrhttp

#TODO sighup reload server list from config..
#TODO cluster
       
class MrqClient(mrhttp.CMrqClient):
  def __init__(self, servers, loop, pool_size=2):
    super().__init__(len(servers))

    if not isinstance(servers, list):
      raise ValueError("Mrq client takes a list of (host, port) servers")

    try:
      snum = 0
      for s in servers:
        for c in range(pool_size):
          coro = loop.create_connection(lambda: mrhttp.MrqProtocol(self,snum), s[0], s[1])
          loop.run_until_complete(coro)
        snum += 1

    except ConnectionRefusedError:
      print("Could not connect to the MrQ server(s)")
      exit(1)
    except Exception as e:
      print(e)
      exit(1)
      
