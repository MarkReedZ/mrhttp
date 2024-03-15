
import asyncio, time
import asyncmrcache, mrpacker

import tracemalloc
tracemalloc.start()

import uvloop
asyncio.set_event_loop_policy(uvloop.EventLoopPolicy())

def lcb(client):
  print("Lost connection")

async def run(loop):

  rc = await asyncmrcache.create_client( [("localhost",7000)], loop, lost_cb=lcb)

  k = b"mrsession43709dd361cc443e976b05714581a7fb"
  user = {"user":"test","id":12 }

  await rc.set(k,mrpacker.pack(user))
  print(await rc.get(k))

  k = b"43709dd361cc443e976b05714581a7fb"
  await rc.set(k,mrpacker.pack(user))
  print(await rc.get(k))

  await rc.close()

if __name__ == '__main__':
  loop = asyncio.get_event_loop()
  loop.run_until_complete(run(loop))
  loop.close()
  print("DONE")


