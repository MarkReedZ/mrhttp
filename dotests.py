import pkgutil, time
import inspect
import types 
import importlib

import tests

# TODO
#  Check for memcached being up and add the session key so we hit and load the json 43709dd361cc443e976b05714581a7fb
#     memcached -l 127.0.0.1 -p 11211 -d -m 50


if 1:
  package = tests
  for importer, modname, ispkg in pkgutil.iter_modules(package.__path__):
    if modname.startswith("test"):
      m = importlib.import_module('tests.'+modname)
      functions = inspect.getmembers(m, inspect.isfunction)
      for f in functions:
        if f[0] == 'setup':
          if f[1]():
            exit()
      for f in functions:
        if f[0].startswith("test_"):
          try:
            f[1]()
          except Exception as e:
            print(e)
      for f in functions:
        if f[0] == 'teardown':
          f[1]()
  
print("Benchmarks")
    
import argparse
import sys
import asyncio
import os
from asyncio.subprocess import PIPE, STDOUT
import statistics

import uvloop
import psutil
import atexit

#from misc import cpu


def run_wrk(loop, endpoint=None, lua=None, options=None):
  try: 
    endpoint = endpoint or 'http://localhost:8080/'
    if lua:
      wrk_fut = asyncio.create_subprocess_exec( 'wrk', '-t', '4', '-c', '32', '-d', '2', '-s', lua, endpoint, stdout=PIPE, stderr=STDOUT)
    else:
      if options != None:
        wrk_fut = asyncio.create_subprocess_exec( 'wrk', '-t', '4', '-c', '32', '-d', '2', *options, endpoint, stdout=PIPE, stderr=STDOUT)
      else:
        wrk_fut = asyncio.create_subprocess_exec( 'wrk', '-t', '4', '-c', '32', '-d', '2', endpoint, stdout=PIPE, stderr=STDOUT)
  
    wrk = loop.run_until_complete(wrk_fut)
    rps = 0
    lines = []
    while 1:
      line = loop.run_until_complete(wrk.stdout.readline())
      if line:
        line = line.decode('utf-8')
        lines.append(line)
        if line.startswith('Requests/sec:'):
          rps = float(line.split()[-1])
      else:
        break
  
    retcode = loop.run_until_complete(wrk.wait())
    if retcode != 0:
      print('\r\n'.join(lines))
  except:
    print("WTF")


  return rps


noisy = ['atom', 'chrome', 'firefox', 'dropbox', 'opera', 'spotify', 'gnome-documents']

def silence():
  for proc in psutil.process_iter():
    if proc.name() in noisy:
      proc.suspend()

  def resume():
    for proc in psutil.process_iter():
      if proc.name() in noisy:
        proc.resume()
  atexit.register(resume)

silence()

loop = uvloop.new_event_loop()

asyncio.set_event_loop(loop)

server_fut = asyncio.create_subprocess_exec( 'python', 'tests/s_bench.py', stdout=asyncio.subprocess.PIPE )
server = loop.run_until_complete(server_fut)
process = psutil.Process(server.pid)

time.sleep(1)
try:

  opts = ('-H','Cookie: mrsession=43709dd361cc443e976b05714581a7fb; foo=fdsfdasdfasdfdsfasdfsdfsdfasdfas; short=fazc;')
  #print ("Hello pipelined", run_wrk(loop, 'http://localhost:8080/',lua='tests/lua/pipeline.lua'), "Requests/second" )
  print ("Hello          ", run_wrk(loop, 'http://localhost:8080/'),             "Requests/second" )

  if 1:
    #print ("Cookies        ", run_wrk(loop, 'http://localhost:8080/printCookies', options=opts), "Requests/second" )
    #print ("many args      ", run_wrk(loop, 'http://localhost:8080/sixargs/one/two/three/four/five/six'), "Requests/second" )
    #print ("404 natural    ", run_wrk(loop, 'http://localhost:8080/dfads404/'), "Requests/second" )
    print ("404            ", run_wrk(loop, 'http://localhost:8080/404/'), "Requests/second" )
    print ("Form parsing   ", run_wrk(loop, 'http://localhost:8080/form',lua='tests/lua/form.lua'), "Requests/second" )
    print ("Templates      ", run_wrk(loop, 'http://localhost:8080/template'),            "Requests/second" )
    print ("mrpacker       ", run_wrk(loop,'http://localhost:8080/mrpacker',lua='tests/lua/mrpacker.lua'), "Requests/second" )
    #print ("Sessions       ", run_wrk(loop, 'http://localhost:8080/s',     options=opts), "Requests/second" )
    # Disabled in s_bench.py print ("Sessions (py)  ", run_wrk(loop, 'http://localhost:8080/pys',   options=opts), "Requests/second" )
    #print ("Session login  ", run_wrk(loop, 'http://localhost:8080/login'),               "Requests/second" )
    #print ("json post      ", run_wrk(loop,'http://localhost:8080/json',lua='tests/lua/json.lua'), "Requests/second" )
    #print ("mrpacker py    ", run_wrk(loop,'http://localhost:8080/mrpackerpy',lua='tests/lua/mrpacker.lua'), "Requests/second" )
    #print ("msgpack py     ", run_wrk(loop,'http://localhost:8080/msgpack',lua='tests/lua/msgpack.lua'), "Requests/second" )

  
    opts = ('-H','XX-Real-IP: 1.2.3.4')
    #print ("get ip         ", run_wrk(loop,'http://localhost:8080/getip',options=opts), "Requests/second" )
    #print ("many num args  ", run_wrk(loop, 'http://localhost:8080/sixargs/155/2001/29999/25/29999543/93243242394'), "Requests/second" )
    #print ("404            ", run_wrk(loop, 'http://localhost:8080/404/'), "Requests/second" )

  # Grab the stdout for debug 
  if 0:
    lines = []
    x = 0
    while 1:
      x += 1
      print(x)
      #if x > 19842: break
      if x > 21605: break
      line = loop.run_until_complete(server.stdout.readline())
      if line:
        line = line.decode('utf-8')
        lines.append(line)
      else:
        break
    print ( len(lines) )
    o = open( "wrkout", "wb" )
    o.write( (''.join(lines)).encode("utf-8") )
    o.close()

except KeyboardInterrupt:
  pass
finally:
  server.terminate()
  loop.run_until_complete(server.wait())

