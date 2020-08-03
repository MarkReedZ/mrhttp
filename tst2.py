
import asyncio

def setup():
  from mrhttp import app

  @app.route('/')
  async def index(r):
    return "indeX"

  return asyncio.ensure_future( app.start_server(), loop=app.loop )
  #return app.start_server()

try:

  def wakeup():
    loop.call_later(0.1, wakeup)
  task = setup()
  print(task)
  #app.run(cores=4)
  loop = asyncio.get_event_loop()
  loop.call_later(0.1, wakeup)
  loop.run_forever()

except KeyboardInterrupt:
  print ("keybd")
except Exception as e:
  print("YAY",e)

