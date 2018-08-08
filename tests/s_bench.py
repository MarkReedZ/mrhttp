from mrhttp import Application
from mrhttp import Request
import mrhttp
import socket
import mrasyncmc
import mrjson

import tenjin
tenjin.set_template_encoding('utf-8')
from tenjin.helpers import *
engine = tenjin.Engine(path=['tests/templates'])


app = Application()
app.config["memcache"] = [ ("127.0.0.1", 11211) ]


@app.on('at_start')
async def dbsetup():
  app.mcc = await mrasyncmc.create_client([("127.0.0.1",11211),("127.0.0.1",11212)], loop=app.loop,pool_size=4)
@app.on('at_end')
async def dbclose():
  await app.mcc.close()

@app.route('/pys', type="text")
async def pys(r):
  j = await app.mcc.get(b"mrsession" + r.cookies["mrsession"].encode("utf-8"))
  return 'py session'



@app.route('/')
def index(r):
  return 'Hello World!'

@app.route('/print/{}')
def pr(r,foo):
  return foo

@app.route('/printPostBody')
def printPostBody(r):
  return r.body
@app.route('/printCookies')
def printCookies(r):
  return str(r.cookies)

@app.route('/foo')
def foo(r):
  return 'foodff'

@app.route('/123456789012345678901234567890/testing')
def testing(r):
  return 'Hello World!'

@app.route('/404/')
def notFound(r):
  raise mrhttp.HTTPError(404)
  return 'Hello World!'

@app.route('/500/')
def error500(r):
  x = {}
  z = x["foo"]
  return 'Hello World!'

@app.route('/foo/{}')
def foo1(r, foo):
  return 'foo1'

@app.route('/foo/{}/{}')
def foo2(r, foo, bar):
  return 'foo2'

@app.route('/foo/bar/{}')
def foo3(r, foo):
  return 'foo3'

@app.route('/{}/bar/{}')
def barmid(r, foo, bar):
  return 'barmid'

@app.route('/content')
def content(r):
  print( r.mime_type )
  print( r.encoding )
  return r.mime_type

@app.route('/form')
def parseForm(r):
  return r.form["param2"]

#@app.route('/json')
#def parseJson(r):
  #return r.form["param2"]

@app.route('/s',tools=['session'])
def session(req):
  return "session"

@app.route('/login', type="text")
def login(r):
  app.setSessionUserAndCookies( r, mrjson.dumps({"user":"Mark"}) )
  return 'Logged in!'

@app.route('/template')
def t2(r):
  context = { "world":"all you python fanatics out there!" } 
  return engine.render('example.ten', context)


app.run(cores=4)

