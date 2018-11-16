
from mrhttp import app

app.err404 = "My 404 page"

@app.route('/')
def hello(request):
  return 'Hello World!'

app.run(cores=2)

# curl -i --raw http://localhost:8080/notfound

