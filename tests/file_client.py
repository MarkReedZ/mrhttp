
import requests

url = 'http://localhost:8080/files'
files = {'file':  open('z', 'rb'),
        'file2': open('zz', 'rb')}
r = requests.post(url, files=files)
print(r.text)

req = requests.Request('POST',url, files=files)
r = req.prepare()
print(r.headers)
print(r.body)
