import requests
import time
import json
import os
import sys
import ipaddress

history = {}
name = os.environ.get("NAME", "va_audit_call_likee_keyword_proxy")
regionid = int(os.environ.get("REGIONID", "-1"))

def get_from_daemon(name):
    global history
    url = 'http://157.119.233.204:3000/daemonquery?name=' + name
    r = requests.get(url)

    flush = False
    for obj in json.loads(r.text)['favours']:
        server = obj['favor3']
        ep = '{0}:{1}'.format(".".join(str(ipaddress.IPv4Address(server['ips'][0]['ip'])).split(".")[::-1]), server['port'])
        if ep not in history:
            flush = True
            break
    if not flush:
        return None
    history = {} 
    ign_ep = {}
    with open("/dev/shm/keywords.endpoints", 'w') as fp:
      print("Flush table. ")
      for obj in json.loads(r.text)['favours']:
          server = obj['favor3']
          history[ep] = True
          ep = '{0}:{1}'.format(".".join(str(ipaddress.IPv4Address(server['ips'][0]['ip'])).split(".")[::-1]), server['port'])
          if regionid >= 0 and obj["regionId"] != regionid:
              ign_ep[ep] = True
              print("Igonre ip: {0}, because regionid not euqal to {1}".format(ep, regionid))
              continue
          fp.write(ep)
          fp.write('\n')
          print(ep)
      if len(ign_ep) == len(history):
          for i in history:
              fp.write(i)
              fp.write('\n')
              print(i)


if __name__ == "__main__":
  print("Name: {}".format(name), flush=True)
  while 1:
    get_from_daemon(name)
    print("Sleep")
    time.sleep(3)
