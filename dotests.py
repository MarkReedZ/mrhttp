import pkgutil, time
import inspect
import types 
import importlib

import tests

# TODO
#  Check for memcached being up and add the session key so we hit and load the json 43709dd361cc443e976b05714581a7fb

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

server_fut = asyncio.create_subprocess_exec( 'python3', 'tests/s_bench.py', stdout=asyncio.subprocess.PIPE )
server = loop.run_until_complete(server_fut)
process = psutil.Process(server.pid)

time.sleep(1)
try:

  opts = ('-H','Cookie: mrsession=43709dd361cc443e976b05714581a7fb;')
  print ("Hello pipelined", run_wrk(loop, 'http://localhost:8080/',lua='tests/lua/pipeline.lua'), "Requests/second" )
  print ("Hello          ", run_wrk(loop, 'http://localhost:8080/'),             "Requests/second" )
  print ("Cookies        ", run_wrk(loop, 'http://localhost:8080/printCookies'), "Requests/second" )

  # TODO Look into speeding this up
  print ("404            ", run_wrk(loop, 'http://localhost:8080/404/'), "Requests/second" )

  # TODO Look into speeding this up
  print ("Form parsing   ", run_wrk(loop, 'http://localhost:8080/form',lua='tests/lua/form.lua'), "Requests/second" )

  print ("Templates      ", run_wrk(loop, 'http://localhost:8080/template'),            "Requests/second" )
  print ("Sessions       ", run_wrk(loop, 'http://localhost:8080/s',     options=opts), "Requests/second" )
  print ("Sessions (py)  ", run_wrk(loop, 'http://localhost:8080/pys',   options=opts), "Requests/second" )

  # TODO - We should be able to speed this up by generating the session id in C not python. 
  print ("Session login  ", run_wrk(loop, 'http://localhost:8080/login'),               "Requests/second" )

  #print ("json post      ", run_wrk(loop,'http://localhost:8080/form'), "Requests/second" )

except KeyboardInterrupt:
  pass
finally:
  server.terminate()
  loop.run_until_complete(server.wait())

