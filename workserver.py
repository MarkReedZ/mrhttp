import asyncio
import mrworkserver

async def callback(ws, msgs):
  for m in msgs:
    print(m)
    pass

ws = mrworkserver.WorkServer(callback=callback)

ws.run(host="127.0.0.1",port=7100)

