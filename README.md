# Mrhttp
Async Python 3.5+ web server written in C

# Benchmarks

```
Hello pipelined  4,152,858 Requests/second
Hello              613,335 Requests/second
Cookies            564,410 Requests/second
404                388,735 Requests/second
Form parsing       230,286 Requests/second
Templates          274,153 Requests/second
Sessions           589,414 Requests/second
Sessions (py)       67,119 Requests/second
Session login      121,745 Requests/second
Write Queue (MrQ)  428,110 Requests/second
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


