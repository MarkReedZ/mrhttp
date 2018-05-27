from mrhttp import Application
from mrhttp import Request
import mrhttp
import socket

app = Application()

@app.route('/foo/{}/{}')
def foo2(r, foo, bar, baz):
  return 'Hello World!'

app.run(cores=1)

