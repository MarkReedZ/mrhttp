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

from nats.aio.client import Client as NATS
from nats.aio.errors import ErrConnectionClosed, ErrTimeout, ErrNoServers

# wrk -t4 -c32 -d5s http://localhost:8080/ -s msgqueue.lua
nc = NATS()
app = Application()

@app.on('at_start')
async def dbsetup():
  bstr = "[1,2,3,4,5,6,7,8,9,10]".encode("utf-8")
  bstr = bstr*10
  #bstr = b"fart"
  app.bstr = bstr
  await nc.connect(io_loop=app.loop)

@app.route('/', type="text")
async def hello(request):
  await nc.publish("foo", app.bstr)
  return 'Hello World!'
   
app.run(debug=True, cores=4)
