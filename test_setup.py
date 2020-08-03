
import asyncio, time
import asyncmrcache, mrpacker
from pymemcache.client.hash import HashClient

import tracemalloc
tracemalloc.start()

import uvloop
asyncio.set_event_loop_policy(uvloop.EventLoopPolicy())

def lcb(client):
  pass
  #test( client.debug_data )

async def run(loop):

  rc = await asyncmrcache.create_client( [("localhost",7000)], loop, lost_cb=lcb)
  await rc.set(b"43709dd361cc443e976b05714581a7fb", mrpacker.pack({"user":"Mark"}))


  mc = HashClient([ ('127.0.0.1', 11211) ], ignore_exc=True)
  mc.set(b'43709dd361cc443e976b05714581a7fb', mrpacker.pack({"user":"Mark"}))
  print(mc.get(b'43709dd361cc443e976b05714581a7fb'))


  await asyncio.sleep(1)

if __name__ == '__main__':
  loop = asyncio.get_event_loop()
  loop.run_until_complete(run(loop))
  loop.close()

