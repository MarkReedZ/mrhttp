from mrhttp import app

app.config["memcache"] = [ ("127.0.0.1", 11211) ]
app.config["mrcache"] =  [("127.0.0.1", 7000 )]
app.session_backend = "mrcache"

@app.route('/',_type='text')
def index(r):
  return 'Hello World!'

@app.route('/s',options=['session'])
def session(r):
  if r.user:
    return "session"
  return "no session"

app.run(cores=1)

