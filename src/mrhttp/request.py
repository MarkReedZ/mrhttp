import urllib.parse
import cgi
import encodings.idna
import collections
import mrhttp, time
try:
  import mrjson as json
except:
  try:
    import ujson as json
  except:
    pass



class Request(mrhttp.CRequest):

  response = mrhttp.Response()
  user = None
  servers_down = False
  memo = {}
  _json = None
  mrpack = None
  def __init__(self):
    super().__init__(self)
    pass

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

  def get_form(self):
    content_type = self.headers.get('Content-Type')
    # TODO This can be sped up considerably in C (parse_qsl)
    if self.mime_type == 'application/x-www-form-urlencoded':
      return dict(urllib.parse.parse_qsl(self.body.decode("utf-8"))), None

    elif self.mime_type == 'multipart/form-data':
      boundary = self.parsed_content_type()[1]['boundary'].encode('utf-8')
      return parse_multipart_form(self.body, boundary)

    return None, None

  @property
  def form(self): 
    return self.get_form()[0]

  @property
  def json(self): 
    if self._json == None:
      try:
        if self.mime_type == "application/json":
          self._json = json.loads(self.body.decode("utf-8"))
      except Exception as e:
        pass
        #print("Error parsing json", str(e))
    return self._json

  @property
  def files(self):
    return self.get_form()[1]

  def set_user(self, j):
    try: #TODO bad json??
      self.user = json.loads(j)
    except Exception as e:
      print("request.set_user exception:", e)
      print("Error parsing json: ", j)


  #def __repr__(self):
      #return '<HttpRequest {0.method} {0.path} {0.version}, {1} headers>' \
          #.format(self, len(self.headers))

File = collections.namedtuple('File', ['type', 'body', 'name'])

def parse_multipart_form(body, boundary):
    files = {}
    fields = {}

    form_parts = body.split(boundary)
    for form_part in form_parts[1:-1]:
        file_name = None
        file_type = None
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
            form_header_field = form_line[0:colon_index]
            form_header_value, form_parameters = cgi.parse_header(
                form_line[colon_index + 2:])

            if form_header_field == 'Content-Disposition':
                if 'filename' in form_parameters:
                    file_name = form_parameters['filename']
                field_name = form_parameters.get('name')
            elif form_header_field == 'Content-Type':
                file_type = form_header_value

        post_data = form_part[line_index:-4]
        if file_name or file_type:
            file = File(type=file_type, name=file_name, body=post_data)
            files[field_name] = file
        else:
            value = post_data.decode('utf-8')
            fields[field_name] = value

    return fields, files

