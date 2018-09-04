from mrhttp import app

import multiplefiles

@app.route('/')
def hello(request):
  return 'Hello World!'

app.run(cores=2)

# curl -i --raw 'http://localhost:8080/'

