

import mrhttp

import tenjin
tenjin.set_template_encoding('utf-8')
from tenjin.helpers import *
engine = tenjin.Engine(path=['templates'])


app = mrhttp.Application()

@app.route('/')
def index(r):
  context = { "world":"all you python fanatics out there!" } 
  return engine.render('example.ten', context)


app.run(cores=2)

