import mrhttp, asyncio

app = mrhttp.Application()

@app.route('/')
def hello(r):
  return 'Hey'

@app.route('/foo/bar')
def foobar(r):
  return 'foobar'

@app.route('/foo/{}')
def onearg(r, a):
  return 'foo/'+a

@app.route('/{}/{}')
def twoargs(r, a, b):
  return a + ", " + b

app.run(cores=2)

# curl -i --raw 'http://localhost:8080/'
# curl -i --raw 'http://localhost:8080/foo/bar/'
# curl -i --raw 'http://localhost:8080/foo/notbar/'
# curl -i --raw 'http://localhost:8080/test/ing/'

