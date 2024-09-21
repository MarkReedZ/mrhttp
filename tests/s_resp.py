from mrhttp import Application
from mrhttp import Request
import mrhttp
import socket
import http.cookies

app = Application()

@app.route('/type', _type='text')
def content_type(r):
  return 'Hello World!'

@app.route('/')
def index(r):
  r.response.headers["test"] = "0123456789012345678901234567890123456789"
  r.response.headers["Foo"] = "b"
  return 'Hello World!'

@app.route('/cook1', _type="text")
def cook1(r):

  cookies = http.cookies.SimpleCookie()
  cookies['hello'] = 'world'
  cookies['hello']['domain'] = 'localhost'
  cookies['hello']['path'] = '/'
  cookies['hello']['max-age'] = 3600
  cookies['foo'] = 'bar'

  r.response.cookies = cookies
  r.response.headers["foo"] = 'bar'

  return 'cookie'

@app.route('/cook2', _type="text")
def cook2(r):

  cookies = http.cookies.SimpleCookie()
  cookies['foo'] = 'bar'
  r.response.cookies = cookies

  return 'cookie'

# python requests can't handle this?? Hangs waiting for response.  curl is fine
#@app.route('/long', _type='text')
#def longresp(r):
  #return "longresp"*64*1000

app.run(cores=1)

