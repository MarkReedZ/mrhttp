
import asyncio

# TODO The idea here is to test some bad requests.  Not used yet

from mrhttp import app

lines = []

req = b'GET / HTTP/1.1\r\nHost: localhost:41111\r\nUser-Agent: curl/7.58.0\r\n\r\n'


@app.on('after_start')
async def after():
  r, w = await asyncio.open_connection('127.0.0.1', 41111)
  #w.write(b'badreq')
  w.write(req)
  while True:
    line = await r.readline()
    print(line)
    print(line.strip())
    if line.strip() == b'':
      break
    lines.append(line)
  w.close()
  app.stop()


@app.route('/')
async def index(r):
  return "test"

try:
  app.run(cores=1, port=41111)
except Exception as e:
  print("YAY",e)


#def test_bad_request_response():
    #app = Sanic('test_bad_request_response')
    #lines = []
    #@app.listener('after_server_start')
    #async def _request(sanic, loop):
        #connect = asyncio.open_connection('127.0.0.1', 42101)
        #reader, writer = await connect
        #writer.write(b'not http')
        #while True:
            #line = await reader.readline()
            #if not line:
                #break
            #lines.append(line)
        #app.stop()
    #app.run(host='127.0.0.1', port=42101, debug=False)
    #assert lines[0] == b'HTTP/1.1 400 Bad Request\r\n'
    #assert lines[-1] == b'Error: Bad Request'
#
