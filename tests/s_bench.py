from mrhttp import Application
from mrhttp import Request
import mrhttp
import socket
import mrasyncmc
import mrjson, mrpacker, msgpack

import tenjin
tenjin.set_template_encoding('utf-8')
from tenjin.helpers import *
engine = tenjin.Engine(path=['tests/templates'])


app = Application()
app.config["memcache"] = [("127.0.0.1", 11211)]
app.config["mrq"] =      [("127.0.0.1", 7100 )]
app.config["mrq"] =      [("127.0.0.1", 7100 ),("127.0.0.1",7001)]
app.session_backend = "memcached"
app.session_backend = "mrworkserver"


@app.on('at_start')
async def dbsetup():
  app.mcc = await mrasyncmc.create_client([("127.0.0.1",11211),("127.0.0.1",11212)], loop=app.loop,pool_size=4)
@app.on('at_end')
async def dbclose():
  await app.mcc.close()

@app.route('/pys', _type="text")
async def pys(r):
  j = await app.mcc.get(b"mrsession" + r.cookies["mrsession"].encode("utf-8"))
  return 'py session'



@app.route('/')
def index(r):
  return 'Hello World!'

@app.route('/print/{}')
def pr(r,foo):
  return foo

@app.route('/sixargs/{}/{}/{}/{}/{}/{}')
def sixargs(r,a,b,c,d,e,f):
  return a

@app.route('/sixnumargs/{num}/{num}/{num}/{num}/{num}/{num}')
def sixnumargs(r,a,b,c,d,e,f):
  return "num "+str(a)

@app.route('/printPostBody')
def printPostBody(r):
  return r.body
@app.route('/printCookies')
def printCookies(r):
  return str(r.cookies)

@app.route('/printHeaders')
def printHeaders(r):
  return str(r.headers)

@app.route('/getip')
def getip(r):
  return str(r.ip)

@app.route('/getip2')
def getip2(r):
  return r.getip

@app.route('/foo')
def foo(r):
  return 'foodff'

@app.route('/123456789012345678901234567890/testing')
def testing(r):
  return 'Hello World!'

@app.route('/404/')
def notFound(r):
  return r.NotFound()

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
  return r.mime_type

@app.route('/form')
def parseForm(r):
  return r.form["param2"]

@app.route('/json')
def parseJ(r):
  if r.json:
    return r.json["name"]
  else:
    return "Send json"

@app.route('/mrpacker')
def parseMrpacker(r):
  return r.mrpack["name"]
  #return "AY"

@app.route('/mrpackerpy')
def parsepacker(r):
  o = mrpacker.unpack(r.body)
  return o["name"]

@app.route('/msgpack')
def parsemsgpack(r):
  o = msgpack.unpackb(r.body,encoding = "utf-8")
  return o["name"]

@app.route('/s',options=['session'])
def session(r):
  if r.user:
    return "user"
  return "session"

@app.route('/mrq/{}',options=['session',"mrq","append_user"])
def mrq(r, tstid):
  return "ok"

@app.route('/mrqget')
async def mrqget(r):
  #o = mrjson.loadb( await app._mrq.get(0,[15,1,0,30]) )
  pk = await app._mrq.get(0,[15,1,0,30])
  #print("len pk", len(pk))
  #j = await app.mcc.get(b"mrqget")
  return "ok"

@app.route('/login', _type="text")
def login(r):
  app.setUserSessionAndCookies( r, 10999, {"user":"Mark"} )
  return 'Logged in!'

@app.route('/template')
def t2(r):
  context = { "world":"all you python fanatics out there!" } 
  return engine.render('example.ten', context)

@app.route('/long',_type='text')
def longresp(r):
  return "fart"*128*1000


app.run(cores=4)

