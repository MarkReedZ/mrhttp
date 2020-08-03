import asyncio
import asyncpg

async def run():
  conn = await asyncpg.connect(user='benchmarkdbuser', password='benchmarkdbpass', database='hello_world', host='127.0.0.1')
  #for x in range(10000):
    #await conn.execute('''insert into World(id,randomNumber) VALUES ($1,$2);''',x,x)
  values = await conn.fetch('''SELECT * FROM World''')
  print (len(values))
  await conn.close()

loop = asyncio.get_event_loop()
loop.run_until_complete(run())
