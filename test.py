import sys
import json
import base64
import requests
from conf import value

url = "http://107.155.44.110:8080/grey/{}".format(value.service_name)

if len(sys.argv) < 2:
  print("Usage: {0} <VideoPath>".format(sys.argv[0]))
  sys.exit(0)

while 1: 
  with open(sys.argv[1], 'rb') as fp:
    video = base64.b64encode(fp.read()).decode("ascii")
    data = {"id": "123", "video": video}
    res = requests.post(url, data=json.dumps(data))
    print(res.text)
