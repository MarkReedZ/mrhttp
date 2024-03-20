
import sanic

app = sanic.Sanic("my-hello-world-app")

@app.route('/')
async def test(request):
  return sanic.text("Hello World")

@app.route("/s")
async def sess(request):
  return sanic.text("session")

if __name__ == '__main__':
    app.run(port=8080)
