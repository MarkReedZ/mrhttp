
#
# Run this and open your browser to http://localhost:8080/ 
#
# Memcached must be running:
#   memcached -l 127.0.0.1 -p 11211 -d -m 50
#

import http.cookies
import mrhttp

app = mrhttp.Application()
app.config["memcache"] = [ ("127.0.0.1", 11211) ]
#app.config["mrq"] = [ ("127.0.0.1", 7100) ]
#app.config["mrcache"] = [ ("127.0.0.1", 7000) ]
app.session_backend = "memcached"
#app.session_backend = "mrworkserver"
#app.session_backend = "mrcache"

@app.route('/',options=['session'])
def session(r):

  if r.user == None:
    return "You are not logged in.  <a href='login'>Login here</a>"

  if ( r.user["name"] == 'Mark' ): 
    return "You are logged in as Mark <a href='logout'>Logout here</a>"

  return "session"


@app.route('/login')
async def login(r):

  # Fetch the user
  user_id = 10999
  user = {"name":"Mark"}

  # This creates a session key, stores it in the backend with the user data, and sets the session
  # cookie in the browser with the key 'mrsession'

  app.setUserSessionAndCookies( r, user_id, user )

  return "You are now logged in! <a href='/'> Try the main page </a>"

@app.route('/logout')
async def logout(r):

  # Clears the mrsession cookie
  app.logoutUser(r)

  return "Logged out! <a href='/'> Return to the main page </a>"

app.run(cores=1)

