from mrhttp import Application
from mrhttp import Request
import mrhttp
import socket

app = Application()

@app.route('/foo')
def foo():
  return 'foo\n'

#@app.route('/',tools=['session'])
@app.route('/')
def hello():
  raise mrhttp.HTTPRedirect("/foo")
  return 'Hello World!'

app.run(debug=True, cores=1)
