# Mrhttp
Async Python 3.5+ web server written in C

# Benchmarks

```
Hello pipelined  4,152,858 Requests/second
Hello              633,097 Requests/second
404                654,053 Requests/second
Form parsing       214,668 Requests/second
Parse JSON         510,606 Requests/second
Templates          257,753 Requests/second
Sessions           589,414 Requests/second
Sessions           193,110 Requests/second
Sessions (py)       83,119 Requests/second
Session login      121,745 Requests/second
MrWorkServer       338,891 Requests/second
```

Versus sanic a pure python async server

```
Hello World       64,366 Requests/second
Cookies           50,867 Requests/second
404                9,256 Requests/second
sessions          29,053 Requests/second
```

Hello World Example
-------------------

```python

import mrhttp

app = mrhttp.Application()

@app.route('/')
def hello(r):
  return 'Hello World!'

app.run(cores=2)

```

Installation
------------

-  ``pip install mrhttp``


