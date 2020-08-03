import mrpacker
from pymemcache.client.base import Client
c = Client(('localhost', 11211))
c.set( "mrsession43709dd361cc443e976b05714581a7fb",mrpacker.pack({"user":"Mark"}) )
c = Client(('localhost', 11212))
c.set( "mrsession43709dd361cc443e976b05714581a7fb",mrpacker.pack({"user":"Mark"}) )
#print( c.get("mrsessionZZZZ9dd361cc443e976b05714581a7fb"))

