#!/usr/bin/env python
# -*- coding: utf-8 -*-

# @File Name : kafka_push_test.py
# @Purpose :
# @Creation Date : 2020-12-29 17:29:41
# @Last Modified : 2021-01-29 15:40:08
# @Created By :  chenjiang
# @Modified By :  chenjiang


import functools
print = functools.partial(print, flush=True)

from kafka import KafkaConsumer, KafkaProducer
import json, time, hashlib, ast
import pdb

import pprint
pp = pprint.PrettyPrinter(indent=4)

## Specified in :
##              conf/configmap/helloyo/helloyo-audio-dev.conf
topic = "leo_shash_audit"
##              conf/configmap/hello/hello-audio-dev.conf
topic = "dev_ml_hello_audio_audit"
##              conf/configmap/helloyo/helloyo-audio-dev.conf
topic = "dev_ml_helloyo_audio_audit"
group_id = topic

# http://b230512bed38:8000

## message
message = {
            "appId": "18",
            "businessType": "helloyo_audio_push",
            "country": "CN",
            "reportTime": 1615998218001,
            "sourceType": "1",
            "extradata": {
                "ml_review_callback": "1",
                "isp": "1",
                "usernum": "0",
                "ispassroom": "0",
                "cost_downloadermodule": "78",
                "sid": "3655819997",
            },
            "timestamp": None,
            "data": {
                "voice": [
                {
                    "content": "http://eu-audit-fsorigin.bigo.sg/eu/audit-audio/7u5/M00/10/00/4L4cAF_q_YuEHd_uAAAAAKU68Es002.raw",
                    "subExtraData": None,
                    "detailMlResult": None,
                    "modelInfo": None
                }
                ]
            },
            "callbackUrl": "https://hk-ml-api.bigo.sg/va_strategy_ml_result_sg/call/voice-result"
        }
## To fill in kafka_producer()
message["orderId"]  = "5b0afbb02fb1b278d6fcdcb4bdb20c9c"
message["uid"]      = "3841557125"
message['data']['voice'][0]['content'] = "http://music-fsorigin.ppx520.com/cn/audit-music/5c5/M0D/D5/4E/pmoUAGBSLQqEf_L-AAAAAD-ybm8301.raw"
message['data']['voice'][0]['id'] = "3841557125"

def kafka_producer(vid, raw_url):
    producer = KafkaProducer(bootstrap_servers=['146.196.79.230:9094', '146.196.79.231:9094', '146.196.79.232:9094'])
    send_msg = message
    # send_msg['uid'] = uid
    send_msg['data']['voice'][0]['content'] = raw_url
    send_msg['data']['voice'][0]['id'] = vid
    order_id = hashlib.md5((raw_url + str(time.time())).encode('utf8')).hexdigest()
    send_msg['orderId'] = order_id

    print(f'\tkafka_producer() : uid = {send_msg["uid"]}')
    print(f'\tkafka_producer() : vid = {vid}')
    print(f'\tkafka_producer() : order_id = {order_id}')
    print(f'\tkafka_producer() : raw_url  = {raw_url}')
    # 1;pp.pprint(send_msg)
    send_msg = json.dumps(send_msg).encode('utf8')

    print(f'\tkafka_producer() : producer.send({topic}, send_msg)')
    producer.send(topic, send_msg)

    time.sleep(0.05)


if __name__ == "__main__":
    '''
    pcd size > 0
    http://audit-fsoriginsg.bigo.sg/sg/audit-audio/cs5/M04/0E/CA/31yNAGBhjoOIW-U-AACI3LIFjhQADnIdwBlXAAAAJAA518.raw

    pcd size == 0 ?
    http://audit-fsoriginsg.bigo.sg/sg/audit-audio/cs7/M02/0E/FB/W12NAGBZ3nyITlb8AAFvJCS_71sADqEmwCKsAAAAXAA758.raw
    http://audit-fsoriginsg.bigo.sg/sg/audit-audio/cs2/M0C/06/BC/JVyNAGBi2JOIa_d4AAAGrq3BeaMABpRwQGMZgMAAAbG651.raw
    '''

    TestLst = './data/1000000files.tsv'
    TestLst = './data/3files.tsv'
    with open(TestLst) as reader:
        for idx, line in enumerate(reader):
            QueryDate, vid, raw_url = line.strip().split("\t", maxsplit=2)

            print(f"\nmain() : QueryDate={QueryDate}, vid={vid}, raw_url={raw_url}")

            kafka_producer(vid, raw_url)
            # time.sleep(0.1)

            # pdb.set_trace()

    print(f'Done')
"""
!import code; code.interact(local=vars())
import json, time, hashlib
pdb.set_trace()

conda deactivate
conda activate Py3_debug
python kafka_push_hive.py
"""
