
from mrhttp import app

@app.route('/')
async def index(r):
  return "indeX"

@app.route('/j')
def parseJ(r):
  return r.json["name"]

@app.route('/m')
def parseMrpacker(r):
  return r.mrpack["name"]


try:
  app.run(cores=4)
except Exception as e:
  print("YAY",e)


