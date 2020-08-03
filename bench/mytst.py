import multiprocessing
import mrhttp
import mrjson as json
import asyncio
import aiomysql
import random, os

app = mrhttp.Application()

@app.on('at_start')
async def dbsetup():
  app.my_pool = await aiomysql.create_pool(host='127.0.0.1', port=3306,
                           user='ch', password='skiz6oon',
                           db='tech', loop=app.loop)

@app.on('at_end')
async def dbsetup():
  app.my_pool.close()

@app.route('/json', type="json")
def j(r):
  return json.dumps({'message': 'Hello, world!'})

@app.route('/plaintext', type="text")
def p(r):
  return "Hello, world!"


@app.route('/db', type="json")
async def db(r):
  #id_ = int(random.random() * 500)
  id_  = mrhttp.randint(0,500)
  #id_ = 1
  async with app.my_pool.acquire() as conn:
     async with conn.cursor() as cur:
       await cur.execute("SELECT randomnumber FROM World WHERE id = %s", id_)
       r = await cur.fetchone()
  return json.dumps({'id': id_, 'randomNumber': r[0]})



app.run('0.0.0.0', 8080, cores=multiprocessing.cpu_count())
#app.run('0.0.0.0', 8080, cores=4)

