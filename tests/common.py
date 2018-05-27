
import sys, os, subprocess, signal
#import ctypes.util
import time
import psutil
import inspect

#num_fails = 0 

#stdout=subprocess.PIPE
def start_server( script, suppress_error=False ):
  server = subprocess.Popen([sys.executable, script], stdout=subprocess.PIPE,stderr=subprocess.PIPE, start_new_session=True)
  #(o, e) = server.communicate()
  process = psutil.Process(server.pid)
  time.sleep(0.5)
  # If server is not running fail
  if not (server.poll() is None):
    if not suppress_error:
      print("ERROR starting server",script)
    return None
  return server

def eq( a, b ):
  #global num_fails
  if a != b:
    cf = inspect.currentframe()
    print( "ERROR Line", cf.f_back.f_lineno, a, "!=", b )
    #print( "ERROR Line", cf.f_back.f_code.co_filename, cf.f_back.f_lineno, a, "!=", b )
    #num_fails += 1
    return -1
  return 0

def contains( a, b ):
  if not b in a:
    cf = inspect.currentframe()
    print( "ERROR Line", cf.f_back.f_lineno, "'"+b+"'", " not found in\n  " , a )
   
def stop_server( s ):
  if s == None: return
  os.killpg(os.getpgid(s.pid), signal.SIGTERM)  
  s.terminate() 
