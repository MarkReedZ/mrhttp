import multiprocessing
from mrhttp import Application
from mrhttp import Request
import mrhttp
import socket
import mrjson as json
import asyncpg, aiomcache
import weakref
import random
from jinja2 import Template
import asyncio
import http.cookies

app = Application()

app.config["memcache"] = [ ("127.0.0.1", 11211) ]

@app.on('at_start')
async def dbsetup():
  app.mc = aiomcache.Client("127.0.0.1",11211, loop=app.loop)

@app.on('at_end')
async def dbclose():
  app.mc.close()

@app.route('/mc', type="text")
async def pys(r):
  j = await app.mc.get(b"mrsession" + r.cookies["mrsession"].encode("utf-8"))
  return 'why is this'


@app.route('/hello', type="text")
def hello(r):
  return 'World!'

@app.route('/', type="text")
def index(r):

  x = r.args

  #print(r.body)
  #print(r.cookies)
  #print(r.mime_type )
  #print(r.encoding )
  #print(r.form)
  return 'Hello World!'

@app.route('/s',tools=['session'])
def session(req):
  if ( req.user["user"] == 'Mark' ): return "Mark"
  return "session"

@app.route('/form')
def parseForm(r):
  #print("test", r.mime_type)
  #print("test", r.form)
  return r.form["param2"]

#print("mc",hex(id(pys)))
#print("/",hex(id(hello)))
#print("form",hex(id(parseForm)))
#print("session",hex(id(session)))

   
app.run(cores=1)
#app.run('0.0.0.0', 8080, cores=multiprocessing.cpu_count())
#app.run('0.0.0.0', 8080, cores=multiprocessing.cpu_count())
