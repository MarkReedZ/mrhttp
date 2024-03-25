
import traceback, mrjson
from mrhttp import app
import mrpacker


app.config["memcache"] = [ ("127.0.0.1", 11211) ]
#app.static_cached("www","/path/to/www")

#@app.on('at_start')
#async def setup():
  #app.c = asyncmrq.Client()
  #await app.c.connect(servers=[("127.0.0.1",7100)])

@app.route('/',_type='text',options=['cache'])
#@app.route('/',_type='text')
def index(r):
  return "hello world"  

@app.route('/json')
def json(r):
  return mrjson.dumps({'message': 'Hello, world!'})



try:
  app.run(cores=1)
except Exception as e:
  print("YAY",e)


