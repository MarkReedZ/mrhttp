
import tests.common
import requests
from tests.common import eq,contains

#process.terminate()

server = None
def setup():
  print("Begin test_requests")
  global server
  server = tests.common.start_server("tests/s1.py")  
  if not server:
    return 1
  return 0


def test_one():
  r = requests.get('http://localhost:8080/foo')
  eq(r.status_code, 200)
  r = requests.get('http://localhost:8080/foo/baz/whatever')
  eq(r.text, "foo2")
  r = requests.get('http://localhost:8080/foo/bar/whatever')
  eq(r.text, "foo3")
  r = requests.get('http://localhost:8080/foo/baz/w')
  eq(r.text, "foo2")
  r = requests.get('http://localhost:8080/foo/bar/w')
  eq(r.text, "foo3")
  r = requests.get('http://localhost:8080/food/bar/w')
  eq(r.text, "barmid")
  r = requests.get('http://localhost:8080/food/bar/whhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhh/')
  eq(r.text, "barmid")
  r = requests.get('http://localhost:8080/print/caf%C3%A9/')
  eq(r.text, "café")
  r = requests.get('http://localhost:8080/print/%E4%B8%8D%E5%8F%AF%E5%86%8D%E7%94%9F%E8%B5%84%E6%BA%90/?test')
  eq(r.text, "不可再生资源")


  #contains(r.text, "Internal Server Error")
  #print( r.status_code, r.headers, r.text )

def teardown():
  global server
  server.terminate()

