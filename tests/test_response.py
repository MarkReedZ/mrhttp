
import tests.common
import requests
from tests.common import eq,contains,stop_server

#process.terminate()

server = None
def setup():
  print("Begin test_response")
  global server
  server = tests.common.start_server("tests/s_resp.py")  
  if not server:
    return 1
  return 0


def test_one():
  r = requests.get('http://localhost:8080/type')
  eq(r.status_code, 200)
  eq(r.headers["Content-Type"],"text/plain")

  #r = requests.get('http://localhost:8080/long')
  #eq(r.status_code, 200)
  #eq(r.headers["Content-Type"],"text/plain")

  r = requests.get('http://localhost:8080/')
  eq(r.status_code, 200)
  eq(len(r.headers.keys()),6)
  eq(r.headers["foo"],"b")
  eq(r.headers["test"],"0123456789012345678901234567890123456789")

  r = requests.get('http://localhost:8080/cook1')
  eq(r.status_code, 200)
  eq(len(r.headers.keys()),6)
  eq(r.headers["foo"],"bar")
  eq(r.headers["Set-Cookie"],"foo=bar, hello=world; Domain=localhost; Max-Age=3600; Path=/")

  r = requests.get('http://localhost:8080/cook2')
  eq(r.status_code, 200)
  eq(len(r.headers.keys()),5)
  eq(r.headers["Set-Cookie"],"foo=bar")


def teardown():
  print("teardown")
  global server
  stop_server(server)
  #server.terminate()

