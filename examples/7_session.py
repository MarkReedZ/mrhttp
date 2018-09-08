

# Run this and open your browser to http://localhost:8080/ 

import http.cookies
import mrhttp
try:
  import mrjson as json
except:
  print("This example requires\n\npip install mrjson\n\n")

app = mrhttp.Application()
app.config["memcache"] = [ ("127.0.0.1", 11211) ]


@app.route('/',options=['session'])
def session(r):

  if r.user == None:
    return "You are not logged in.  <a href='login'>Login here</a>"

  if ( r.user["user"] == 'Mark' ): 
    return "You are logged in as Mark <a href='logout'>Logout here</a>"

  return "session"


@app.route('/login')
async def login(r):

  # This creates a session key, stores it in memcache with the user's data, and sets the session
  # cookie in the browser
  try:
    app.setUserSessionAndCookies( r, json.dumps({"user":"Mark"}) )
  except Exception as e:
    print(e)

  return "You are now logged in! <a href='/'> Try the main page </a>"

@app.route('/logout')
async def logout(r):
  c = http.cookies.SimpleCookie()
  c['mrsession'] = ""
  c['mrsession']['max-age'] = 0
  r.response.cookies = c
  return "Logged out! <a href='/'> Return to the main page </a>"

app.run(cores=1)

# Open your browser to http://localhost:8080/ 

