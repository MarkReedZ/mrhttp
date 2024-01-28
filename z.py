import requests, mrpacker

if 0:
  data = {}
  s = "lo(ng"*50000
  data["long"] = s
  r = requests.post('http://localhost:8080/form',data) # TODO have this timeout quickly

headers = {'Content-type': 'application/mrpacker'}
o = { "typ":"post", "s":2, "t": 'Blonde: "What does IDK stand for?"', "l":"localhost/sub/3", "txt": 'Brunette: "I don’t know."\nBlonde: "OMG, nobody does!"' }
r = requests.post('http://localhost:8080/mrp', data=mrpacker.pack(o), headers=headers)
print( r.raw.headers )
print( "text is ", r.text )
