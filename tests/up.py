import requests

with open('delme', 'r') as f:
    files = {"data": f}
    requests.post(f'http://localhost:8080/', files=files)
