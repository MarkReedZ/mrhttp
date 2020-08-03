import multiprocessing
from mrhttp import Application
from mrhttp import Request
import mrhttp
import socket
import mrjson as json
import asyncpg
import weakref
import random
from jinja2 import Template
from mrq.client import Client



app = Application()

@app.on('at_start')
async def dbsetup():
  app.c = Client()
  await app.c.connect(io_loop=app.loop)
  bstr = "[1,2,3,4,5,6,7,8,9,10]".encode("utf-8")
  bstr = bstr*10
  bstr = b"fart"
  l = len(bstr)
  app.bstr = bstr
  app.l = l

  #app.pg_pool = await asyncpg.create_pool(database="world", loop=app.loop, min_size=2, max_size=2, max_inactive_connection_lifetime=0,init=pg_conn_init)
  #app.pg_pool = await asyncpg.create_pool(database="world", loop=app.loop, min_size=2, max_size=2, max_inactive_connection_lifetime=0)

@app.route('/', type="text")
async def hello(request):
  await app.c.push( 0, 0, app.bstr, app.l )
  return 'Hello World!'
   
app.run(debug=True, cores=4)
#app.run('0.0.0.0', 8080, cores=multiprocessing.cpu_count())
#app.run('0.0.0.0', 8080, cores=multiprocessing.cpu_count())
