
import tests.common
import requests
from tests.common import eq,contains,stop_server

#process.terminate()

server = None
def setup():
  print("Begin test_exceptions")
  global server
  server = tests.common.start_server("tests/s1.py")  
  if not server:
    return 1
  return 0


def test_one():
  r = requests.get('http://localhost:8080/404/')
  eq(r.status_code, 404)
  contains(r.text, "The requested page was not found")
  r = requests.get('http://localhost:8080/foo')
  eq(r.status_code, 200)
  r = requests.get('http://localhost:8080/500/')
  eq(r.status_code, 500)
  contains(r.text, "Internal Server Error")

  r = requests.get('http://localhost:8080/noreturn')
  eq(r.status_code, 500)
  contains(r.text, "Internal Server Error")



  #print( r.status_code, r.headers, r.text )

def teardown():
  global server
  stop_server(server)
  #server.terminate()

