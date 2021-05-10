import mrpacker
from pymemcache.client.base import Client
c = Client(('localhost', 11211))
c.set( "mrsession43709dd361cc443e976b05714581a7fb",mrpacker.pack({"user":"Mark"}) )
c = Client(('localhost', 11212))
c.set( "mrsession43709dd361cc443e976b05714581a7fb",mrpacker.pack({"user":"Mark"}) )
#print( c.get("mrsessionZZZZ9dd361cc443e976b05714581a7fb"))

import asyncmrcache, asyncio

def lcb(client):
  print("Lost conn")

async def run(loop):


  rc = await asyncmrcache.create_client( [("localhost",7000)], loop, lost_cb=lcb)

  await rc.set( b"mrsession43709dd361cc443e976b05714581a7fb",mrpacker.pack({"user":"Mark"}) )
  print(await rc.get(b"mrsession43709dd361cc443e976b05714581a7fb"))
  exit()



loop = asyncio.get_event_loop()
loop.run_until_complete(run(loop))
loop.close()

