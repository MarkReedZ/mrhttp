# Mrhttp
Async Python 3.5+ web server written in C

# Benchmarks

```
  Pipelined
    Hello           6834994.51 Requests/second
    More hdrs       6193307.49 Requests/second
    Sessions        4396364.13 Requests/second
    File Upload     3510289.14 Requests/second
    mrpacker        2052674.93 Requests/second
    Form            1182228.98 Requests/second

  One by one
    Hello           707667.74 Requests/second
    Hello hdrs      728639.36 Requests/second
    Cookies         588212.04 Requests/second
    many args       691910.28 Requests/second
    404 natural     763643.3 Requests/second
    404             580424.69 Requests/second
    Form parsing    338553.65 Requests/second
    mrpacker        533242.09 Requests/second
    Sessions        325354.58 Requests/second
    File Upload     292331.03 Requests/second
    get ip          503454.35 Requests/second
    
```

Versus sanic a pure python async server

```
Hello World       22,366 Requests/second
Cookies           20,867 Requests/second
404                8,256 Requests/second
forms             11,104 Requests/second
sessions           4,053 Requests/second
File upload        1,457 Requests/second
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

```
sudo apt install python3-dev -y
pip3 install mrhttp
```


