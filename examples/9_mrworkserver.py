
# What is this?
#
# Json is posted to mrhttp and forwarded to a mrworkserver cluster using C code.  This is much faster than doing it using python
# in the page handler.
#
# Why do this?
#
# My game server receives json. Instead of receiving the json, doing work and then replying 
# the json is passed off to a mrworkserver and we respond immediately so the client doesn't
# wait.  By gathering messages in mrworkserver I'm able to process many related messages 
# together to speed up msg handling
#
# Setup the memcached and mrq servers first:
#
# memcached -l 127.0.0.1 -p 11211 -d -m 50
# pip install mrworkserver
# git clone https://github.com/MarkReedZ/mrworkserver.git; cd mrworkserver
# python tst.py 7100
# python tst.py 7101
#
# curl -i --raw http://localhost:8080/q/0/0/ -X POST -d '{"username":"xyz"}'
#
# To fetch a user's session and pass it to mrworkserver:
#
# Set a session in memcached by logging in:
#   curl -i --raw http://localhost:8080/
#   example output:   AeeZjIBxhxDEaTKc69MTge3tq_kqf7Bh
#
# Use the session key that was output:
#   curl -i --raw http://localhost:8080/sq/ -H "Cookie: mrsession=AeeZjIBxhxDEaTKc69MTge3tq_kqf7Bh;" -X POST -d '{"test":"xyz"}'

import mrhttp
import mrjson as json

app = mrhttp.Application()
app.config["memcache"] = [ ("127.0.0.1", 11211) ]
app.config["mrq"] = [("127.0.0.1",7100),("127.0.0.1",7101)]

@app.route('/q/{}/{}/',options=['mrq'])
def queue(r, s, t):
  if r.servers_down:
    return "Servers not available, try again later"
  return 'Hello World!'

@app.route('/')
async def login(r):

  user_id = 10999
  user = {"name":"Mark"}
  return str( app.setUserSessionAndCookies( r, user_id, user, json=True ) ) + "\n"


@app.route('/sq/',options=['session','mrq','append_user'])
def session_queue(r):
  if r.user == None:
    return "Not logged in"
  if r.servers_down:
    return "Servers not available, try again later"
  return 'Hello World!'
   
app.run(cores=1)

