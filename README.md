# Mrhttp
Async Python 3.5+ web server written in C

# Benchmarks

```
pipelined      3,939,751 Requests/second
Hello World      523,384 Requests/second
Cookies          492,241 Requests/second
404              339,629 Requests/second
form parsing     231,255 Requests/second
sessions (c)     521,972 Requests/second
sessions (py)     53,711 Requests/second

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
app = Application()

@app.route('/', type="text")
def hello(r):
  return 'Hello World!'

app.run(cores=1)

```

Installation
------------

-  ``pip install mrhttp``


