
from mrhttp import app

@app.route('/')
def hello(r):
  if r.file == None:
    return "No file uploaded"
  #for f in r.files:
    #print(f)
  name = r.file['name']
  typ  = r.file['type']
  body = r.file['body']
  return name

app.run(cores=4)

# curl -i -X POST -F "data=@14_upload.py" http://localhost:8080/


