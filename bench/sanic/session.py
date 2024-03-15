
import aiomcache
import uvloop

from sanic import Sanic
from sanic.response import text
from sanic_session import Session, MemcacheSessionInterface

app = Sanic("app")

# create a memcache client
client = aiomcache.Client("127.0.0.1", 11211)

# pass the memcache client into the session
session = Session(app, interface=MemcacheSessionInterface(client))

@app.route("/")
async def test(request):
    # interact with the session like a normal dict
    if not request.ctx.session.get('foo'):
        request.ctx.session['foo'] = 0

    request.ctx.session['foo'] += 1

    response = text("YAY")

    return response

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=8000, debug=True)
