# Mrhttp
Async Python 3.5+ web server written in C

# Benchmarks

```
Hello pipelined  4,152,858 Requests/second
Hello              633,097 Requests/second
404                654,053 Requests/second
Cookies            422,728 Requests/second
Form parsing       328,780 Requests/second
Parse JSON         224,872 Requests/second
Templates          257,753 Requests/second
Sessions:
  memcached        163,833 Requests/second
  mrcache          283,359 Requests/second
MrWorkServer       338,891 Requests/second
File Upload        132,242 Requests/second
```

Versus sanic a pure python async server

```
Hello World       64,366 Requests/second
Cookies           50,867 Requests/second
404                9,256 Requests/second
forms             27,104 Requests/second
sessions           4,053 Requests/second
File upload       21,457 Requests/second
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


