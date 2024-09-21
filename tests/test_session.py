
import tests.common
import requests
import mrpacker
from tests.common import eq,contains,stop_server

server = None
def setup():
  print("Begin test_session")
  global server
  server = tests.common.start_server("tests/s_session.py")  
  if not server:
    return 1
  return 0


def test_one():

  # TODO login and check session

  cookie = {'mrsession': '43709dd361cc443e976b05714581a7fb'}
  r = requests.post('http://localhost:8080/s', cookies=cookie)
  eq(r.text, "session")

def teardown():
  global server
  stop_server(server)
  #server.terminate()

