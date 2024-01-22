
import os
import ujson as json
#import mrjson as json

from socketify import App


def plaintext(res, req):
    res.send(b'Hello, World!')

def applicationjson(res, req):
    res.send({"message":"Hello, World!"})


def run_app():
    app = App(None, 200_000, 0)
    app.json_serializer(json)
    app.get("/", plaintext)
    app.get("/json", applicationjson)
    app.get("/plaintext", plaintext)
    app.listen(8080, None)
    app.run()

run_app()

