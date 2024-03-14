
import common
import requests
from common import eq,contains,stop_server
import mrpacker

if 1:

  headers = {'Content-Type': 'application/mrpacker'}
  o = { "typ":"post", "s":2, "t": 'Blonde: "What does IDK stand for?"', "l":"localhost/sub/3", "txt": 'Brunette: "I don’t know."\nBlonde: "OMG, nobody does!"' }
  r = requests.post('http://localhost:8080/mrp', data=mrpacker.pack(o), headers=headers)
  if eq(r.text, 'post') != 0:
    print( r.raw.headers )
    print( "text is ", r.text )

  cookie = {'foo': 'bar','baz':'3'}
  r = requests.post('http://localhost:8080/printCookies', cookies=cookie)
  eq(r.text, "{'baz': '3', 'foo': 'bar'}")

  cookie = {'foo': 'b=ar'}
  r = requests.post('http://localhost:8080/printCookies', cookies=cookie)
  eq(r.text, "{'foo': 'b=ar'}")
  

if 0:
  data = {}
  s = "lo(ng"*10000
  data["long"] = s
  r = requests.post('http://localhost:8080/form',data)
  eq(r.status_code, 200)

if 0:
  headers = {'Content-type': 'application/mrpacker'}
  o = { "typ":"post", "s":4, "t": 'Blonde: "What does IDK stand for?"', "l":"", "txt": 'Brunette: "I don’t know."\nBlonde: "OMG, nobody does!"' }
  o = { "typ": "post", "l": "", "t": "Blonde: \"What does IDK stand for?\"", "txt": "Brunette: \"I don’t know.\"\n\nBlonde: \"OMG, nobody does!\"", "s": 3}
  print(len(o["txt"]))
  b = mrpacker.pack(o)
  s = ""
  for c in b:
    #s = s + str(hex(int(c))) + ", "
    s = s + str(int(c)) + ", "
  print(s)
  print( mrpacker.unpack(b) )
  #r = requests.post('http://localhost:8080/mrp', data=mrpacker.pack(o), headers=headers)
  #eq(r.text, 'post')

l = [66, 114, 117, 110, 101, 116, 116, 101, 58, 32, 34, 73, 32, 100, 111, 110, 226, 128, 153, 116, 32, 107, 110, 111, 119, 46, 34, 10, 10, 66, 108, 111, 110, 100, 101, 58, 32, 34, 79, 77, 71, 44, 32, 110, 111, 98, 111, 100, 121, 32, 100, 111, 101, 115, 33, 34]
print(len(l))
s = ""
#for c in l:
  #s += chr(c)
#print(s)

z = """
"""
54
37, 131, 116, 121, 112, 132, 112, 111, 115, 116, 129, 108, 128, 129, 116, 102, 34, 0, 0, 0, 66, 108, 111, 110, 100, 101, 58, 32, 34, 87, 104, 97, 116, 32, 100, 111, 101, 115, 32, 73, 68, 75, 32, 115, 116, 97, 110, 100, 32, 102, 111, 114, 63, 34, 131, 116, 120, 116, 102, 56, 0, 0, 0, 66, 114, 117, 110, 101, 116, 116, 101, 58, 32, 34, 73, 32, 100, 111, 110, 226, 128, 153, 116, 32, 107, 110, 111, 119, 46, 34, 10, 10, 66, 108, 111, 110, 100, 101, 58, 32, 34, 79, 77, 71, 44, 32, 110, 111, 98, 111, 100, 121, 32, 100, 111, 101, 115, 33, 34, 129, 115, 195, 
{'typ': 'post', 'l': '', 't': 'Blonde: "What does IDK stand for?"', 'txt': 'Brunette: "I don’t know."\n\nBlonde: "OMG, nobody does!"', 's': 3}
56
