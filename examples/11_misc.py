
from mrhttp import app
import time

print( app.timesince(int(time.time()-60*60*4)) )
print( app.to64(1234) )
print( app.from64(app.to64(12345)) )

# python3 -mtimeit -s'from mrhttp import app' 'app.to64(200000)'
# python3 -mtimeit -s'from mrhttp import app' 'app.from64("b3432hfds")'
# python3 -mtimeit -s'from mrhttp import app;import time;ts = int(time.time()-(60*60*3))' 'app.timesince(ts)'

