
import traceback
from mrhttp import app
import asyncmrq, mrpacker

app.config["memcache"] = [ ("127.0.0.1", 11211) ]

#@app.on('at_start')
#async def setup():
  #app.c = asyncmrq.Client()
  #await app.c.connect(servers=[("127.0.0.1",7100)])

msg = mrpacker.pack( [1,2,3] )
msglen = len(msg)
print(msg)

@app.route('/',options=['session'])
async def index(r):
  #await app.c.push( r.user["id"], 0, msg, msglen )
  #print( r.headers )
  #print( r.ip )
  return "yay"  
  #x = r.form
  #d = r.mrpack
  #return d["name"]
  #x = r.form
  #return x["param2"]

@app.route('/json')
def json(r):
  return r.json["name"]

@app.route('/mrpacker')
def mrpacker(r):
  return r.mrpack["name"]


try:
  app.run(cores=4)
except Exception as e:
  print("YAY",e)


