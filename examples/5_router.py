import mrhttp, asyncio

app = mrhttp.Application()

@app.route('/')
def hello(r):
  return 'Hey\n'

@app.route('/foo/bar')
def foobar(r):
  return 'foobar\n'

@app.route('/foo/{}')
def onearg(r, a):
  return 'foo/'+a+'\n'

@app.route('/{}/{}')
def twoargs(r, a, b):
  return a + ", " + b+'\n'

@app.route('/num/{num}')
def onearg(r, a):
  return 'num/'+a+'\n'

app.run(cores=2)

z="""
curl -i --raw 'http://localhost:8080/'
curl -i --raw 'http://localhost:8080/foo/bar/'
curl -i --raw 'http://localhost:8080/foo/notbar/'
curl -i --raw 'http://localhost:8080/test/ing/'
curl -i --raw 'http://localhost:8080/num/2991/'
"""

