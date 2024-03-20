# Mrhttp
Async Python 3.5+ web server written in C

# Benchmarks

```
  Pipelined
    Hello (cached)  8,534,332 Requests/second
    Hello           6,834,994 Requests/second
    More hdrs       6,193,307 Requests/second
    Sessions        4,396,364 Requests/second
    File Upload     3,510,289 Requests/second
    mrpacker        2,052,674 Requests/second
    Form            1,182,228 Requests/second

  One by one
    Hello           707,667 Requests/second
    Hello hdrs      728,639 Requests/second
    Cookies         588,212 Requests/second
    many args       691,910 Requests/second
    404 natural     763,643 Requests/second
    404             580,424 Requests/second
    Form parsing    338,553 Requests/second
    mrpacker        533,242 Requests/second
    Sessions        325,354 Requests/second
    File Upload     292,331 Requests/second
    get ip          503,454 Requests/second
    
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


