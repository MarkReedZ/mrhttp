from mrhttp import Application
from mrhttp import Request
import mrhttp
import socket

app = Application()
app.config["memcache"] = [ ("127.0.0.1", 11211) ]

@app.route('/',type='text')
def index(r):
  return 'Hello World!'

@app.route('/print/{}')
def pr(r,foo):
  return foo
@app.route('/args/{}/{}/{}/{}/{}/{}/')
def multipleargs(r,a,b,c,d,e,f):
  return a+b+c+d+e+f
@app.route('/args/{num}/')
def numarg(r,n):
  return str(n)


@app.route('/printPostBody')
def printPostBody(r):
  return r.body
@app.route('/printCookies')
def printCookies(r):
  return str(r.cookies)

@app.route('/foo')
def foo(r):
  return 'foodff'

@app.route('/123456789012345678901234567890/testing')
def testing(r):
  return 'Hello World!'

@app.route('/404/')
def notFound(r):
  raise mrhttp.HTTPError(404)
  return 'Hello World!'

@app.route('/500/')
def error500(r):
  x = {}
  z = x["foo"]
  return 'Hello World!'

@app.route('/to64')
def to64test(r):
 
  if 'uHfE' != mrhttp.to64(1234567): return 'failed1'
  if mrhttp.from64('uHfE') != 1234567: return 'failed2'
  if '3mM' != mrhttp.to64(12345): return 'failed3'
  return 'ok'

@app.route('/foo/{}')
def foo1(r, foo):
  return 'foo1'

@app.route('/foo/{}/{}')
def foo2(r, foo, bar):
  return 'foo2'

@app.route('/foo/bar/{}')
def foo3(r, foo):
  return 'foo3'

@app.route('/{}/bar/{}')
def barmid(r, foo, bar):
  return 'barmid'

@app.route('/content')
def content(r):
  #print( r.mime_type )
  #print( r.encoding )
  return r.mime_type

@app.route('/query_string')
def query_string(r):
  return str(r.args)

@app.route('/json')
def parseJ(r):
  return r.json["name"]

@app.route('/form')
def parseForm(r):
  if r.form == None: return "No form"
  return r.form["param2"]

@app.route('/s',options=['session'])
def session(r):
  if r.user:
    return r.user["user"]
  return "session"

@app.route('/noreturn')
def noreturn(r):
  pass


app.run(cores=1)

