import urllib.parse
import cgi, time
import encodings.idna
import collections
import mrhttp
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
  user = None
  servers_down = False
  def __init__(self):
    super().__init__(self)
    pass

  #TODO we reuse requests so we need to clear memo to use it
  def parsed_content_type(self):
    content_type = self.headers.get('Content-Type')
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
        self._form = dict(urllib.parse.parse_qsl(self.body.decode("utf-8")))
        #TODO self._form = self.parse_urlencoded_form()
      elif self.mime_type == 'multipart/form-data':
        self.parse_mp_form()
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

  def set_user(self, j):
    try: #TODO bad json??
      self.user = json.loads(j)
    except Exception as e:
      print("request.set_user exception:", e)
      print("Error parsing json: ", j)


  def parse_multipart_form(self, body, boundary):

    #print(boundary)
    files = {}
    fields = {}
    form_parts = body.split(boundary)

    st = time.time()
    for form_part in form_parts[1:-1]:
      content_type = 'text/plain'
      content_charset = 'utf-8'
      file_name = None
      field_name = None
      line_index = 2
      line_end_index = 0
      while not line_end_index == -1:
        line_end_index = form_part.find(b'\r\n', line_index)
        form_line = form_part[line_index:line_end_index].decode('utf-8')
        line_index = line_end_index + 2

        if not form_line:
            break

        colon_index = form_line.index(':')
        form_header_field = form_line[0:colon_index].lower()
        form_header_value, form_parameters = cgi.parse_header( form_line[colon_index + 2:])

        if form_header_field == 'content-disposition':
            file_name = form_parameters.get('filename')
            field_name = form_parameters.get('name')
        elif form_header_field == 'content-type':
            content_type = form_header_value
            content_charset = form_parameters.get('charset', 'utf-8')

      if field_name:
        post_data = form_part[line_index:-4]
        if file_name:
            form_file = File(type=content_type,
                             name=file_name,
                             body=post_data)
            if field_name in files:
                files[field_name].append(form_file)
            else:
                files[field_name] = [form_file]
        else:
            value = post_data.decode(content_charset)
            if field_name in fields:
                fields[field_name].append(value)
            else:
                fields[field_name] = [value]

    print("took", time.time()-st)
    return fields, files



