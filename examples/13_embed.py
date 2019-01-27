
# You can embed an http server into an existing asyncio app
# See setup_statserver in https://github.com/MarkReedZ/mrworkserver
#
# curl -i --raw http://localhost:8080/

import asyncio

def setup(loop):
  from mrhttp import app

  @app.route('/')
  async def index(r):
    return "indeX"

  return asyncio.ensure_future( app.start_server(host='0.0.0.0', port=8080, loop=loop), loop=loop )

try:
  #def someapp():
    #loop.call_later(1, someapp)

  loop = asyncio.get_event_loop()
  task = setup(loop)
  #loop.call_later(1, someapp)
  loop.run_forever()

except Exception as e:
  print(e)

