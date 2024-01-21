import multiprocessing
import mrhttp
import mrjson as json
import asyncpg
import random, os

app = mrhttp.Application()

@app.on('at_start')
async def dbsetup():
  return
  app.pg_pool = await asyncpg.create_pool(database="hello_world", loop=app.loop, max_size=100,
                                          password=os.getenv('PGPASS', 'benchmarkdbpass'),
                                          host='127.0.0.1',
                                          port='5432',
                                          user=os.getenv('PGUSER', 'benchmarkdbuser'))

@app.on('at_end')
async def dbsetup():
  print("tst at_end")
  await app.pg_pool.close()
  print("tst at_end after close")

@app.route('/json',_type="json")
def j(r):
  return json.dumps({'message': 'Hello, world!'})

@app.route('/plaintext',_type="text")
def p(r):
  return "Hello, world!"

@app.route('/db')
async def db(r):
  #id_ = random.randint(1, 500)
  id_ = int(random.random() * 500)
  #id_ = 1
  async with app.pg_pool.acquire() as conn:
    r = await conn.fetchval('SELECT randomnumber FROM world WHERE id = $1', id_)
  return json.dumps({'id': id_, 'randomNumber': r})



app.run('127.0.0.1', 8080, cores=multiprocessing.cpu_count())
#app.run('0.0.0.0', 8080, cores=4)

