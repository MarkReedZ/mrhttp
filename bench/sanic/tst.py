
import sanic
from sanic.response import json

app = sanic.Sanic("my-hello-world-app")

@app.route('/')
async def test(request):
    return sanic.text("Hello World")

if __name__ == '__main__':
    app.run()

