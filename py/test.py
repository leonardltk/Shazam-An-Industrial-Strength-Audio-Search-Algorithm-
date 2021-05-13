import os, sys
import requests
from datetime import datetime
from kafka import KafkaConsumer
from kafka import KafkaProducer
from kafka import TopicPartition
import time
import json
import logging


logging.basicConfig(stream=sys.stdout, level=logging.INFO)
group_id = os.environ.get("GROUP_ID", "test_yxp")



def get_kafka_info(file_path):
  topic = None
  brokers = None
  with open(file_path, 'r') as fp:
    for line in fp:
      if "topic" in line:
        topic = line.split("[")[-1].split("]")[0].strip("\"").strip()
      if "brokers" in line:
        brokers = line.split("\"")[1].strip()
      if brokers is not None and topic is not None:
        break
  return brokers, topic


def copy_old(broker, topic, groupid):
    data_from = KafkaConsumer(topic,
                            group_id= groupid,
                            auto_offset_reset='earliest',
                            bootstrap_servers=broker,
                            consumer_timeout_ms=1000)

    data_to = KafkaProducer(bootstrap_servers=broker,compression_type="lz4")

    while True:
        item = data_from.poll(timeout_ms=100)
        if len(item) == 0:
            continue
        for key in item:
            lst = item[key]
            for ret in lst:
                data = json.loads(ret.value.decode("utf8"))
                print(data)  
                data_to.send(topic, json.dumps(data).encode("utf8") )
                time.sleep(3)

def scan_kafka(broker, topic, groupid):
    data_from = KafkaConsumer(topic,
                            group_id= groupid,
                            auto_offset_reset='earliest',
                            bootstrap_servers=broker,
                            consumer_timeout_ms=1000)

    data_to = KafkaProducer(bootstrap_servers=broker,compression_type="lz4")
    while True:
        item = data_from.poll(timeout_ms=100)
        if len(item) == 0:
            continue
        for key in item:
            lst = item[key]
            for ret in lst:
                data = json.loads(ret.value.decode("utf8"))
                print(data)  
                time.sleep(3)

copy_old("146.196.79.230:9094,146.196.79.231:9094,146.196.79.232:9094","dev_ml_likee_crawler_video_audit","1231231")
#copy_old("146.196.79.230:9094,146.196.79.231:9094,146.196.79.232:9094","grey_ml_bigolive_screenshot_video_audit","1231231")
scan_kafka("kafka-likee-conn1.bigdata-out.bigo.inner:9092,kafka-likee-conn2.bigdata-out.bigo.inner:9092,kafka-likee-conn3.bigdata-out.bigo.inner:9092", "likee-cv-ml_audit_pic_likee_avatar", "test")
