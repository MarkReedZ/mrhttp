
import common
import requests
from common import eq,contains,stop_server
import mrpacker

if 1:
  r = requests.post('http://localhost:8765', data={"p1":"v1","param2":"value2"})
  #r = requests.post('http://localhost:8765', files={"p1":"v1","param2":"value2"})
  #r = requests.post('http://localhost:8765', data={"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa":"","英":"+=&ue2"})

  #eq(r.text, '{"p1":"v1","param2":"value2"}')

