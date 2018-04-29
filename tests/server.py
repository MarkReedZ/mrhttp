from mrhttp import Application
from mrhttp import Request
import mrhttp
import socket

app = Application()


@app.route('/')
def index():
  return 'Hello World!'

@app.route('/foo')
def foo():
  return 'foo'

@app.route('/123456789012345678901234567890/testing')
def testing():
  return 'Hello World!'

@app.route('/404/')
def notFound():
  raise mrhttp.HTTPError(404)
  return 'Hello World!'

@app.route('/foo/{}')
def foo1(foo):
  return 'Hello World!'

@app.route('/foo/{}/{}')
def foo2(foo, bar):
  return 'Hello World!'

app.run(cores=1)

