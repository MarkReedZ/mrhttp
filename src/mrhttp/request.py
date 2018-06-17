import urllib.parse
import cgi
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



class Request(mrhttp.CRequest):

  response = mrhttp.Response()
  user = None
  memo = {}
  def __init__(self):
    super().__init__(self)
    pass

  # decorator
  def memoize(func):
    def wrapper(self):
      try:
        return self.memo[func.__name__]
      except KeyError:
        pass
      except Exception as e:
        print("wrapper exception", e)

      try:
        result = func(self)
        self.memo[func.__name__] = result
      except Exception as e:
        print("wrapfunc exception", e)


      return result
    return wrapper

  #TODO we reuse requests so we need to clear memo to use it
  def parsed_content_type(self):
    content_type = self.headers.get('Content-Type')
    if not content_type: return None, {}
    return content_type, {} #cgi.parse_header(content_type)

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
      boundary = self.get_form()[1]['boundary'].encode('utf-8')
      return parse_multipart_form(self.body, boundary)

    return None, None

  @property
  def form(self): 
    return self.get_form()[0]

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

z="""
@memoize
def text(request):
    if request.body is None:
        return None

    return request.body.decode(request.encoding or 'utf-8')


@memoize
def json(request):
    if request.body is None:
        return None

    return json_loads(request.text)


@memoize
def query(request):
    qs = request.query_string
    if not qs:
        return {}
    return dict(urllib.parse.parse_qsl(qs))


def remote_addr(request):
    return request.transport.get_extra_info('peername')[0]


@memoize
def parsed_content_type(request):
    content_type = request.headers.get('Content-Type')
    if not content_type:
        return None, {}

    return cgi.parse_header(content_type)


def mime_type(request):
    return parsed_content_type(request)[0]


def encoding(request):
    return parsed_content_type(request)[1].get('charset')


@memoize
def parsed_form_and_files(request):
    if request.mime_type == 'application/x-www-form-urlencoded':
        return dict(urllib.parse.parse_qsl(request.text.decode("utf-8"))), None
    elif request.mime_type == 'multipart/form-data':
        boundary = parsed_content_type(request)[1]['boundary'].encode('utf-8')
        return parse_multipart_form(request.body, boundary)

    return None, None


def form(request):
    return parsed_form_and_files(request)[0]


def files(request):
    return parsed_form_and_files(request)[1]


@memoize
def hostname_and_port(request):
    host = request.headers.get('Host')
    if not host:
        return None, None

    hostname, *rest = host.split(':', 1)
    port = rest[0] if rest else None

    return encodings.idna.ToUnicode(hostname), int(port)


def port(request):
    return hostname_and_port(request)[1]


def hostname(request):
    return hostname_and_port(request)[0]


def parse_cookie(cookie):
    ""Parse a ``Cookie`` HTTP header into a dict of name/value pairs.
    This function attempts to mimic browser cookie parsing behavior;
    it specifically does not follow any of the cookie-related RFCs
    (because browsers don't either).
    The algorithm used is identical to that used by Django version 1.9.10.
    ""
    cookiedict = {}
    for chunk in cookie.split(str(';')):
        if str('=') in chunk:
            key, val = chunk.split(str('='), 1)
        else:
            # Assume an empty name per
            # https://bugzilla.mozilla.org/show_bug.cgi?id=169091
            key, val = str(''), chunk
        key, val = key.strip(), val.strip()
        if key or val:
            # unquote using Python's algorithm.
            cookiedict[key] = unquote_cookie(val)
    return cookiedict


@memoize
def cookies(request):
    if 'Cookie' not in request.headers:
        return {}

    try:
        cookies = parse_cookie(request.headers['Cookie'])
    except Exception:
        return {}

    return {k: urllib.parse.unquote(v) for k, v in cookies.items()}


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
"""
