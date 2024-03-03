
import traceback
from mrhttp import app
#import asyncmrq, mrpacker

app.config["mrcache"] =  [("127.0.0.1", 7000 )]
app.session_backend = "mrcache"


#@app.on('at_start')
#async def setup():
  #app.c = asyncmrq.Client()
  #await app.c.connect(servers=[("127.0.0.1",7100)])

@app.route('/',options=['session'])
#@app.route('/')
async def index(r):
  print( r.user )
  return "yay"  

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


