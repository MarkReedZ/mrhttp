
import common
import requests
from common import eq,contains,stop_server

data = {}
s = "lo(ng"*10000
data["long"] = s
r = requests.post('http://localhost:8080/form',data)
eq(r.status_code, 200)

