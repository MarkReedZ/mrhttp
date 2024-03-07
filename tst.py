
import traceback
from mrhttp import app
import mrpacker


app.config["memcache"] = [ ("127.0.0.1", 11211) ]

#@app.on('at_start')
#async def setup():
  #app.c = asyncmrq.Client()
  #await app.c.connect(servers=[("127.0.0.1",7100)])

#@app.route('/',options=['session'])
@app.route('/',_type="html")
def index(r):
  print( r.headers )
  #print( r.ip )
  return "yay"  
  #d = r.mrpack
  #return d["name"]
  #x = r.form
  #return x["param2"]

@app.route('/json')
def json(r):
  return r.json["name"]

@app.route('/mrp',_type="mrp")
def mrp(r):
  return mrpacker.pack("hello")


try:
  app.run(cores=1)
except Exception as e:
  print("YAY",e)


