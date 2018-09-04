
from mrhttp import app

@app.route('/tst', type="text")
def inex(r):
  return "llo World!"


