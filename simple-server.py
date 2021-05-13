#!/bin/python3
from flask import Flask
from flask import request
from flask import Response
from flask import *
import time
import sys
import logging
from werkzeug.middleware.dispatcher import DispatcherMiddleware
from prometheus_client import start_http_server, Summary, Counter, Histogram

import os
import librosa
import base64
import value
import queue
import json
import numpy as np


# name format: bigoai_svc_${name}_${type-info}
# metrics will be collect by promethues
# more ref: https://github.com/prometheus/client_python
# monitor url: http://metrics-sysop.bigo.sg/d/LJ_uJAvmk/istio-service-dashboard?orgId=1&refresh=10s
PING_REQUEST_LANTENCY_SUM  = Summary('bigoai_svc_ping_summary', '')
PING_REQUEST_LANTENCY_HIST = Histogram('bigoai_svc_ping_histgram', '')
PING_REQUEST_CNT           = Counter('bigoai_svc_ping_counter', '')

logging.basicConfig(stream=sys.stdout, level=logging.DEBUG)

service_name    = value.service_name
work_index      = int(os.environ.get("WORK_INDEX", "-1"))
ins_num_per_gpu = value.ins_num_per_gpu
model_path      = value.model_path

app = Flask(__name__)

# model_queue = queue.Queue()
# for i in range(ins_num_per_gpu):
#     if os.path.isfile(model_path):
#         model = torch.jit.load(model_path)
#         model.eval()
#     else:
#         raise Exception('Model not exist')
#     d = Decoder(second_mode=1, req_sample_rate=16000, use_nvidia_sdk=0, dev_id=-1, only_audio=True)
#     model_queue.put((model, d))


@app.route("/api/" + service_name, methods=['POST'])
def proxy():
    tag = []
    resp_dict = {}
    resp_dict['version'] = value.service_version
    resp_dict['status'] = 200
    resp_dict['valid duration'] = 0
    resp_dict['prediction'] = 'None'
    resp_dict['model_outputs'] = {'VAD': 0, 'Sing': 0, 'BGM': 0}
    model, d = model_queue.get()

    try:
        data_dic = json.loads(request.get_data().decode('utf-8'))
        wav_bytes = base64.b64decode(data_dic['audio_base64'].encode('utf8'))
    
        try:
            _, y, _ = d.decode(np.frombuffer(wav_bytes, dtype=np.uint8))
            validDur = 20
            resp_dict['valid duration'] = float(validDur)
        except Exception as err:
            reps_dict["status"] = 400
            reps_dict["msg"] = "Failed to decode audio"
            return Response(json.dumps(resp_dict), mimetype="application/json;charset=UTF-8", status=500)

        if validDur <= 2:
            resp_dict["prediction"] = "Silence"
        else:
            audio = torch.zeros((1, 20*16000))
            audio[0, :len(y)] = torch.tensor(y[:20*16000])
            # audio[0, :len(y)] = torch.tensor(y[-20*16000:])
            # audio[0, :len(y[5*16000:25*16000])] = torch.tensor(y[5*16000:25*16000])
            with torch.no_grad():
                glob_prob, _ = model(audio.cuda())
            probs = glob_prob.cpu().numpy()
            for idx, k in enumerate(['VAD', 'Sing', 'BGM']):
                resp_dict['model_outputs'][k] = float(probs[idx])

            if resp_dict['model_outputs']['VAD'] >= 0.2:
               resp_dict['prediction'] = 'Speak'
            elif resp_dict['model_outputs']['BGM'] > 0.8 or resp_dict['model_outputs']['Sing'] > 0.8:
                resp_dict['prediction'] = 'Only Songs'
            else:
                resp_dict['prediction'] = 'Noise'

        return Response(json.dumps(resp_dict), mimetype="application/json;charset=UTF-8")

    except:
        # if something bad happen
        resp_dict['status'] = 500
        return Response(json.dumps(resp_dict), mimetype="application/json;charset=UTF-8", status=500)

    finally:
        model_queue.put((model, d))


@app.route("/api/ping")
@app.route("/ping")
@PING_REQUEST_LANTENCY_SUM.time()
@PING_REQUEST_LANTENCY_HIST.time()
def Ping():
  PING_REQUEST_CNT.inc()
  return "pong"

start_http_server(3000)
app.run(host="103.240.149.46", port=4477)
# app.run(host="0.0.0.0", port=1974)
