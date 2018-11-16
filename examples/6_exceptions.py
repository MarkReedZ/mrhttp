import mrhttp

app = mrhttp.Application()


# You can raise any error code

@app.route('/404/')
def not_found(r):
  return app.err404

# Python errors should return 500 and print the exception

@app.route('/500/')
def error_500(r):
  x = {}
  z = x["foo"]
  return 'Hello World!'


app.run(cores=2)

# curl -i --raw 'http://localhost:8080/'
# curl -i --raw 'http://localhost:8080/404/'
# curl -i --raw 'http://localhost:8080/500/'

