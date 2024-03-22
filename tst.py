
import traceback, mrjson
from mrhttp import app
import mrpacker


app.config["memcache"] = [ ("127.0.0.1", 11211) ]
#app.static_cached("www","/path/to/www")

#@app.on('at_start')
#async def setup():
  #app.c = asyncmrq.Client()
  #await app.c.connect(servers=[("127.0.0.1",7100)])

#@app.route('/')
@app.route('/',options=['cache'])
def index(r):
  return "hello world"  

@app.route('/123456789123456789')
async def long(r):
  return "long"

@app.route('/json')
def json(r):
  return mrjson.dumps({'message': 'Hello, world!'})

@app.route('/mrp',_type="mrp")
def mrp(r):
  return mrpacker.pack("hello")

@app.route('/{}/tst')
def firstarg(r,a):
  return a


try:
  app.run(cores=4)
except Exception as e:
  print("YAY",e)


