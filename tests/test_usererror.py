
import tests.common
import requests
from tests.common import eq,contains,stop_server
#process.terminate()


server = None
#def setup():
  #print("Begin test_requests")
  #global server
  #server = tests.common.start_server("tests/s_badroute.py")
  #if not server:
    #return 1
  #return 0

def setup():
  print("Begin test_usererror")
  #global server
  #server = tests.common.start_server("tests/server.py")  

def test_badrouteargs():
  global server
  server = tests.common.start_server("tests/s_badroute.py", True)  
  eq(server, None)
  stop_server(server)
  #r = requests.get('http://localhost:8080/123456789012345678901234567890/testing')
  #print (r.status_code, r.headers, r.encoding, r.text)

def teardown():
  pass
  #global server
  #server.terminate()
