import mrhttp, asyncio

app = mrhttp.Application()

@app.route('/')
async def hello(r):
  await asyncio.sleep(0.5)
  return 'Hello World!'

app.run(cores=2)

# curl -i --raw 'http://localhost:8080/'

