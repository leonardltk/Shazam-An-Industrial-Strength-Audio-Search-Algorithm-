import configargparse
import redis
import requests
import sys
import os
import base64
import sched
import time
import yaml
from datetime import datetime

def define_parser():
    parser = configargparse.ArgumentParser(
                formatter_class=configargparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument("dir", type=str, help="update dir")
    parser.add_argument("--pure_upload", action="store_true")
    parser.add_argument("--pure_download", action="store_true")
    parser.add_argument("--dump_url", type=str, default=None, help="audio db dump url")
    parser.add_argument("--update_min", type=int, default=0, help="update minutes")
    parser.add_argument("--update_sec", type=int, default=0, help="update secondes")
    parser.add_argument("--redis_config", type=str, default="redis_config.yml", help="the config file of redis")
    return parser


def hash_sync_args_check(args):
    update_sec = args.update_min * 60 + args.update_sec
    if update_sec < 0:
        raise ValueError(f"the update sec is less than 0")
    if not os.path.isdir(args.dir):
        raise ValueError(f"{args.dir} dir isn't exists")
    files_list = ["Info", "Labels"]
    files_dict = dict()
    for file_ in files_list:
        file_path = os.path.join(args.dir, file_)
        files_dict[file_] = file_path
    return files_dict, update_sec

def init_redis(config):
    redis_host = config["redis"]["host"]
    redis_port = config["redis"]["port"]
    redis_pwd = config["redis"]["pwd"]
    print(f"init redis at {redis_host}:{redis_host}")
    connection = redis.ConnectionPool(host=redis_host, port=redis_port, password=redis_pwd, socket_timeout=3000)
    redis_client = redis.Redis(connection_pool=connection)
    return redis_client


def redis_uploader(redis_client, files_dict, prefix=None):
    print(f"start upload with files: {files_dict.keys()}")
    for k, v in files_dict.items():
        with open(v, 'rb') as fileObj:
            file_data = base64.b64encode(fileObj.read())
        k = k if prefix is None else prefix + "_" + k
        redis_client.set(k, file_data)
    t = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime())
    redis_client.set(prefix + "_" + "time", t)
    print(f"end upload with files: {files_dict.keys()}")


def redis_downloader(redis_client, files_dict, prefix=None):
    print(f"start download with files: {files_dict.keys()}")
    for k, v in files_dict.items():
        k = k if prefix is None else prefix + "_" + k
        file_data = redis_client.get(k)
        with open(v, "wb") as fileObj:
            fileObj.write(base64.b64decode(file_data))
    print(f"end download with files: {files_dict.keys()}")


def dump_redis_uploader(dump_url, redis_client, files_dict, prefix=None):
    global update_sec
    global schedule
    response = requests.get(dump_url)
    if response.status_code == 200:
        print("dump succeed")
        redis_uploader(redis_client, files_dict, prefix)
    else:
        print(f"dump failed with status_code {response.status_code}")
        print(f"and text {response.text}")
    schedule.enter(
        update_sec, 0, dump_redis_uploader,
        (dump_url, redis_client, files_dict, prefix))


def main(cmd_args):
    parser = define_parser()
    args,_ = parser.parse_known_args(cmd_args)
    global update_sec
    global schedule
    files_dict, update_sec = hash_sync_args_check(args)
    
    # init
    # copy from https://git.sysop.bigo.sg/infer-serving/flask-deploy/blob/imo_audio_userAttr_offline/server.py
    config = yaml.safe_load(open(args.redis_config))
 
    redis_client = init_redis(config)
    if args.pure_upload:
        print(f"pure upload with files: {files_dict.keys()}")
        redis_uploader(redis_client, files_dict, config['redis']['prefix'])
    elif args.pure_download:
        print(f"pure download with files: {files_dict.keys()}")
        redis_downloader(redis_client, files_dict, config['redis']['prefix'])
    else:
        redis_downloader(redis_client, files_dict, config['redis']['prefix']) 

        schedule = sched.scheduler(time.time, time.sleep)
        schedule.enter(
            update_sec, 0, dump_redis_uploader,
                (args.dump_url, redis_client, files_dict, config['redis']['prefix']))
        print(f"upload with files: {files_dict.keys()} and time period {update_sec}")
        schedule.run()
        
if __name__ ==  "__main__":
    main(sys.argv[1:])
