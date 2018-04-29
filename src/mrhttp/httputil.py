
from http.server import BaseHTTPRequestHandler

response_codes = BaseHTTPRequestHandler.responses.copy()
response_codes[404] = ('Not Found',
                       'The requested page was not found')
response_codes[500] = ('Internal Server Error',
                       'The server encountered an unexpected condition '
                       'which prevented it from fulfilling the request.')
response_codes[503] = ('Service Unavailable',
                       'The server is currently unable to handle the '
                       'request due to a temporary overloading or '
                       'maintenance of the server.')

class MrhttpPyException(Exception):
    pass

class HTTPRedirect(MrhttpPyException):
  """Exception used to return an HTTP redirect code (3xx) to the client.
  Defaults to 301 Moved Permanently
  See https://en.wikipedia.org/wiki/List_of_HTTP_status_codes#3xx_Redirection
  """
  url = None
  code = None
  def __init__(self, url, code=301):
    self.url = url
    self.code = code
    if self.code < 300 or self.code > 399:
      raise ValueError('status code must be between 300 and 399.')

 
class HTTPError(MrhttpPyException):

  """Exception used to return an HTTP error code (4xx-5xx) to the client.
  See `RFC2616 <http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html#sec10.4>`_
  Examples::
      raise mrhttp.HTTPError(403)
  """

  code     = None
  reason   = None
  _message = None

  def __init__(self, code=500):
    self.code = code
    if self.code < 400 or self.code > 599:
      raise ValueError('status code must be between 400 and 599.')
    try:
      self.reason, defaultmsg = response_codes[self.code]
      #print(self.reason)
      #print(defaultmsg)
    except ValueError:
      raise self.__class__(500, _exc_info()[1].args[0])

    self._message = defaultmsg
    MrhttpPyException.__init__(self, code)
