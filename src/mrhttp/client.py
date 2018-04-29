
import asyncio
import socket
import os
from mrhttp import MemcachedProtocol

class MrProtocol(asyncio.Protocol):
    def connection_made(self, transport):
        message = "hello"
        transport.write(message.encode())
        print('Data sent: {!r}'.format(message))

    def data_received(self, data):
        print('Data received: {!r}'.format(data.decode()))

    def connection_lost(self, exc):
        print('The server closed the connection')
        print('Stop the event loop')
        loop.stop()

# TODO
#  C code needs access to connections
#  If no conns then ?
#  Multiple servers
#  Failed connection handling
class Client(object):
  def __init__(self, host, port, loop):
    self._connections = []

    #self._protocol_factory = MemcachedProtocol;
    #coro = loop.create_connection(lambda: self._protocol_factory(self), '127.0.0.1', 7000)
    #coro = loop.create_connection(MrProtocol, '127.0.0.1', 7000)
    try:
      coro = loop.create_connection(lambda: MemcachedProtocol(self), '127.0.0.1', 7000)
      self.conn = None;
      loop.run_until_complete(coro)
    except ConnectionRefusedError:
      pass
      

  def setConnection(self, conn):
    self.conn = conn

