import mrhttp
import http.cookies

app = mrhttp.Application()

@app.route('/')
def hello(r):

  # Add or replace headers
  r.response.headers["foo"] = "bar"
  r.response.headers["Content-Type"] = "plain/text"

  c = http.cookies.SimpleCookie()
  c['hello'] = 'world'
  c['hello']['domain'] = 'localhost'
  c['hello']['path'] = '/'
  c['hello']['max-age'] = 3600
  c['foo'] = 'bar'

  r.response.cookies = c
  
  return 'Hello World!'


# Type text sets Content-Type to plain/text

@app.route('/text', type="text")
def plaintext(r):
  return 'Hello World!'

# Type text sets Content-Type to application/json

@app.route('/json', type="json")
def json(r):
  return '[1,2,3]'


app.run(cores=2)

# curl -i --raw 'http://localhost:8080/' -H "Cookie: mrsession=43709dd361cc443e976b05714581a7fb;"

# curl -i --raw 'http://localhost:8080/text'
# curl -i --raw 'http://localhost:8080/json'
