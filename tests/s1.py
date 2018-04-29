from mrhttp import Application
from mrhttp import Request
import mrhttp
import socket

app = Application()

@app.route('/')
def index():
  return 'Hello World!'

@app.route('/print/{}')
def pr(foo):
  return foo

@app.route('/printPostBody')
def printPostBody():
  return app.request.body

@app.route('/foo')
def foo():
  return 'foodff'

@app.route('/123456789012345678901234567890/testing')
def testing():
  return 'Hello World!'

@app.route('/404/')
def notFound():
  raise mrhttp.HTTPError(404)
  return 'Hello World!'

@app.route('/500/')
def error500():
  x = {}
  z = x["foo"]
  return 'Hello World!'

@app.route('/foo/{}')
def foo1(foo):
  return 'foo1'

@app.route('/foo/{}/{}')
def foo2(foo, bar):
  return 'foo2'

@app.route('/foo/bar/{}')
def foo3(foo):
  return 'foo3'

@app.route('/{}/bar/{}')
def barmid(foo, bar):
  return 'barmid'

app.run(cores=1)

