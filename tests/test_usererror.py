
import tests.common
import requests
from tests.common import eq,contains
#process.terminate()

server = None

def setup():
  print("Begin test_usererror")
  #global server
  #server = tests.common.start_server("tests/server.py")  

def test_badrouteargs():
  global server
  server = tests.common.start_server("tests/s_badroute.py", True)  
  eq(server, None)
  #r = requests.get('http://localhost:8080/123456789012345678901234567890/testing')
  #print (r.status_code, r.headers, r.encoding, r.text)

#def teardown():
  #print("Bad routes tests complete")
  #global server
  #server.terminate()

