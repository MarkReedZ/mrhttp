import mrhttp

app = mrhttp.Application()

@app.route('/')
def hello(r):
  print("Path: ", r.path)
  print("Method: ", r.method)
  print("Mime Type: ", r.mime_type)
  print("IP: ", r.ip)
  print("Body: ", r.body)
  print("Json: ", r.json)
  print("Headers: ", r.headers)
  print("Form data: ", r.form)
  print("Cookies: ", r.cookies)
  print("Query string arguments (?x=y) are found in:")
  print("r.args: ", r.args)
  return 'Hello World!'

@app.route('/{}/')
def argpath(r, arg):
  print("Path arg: ", arg)
  return 'Hello World!'

@app.route('/num/{num}/')
def numarg(r, arg):
  print("arg is a number")
  print(type(arg))
  print(arg)
  return str(arg)

@app.route('/{}/{}/{}/')
def multipleargs(r, a,b,c):
  print(a,b,c)
  return a+b+c


app.run(cores=2)

# curl -i --raw http://localhost:8080/
# curl -i --raw http://localhost:8080/testing

# Query string, cookie header
# curl -i --raw http://localhost:8080/?foo=bar -H "Cookie: mrsession=43709dd361cc443e976b05714581a7fb;"

# Post json
# Note that if you don't use the r.json you must check the application/json header to avoid a security vulnerability
# curl -i --raw http://localhost:8080/ -H "Content-Type: application/json" -X POST -d '[1,2,3,4,5,6,7,8,9]'

# Form:
# curl http://localhost:8080/ -d "param1=value1&param2=value2" -X POST

# Args:
# curl -i --raw http://localhost:8080/num/99/
# curl -i --raw http://localhost:8080/one/two/three/

