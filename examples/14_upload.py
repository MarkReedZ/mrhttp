
from mrhttp import app

@app.route('/')
def hello(r):
  print(r.headers)
  if r.file == None:
    return "No file uploaded"
  for f in r.files:
    print(f['name'])
  name = r.file['name']
  typ  = r.file['type']
  body = r.file['body']
  return name

app.run(cores=1)

# curl -i -X POST -F "data=@14_upload.py" http://localhost:8080/


