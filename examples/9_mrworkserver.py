
# What is this?
#
# Json is posted to mrhttp and forwarded to mrworkserver in C.  This is much faster than pure python
#
# Why do this?
#
# My game server receives json. Instead of receiving the json, doing work and then replying 
# the json is passed off to a mrworkserver and we respond immediately.  By gathering messages
# in mrworkserver I'm able to process many related messages together to speed up msg handling
#
# Setup the memcached and mrq servers first:
#
# memcached -l 127.0.0.1 -p 11211 -d -m 50
# pip install mrworkserver
# git clone https://github.com/MarkReedZ/mrworkserver.git; cd mrworkserver
# python tst.py 7100
# python tst.py 7101
#
# Set a session in memcached: set("mrsession11111111111111111111111111111111", '{"username":"xyz"}')
#   Note the session key length must be 32.  mrhttp only supports 32 for performance reasons. 
#   See the session example for how to use sessions
#
# curl -i --raw http://localhost:8080/q/0/0/ -H "Cookie: mrsession=11111111111111111111111111111111;" -X POST -d '{"username":"xyz"}'

import mrhttp
import mrjson as json

app = mrhttp.Application()
app.config["memcache"] = [ ("127.0.0.1", 11211) ]
app.config["mrq"] = [("127.0.0.1",7100),("127.0.0.1",7101)]

@app.route('/', type="text")
async def hello(r):
  return 'Hello World!'

@app.route('/q/{}/{}/',options=['session','mrq','append_user'])
def queue(r, s, t):
  if r.user == None:
    return "Not logged in"
  if r.servers_down:
    return "Servers not available, try again later"
  
  return 'Hello World!'
   
app.run(cores=1)

