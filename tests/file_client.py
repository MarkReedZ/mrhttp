
import requests

url = 'http://localhost:8080/'
files = {'file':  open('z', 'rb'),
        'file2': open('zz', 'rb')}

r = requests.post(url, files=files)
print(r.text)
