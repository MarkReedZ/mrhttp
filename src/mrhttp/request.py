import urllib.parse
import cgi, time
import encodings.idna
import collections
import mrhttp
import mrpacker
try:
  import mrjson as json
except:
  try:
    import ujson as json
  except:
    pass


File = collections.namedtuple('File', ['type', 'body', 'name'])

class Request(mrhttp.CRequest):

  response = mrhttp.Response()
  def __init__(self):
    super().__init__(self)
    pass

  def parsed_content_type(self):
    content_type = self.headers.get('Content-Type')
    if not content_type: 
      content_type = self.headers.get('content-type')
      if not content_type: return None, {}
    return cgi.parse_header(content_type)

  @property
  def mime_type(self):
    return self.parsed_content_type()[0]

  @property
  def encoding(self):
    return self.parsed_content_type()[1].get('charset')

  @property
  def form(self): 
    if self._form == None:
      if self.mime_type == 'application/x-www-form-urlencoded':
        #self._form = dict(urllib.parse.parse_qsl(self.body.decode("utf-8")))
        self.parse_urlencoded_form() # Sets _form
      elif self.mime_type == 'multipart/form-data':
        self.parse_mp_form() # Sets _form
    return self._form

  @property
  def json(self): 
    if self._json == None:
      try:
        if self.mime_type == "application/json":
          self._json = json.loads(self.body.decode("utf-8"))
      except:
        pass
    return self._json

  @property
  def file(self):
    if self._file == None:
      if self.mime_type == 'multipart/form-data':
        self.parse_mp_form()
    return self._file

  @property
  def files(self):
    if self._files == None:
      if self.mime_type == 'multipart/form-data':
        self.parse_mp_form()
    return self._files

  def set_usermrp(self, j):
    try:
      self.user = mrpacker.unpack(j)
    except:
      pass
  def set_user(self, j):
    try:
      self.user = json.loadb(j)
    except Exception as e:
      print(e)
      pass
    #try: #TODO bad json??
      #self.user = json.loads(j)
    #except Exception as e:
      #print("request.set_user exception:", e)
      #print("Error parsing json: ", j)



