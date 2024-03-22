
readme = """
  pip install psutil requests msgpack mrasyncmc tenjin mrpacker mrworkserver
  mrcache and mrworkserver must be running for the benchmarks
  mrcache:
    git clone https://github.com/MarkReedZ/mrcache.git
    cd mrcache; ./bld; ./mrcache
  mrworkserver
    python workserver.py    
    
"""


import pkgutil, time
import inspect
import types 
import importlib

import tests

import argparse
import sys
import asyncio
import os
from asyncio.subprocess import PIPE, STDOUT
import statistics

import uvloop
import psutil
import atexit

# TODO
#  Check for mrworkserver and mrcache being up and add the session key so we hit and load the json 43709dd361cc443e976b05714581a7fb
#     mrcache -m 64 -i 16
#     python mrworkserver/tst.py


async def run_tests():
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
  
    

#from misc import cpu


async def run_wrk(loop, endpoint=None, lua=None, options=None):
  rps = 0
  try: 
    endpoint = endpoint or 'http://localhost:8080/'
    if lua:
      if options != None:
        proc = await asyncio.create_subprocess_exec( 'wrk', '-t', '4', '-c', '32', '-d', '2', '-s', lua, *options, endpoint, stdout=PIPE, stderr=STDOUT)
      else:
        proc = await asyncio.create_subprocess_exec( 'wrk', '-t', '4', '-c', '32', '-d', '2', '-s', lua, endpoint, stdout=PIPE, stderr=STDOUT)
    else:
      if options != None:
        proc = await asyncio.create_subprocess_exec( 'wrk', '-t', '4', '-c', '32', '-d', '2', *options, endpoint, stdout=PIPE, stderr=STDOUT)
      else:
        proc = await asyncio.create_subprocess_exec( 'wrk', '-t', '4', '-c', '32', '-d', '2', endpoint, stdout=PIPE, stderr=STDOUT)
  
    stdout, stderr = await proc.communicate()
    rps = 0
    lines = stdout.decode('utf-8').split("\n")
    for line in lines:
      if line.startswith('Requests/sec:'):
        rps = float(line.split()[-1])
  
  except Exception as e:
    print(e)


  return rps


async def run_benchmarks():
  proc = await asyncio.create_subprocess_exec( 'python', 'tests/s_bench.py', stdout=asyncio.subprocess.PIPE, stderr=asyncio.subprocess.PIPE )
  process = psutil.Process(proc.pid)

  await asyncio.sleep(1)

  if proc.returncode != None:
    print("tests/s_bench.py failed to start:")
    print(await proc.stdout.read())
    print(await proc.stderr.read())
    exit()

  print("Benchmarks")

  try:
  
    more_headers = ('-H','User-Agent: Mozilla/5.0 (X11; Linux x86_64) Gecko/20130501 Firefox/30.0 AppleWebKit/600.00 Chrome/30.0.0000.0 Trident/10.0 Safari/600.00',
      '-H','Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8',
      '-H','Accept-Language: en-US,en;q=0.5',
      '-H','Cookie: mrsession=43709dd361cc443e976b05714581a7fb; foo=fdsfdasdfasdfdsfasdfsdfsdfasdfas; short=fazc;',
      '-H','Connection: keep-alive')
    opts = ('-H','Cookie: mrsession=43709dd361cc443e976b05714581a7fb; foo=fdsfdasdfasdfdsfasdfsdfsdfasdfas; short=fazc;')

    print("  Pipelined")
    print ("    Hello          ", await run_wrk(loop, 'http://localhost:8080/',lua='tests/lua/pipeline.lua'), "Requests/second" )
    print ("    Hello          ", await run_wrk(loop, 'http://localhost:8080/',lua='tests/lua/pipeline.lua'), "Requests/second" )
    print ("    More hdrs      ", await run_wrk(loop, 'http://localhost:8080/',options=more_headers,lua='tests/lua/pipeline.lua'), "Requests/second" )
    print ("    Sessions       ", await run_wrk(loop, 'http://localhost:8080/s',lua='tests/lua/q-session.lua'), "Requests/second" )
    print ("    File Upload    ", await run_wrk(loop, 'http://localhost:8080/upload',lua='tests/lua/q-upload.lua'), "Requests/second" )
    print ("    mrpacker       ", await run_wrk(loop, 'http://localhost:8080/mrpacker',lua='tests/lua/q-mrp.lua'), "Requests/second" )
    print ("    Form           ", await run_wrk(loop, 'http://localhost:8080/form',lua='tests/lua/q-form.lua'), "Requests/second" )
    if 1:

      print("")
      print("  One by one")
      print ("    Hello          ", await run_wrk(loop, 'http://localhost:8080/'),             "Requests/second" )
      print ("    Hello hdrs     ", await run_wrk(loop, 'http://localhost:8080/', options=more_headers), "Requests/second" )
      print ("    Cookies        ", await run_wrk(loop, 'http://localhost:8080/printCookies', options=opts), "Requests/second" )
      print ("    many args      ", await run_wrk(loop, 'http://localhost:8080/sixargs/one/two/three/four/five/six'), "Requests/second" )
      print ("    404 natural    ", await run_wrk(loop, 'http://localhost:8080/dfads404/'), "Requests/second" )
      print ("    404            ", await run_wrk(loop, 'http://localhost:8080/404/'), "Requests/second" )
      print ("    Form parsing   ", await run_wrk(loop, 'http://localhost:8080/form',lua='tests/lua/form.lua'), "Requests/second" )
      #print ("   Templates      ", await run_wrk(loop, 'http://localhost:8080/template'),            "Requests/second" )
      print ("    mrpacker       ", await run_wrk(loop,'http://localhost:8080/mrpacker',lua='tests/lua/mrpacker.lua'), "Requests/second" )
      print ("    Sessions       ", await run_wrk(loop, 'http://localhost:8080/s',     options=opts), "Requests/second" )
      print ("    File Upload    ", await run_wrk(loop,'http://localhost:8080/upload',lua='tests/lua/upload.lua'), "Requests/second" )
      # Disabled in s_bench.py print ("Sessions (py)  ", run_wrk(loop, 'http://localhost:8080/pys',   options=opts), "Requests/second" )
      #print ("    Session login  ", await run_wrk(loop, 'http://localhost:8080/login'),               "Requests/second" )
      #print ("    json post      ", await run_wrk(loop,'http://localhost:8080/json',lua='tests/lua/json.lua'), "Requests/second" )
      #print ("    mrpacker py    ", await run_wrk(loop,'http://localhost:8080/mrpackerpy',lua='tests/lua/mrpacker.lua'), "Requests/second" )
      #print ("    msgpack py     ", await run_wrk(loop,'http://localhost:8080/msgpack',lua='tests/lua/msgpack.lua'), "Requests/second" )
    
      
      opts = ('-H','XX-Real-IP: 1.2.3.4')
      print ("    get ip         ", await run_wrk(loop,'http://localhost:8080/getip',options=opts), "Requests/second" )
      #print ("many num args  ", await run_wrk(loop, 'http://localhost:8080/sixargs/155/2001/29999/25/29999543/93243242394'), "Requests/second" )
      #print ("404            ", await run_wrk(loop, 'http://localhost:8080/404/'), "Requests/second" )
  

  except KeyboardInterrupt:
    pass
  finally:
    proc.terminate()
    await proc.wait()

async def main():
  print("main")  
  await run_tests()
  await run_benchmarks()

loop = uvloop.new_event_loop()
asyncio.set_event_loop(loop)
asyncio.run( main() )

