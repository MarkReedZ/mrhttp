
# curl -i --raw http://localhost:8080/mc -H "Cookie: foo=bar;"


import mrhttp
import asyncio
import aiomcache

app = mrhttp.Application()

@app.on('at_start')
async def dbsetup():
  app.mc = aiomcache.Client("127.0.0.1",11211, loop=app.loop)

@app.on('at_end')
async def dbclose():
  app.mc.close()

@app.route('/mc', type="text")
async def pysession(r):
  j = await app.mc.get(b"foo" + r.cookies["foo"].encode("utf-8"))
  print(j)
  return 'aiomcache test'


app.run(cores=1)
