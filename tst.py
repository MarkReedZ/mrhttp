
import traceback, mrjson
from mrhttp import app

@app.route('/',_type='text')
def index(r):
  return "hello world"  

@app.route('/json')
def json(r):
  return mrjson.dumps({'message': 'Hello, world!'})

@app.route('/upload')
def upload(r):
  try:
    print(r.headers)
    print(r.file)
  except:
    print("ERROR")
  return "Success"


try:
  app.run(cores=1)
except Exception as e:
  print("Done",e)


