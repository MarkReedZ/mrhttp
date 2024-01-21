import requests

data = {}
s = "lo(ng"*50000
data["long"] = s
r = requests.post('http://localhost:8080/form',data) # TODO have this timeout quickly
