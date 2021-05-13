import configargparse, os, sys, requests, json, base64, re, subprocess, shutil, time
import pdb, traceback, ast, pprint
from tqdm import tqdm
pp = pprint.PrettyPrinter(indent=4)

def get_parser():
    parser = configargparse.ArgumentParser(formatter_class=configargparse.ArgumentDefaultsHelpFormatter)
    # 
    parser.add_argument("--api", type=str,          help="api, for example: http://127.0.0.1:8010/api/audio-feature")
    parser.add_argument("--src_file", type=str,     help='text file that contains urls of samples, first col url, second col vid')
    parser.add_argument("--out_file", type=str,     help="outout file")
    parser.add_argument("--input_mode", type=str,   help="{audio,video}, whether we are uploading wav files or mp4 files")
    parser.add_argument("--service_name", type=str, default="hello-shash-serv")
    return parser

def updateRequest_Video(src, ID, MyUid="3841557125"):
    return {
            "video": base64.b64encode(open(src, "rb").read()).decode("ascii"),
            "id": MyUid,
            "post_id": ID,
            "op": "query",
            "label" : os.path.basename(src).replace('.mp4', ''),
            "cache": False,
        }
def updateRequest_Audio(src, ID, MyUid="3841557125"):
    # "data":  base64.b64encode(load_pcm_s16_sr16000(src)).decode("ascii"),
    return {
            "op": "query",
            "id": MyUid,
            "post_id": ID,
            "data": base64.b64encode(open(src, "rb").read()).decode("ascii"),
            "url": src,
            "label" : os.path.basename(src).replace('.mp4', ''),
            "cache": False,
        }

def main(cmd_args):
    parser = get_parser()
    args, _ = parser.parse_known_args(cmd_args)

    f_w = open(args.out_file, 'w')
    print(f"Writing to {args.out_file}")
    f_w.write(f"name\tdb_vid\tresult\n")

    flen = 10
    # flen = int(subprocess.run(["wc", "-l", args.src_file], capture_output=True).stdout.split()[0])
    x, y, average_time = 0, 0, 0.0
    with open(args.src_file, "r") as fopen:
        with tqdm(fopen, total=flen, smoothing=0) as pbar:
            for i, line in enumerate(pbar):
                try:


                    src,ID = line.strip().split(",", maxsplit=1)
                    if args.input_mode == "audio":
                        request = updateRequest_Audio(src, ID)
                    elif args.input_mode == "video":
                        request = updateRequest_Video(src, ID)
                    else:
                        """
                            src_json = "{""voice"":[{""content"":""http://music-fsorigin.ppx520.com/cn/audit-music/5c4/M02/22/BB/aGoUAGBaEH2Eb_tzAAAAABO0eyQ214.raw"",""id"":""57988210"",""subExtraData"":{""download_ip"":""111.174.0.67:80"",""duration"":""19.500000"",""download_status_code"":""200""},""detailMlResult"":{""explos"":""pass"",""porn"":""pass"",""voice_activity"":""pass""},""modelInfo"":{""asr-ign"":""empty"",""explos"":""0.002572"",""voice_activity"":""0.075682"",""audio_core"":""{\""model_name\"":[\""explos\"",\""porn\"",\""voice_activity\""],\""score\"":[0.0025724750012159349,0.00027766928542405367,0.07568187266588211],\""threshold\"":[0.0,0.7371000051498413,0.0],\""version\"":\""1.5\""}"",""porn"":""0.000278""},""threshold"":{""voice_activity"":0.15449999272823335,""porn"":0.7371000051498413,""explos"":0.9459999799728394}}]}"
                        src_json = src_json.replace('""', '"')
                        src_json = src_json[1:-1]
                        src_dict = ast.literal_eval(src_json)
                        raw_url = src_dict['voice'][0]['content']
                        """
                        request = {}

                    sess = requests.session()
                    response = sess.post(url=args.api, json=request)

                    if (request["op"] == "query") and (response.status_code == 200):
                        response_json = response.json()
                        # _ = response_json["results"].pop('shash')
                        if response_json["status"] != 200:
                            tqdm.write(f"ID {ID} response status fail with {response.text}")
                            continue
                        y += 1
                        if not "act" in response_json:
                            tqdm.write(f"ID {ID} response status code fail without act and text {response.text}")
                            pdb.set_trace()
                            continue

                        act = response_json['act']
                        current_time = response.elapsed.total_seconds()
                        average_time = (i * average_time + current_time) / (i+1)
                        tqdm.write(f"query succeed with ID {ID}, act response: {act}, current time {current_time} and average time {average_time}")

                        name, ext = os.path.splitext(os.path.basename(src))

                        if act == 1:
                            db_vid = response_json['db_vid']
                            result = response_json['label_string']
                            num_matches = response_json['num_matches']
                            qry_vid = response_json['qry_vid']
                            ret = response_json['ret']
                            assert ret == 0, f"ret={ret}"
                            x += 1
                        elif act == 0:
                            db_vid = 0
                            result = ""
                        else:
                            pdb.set_trace()

                        f_w.write(f"{name}\t{db_vid}\t{result}\n")
                        tqdm.write(f"\t name={name}\tdb_vid={db_vid}\tresult={result}")
                    else:
                        tqdm.write(f"ID {ID} response status code fail with {response.text}")

                except:
                    traceback.print_exc()
                    pdb.set_trace()

                pbar.set_description(f"Checked: {x}/{y}|{x/(y+10e-8)*100:.2f}%")
    print(f"{x/(y+10e-8)*100}% hit with total {y} with {x} hitted")

    print(f"Finished writing to {args.out_file}")
    f_w.close()

if __name__ == "__main__":
    main(sys.argv[1:])
    print("Done")
    print('python ' + " ".join(sys.argv))

"""
pdb.set_trace()
"""

""" Check Precision Recall
python checkTP.py out_file.Pos.txt out_file.Neg.txt

wc -l qry2db2label out_file.Pos.txt out_file.Neg.txt
head -n3 qry2db2label out_file.Pos.txt out_file.Neg.txt
"""

"""
wavfile="./wav/KmoUAF8uto6Ebi-3AAAAAKQ1Nzs520__out16k_0.wav"
wavfile="wav/AllDatabase/53、绝世小攻（苏仨）.wav"
jpgpath=./mp4/3096.jpg
ffmpeg \
    -loop 1 \
    -i "$jpgpath" \
    -i "$wavfile" \
    -c:a aac \
    -ab 112k \
    -c:v libx264 \
    -shortest \
    -strict \
    -2 out.mp4
"""

"""
wavfile="wav/AllDatabase/53、绝世小攻（苏仨）.wav"
jpgpath=./mp4/3096.jpg
CroppedWavPath="tmp.wav"
duration=50;mp4path="out.crop.mp4"
duration=5;mp4path="out.short.mp4"
duration=1;mp4path="out.1.mp4"

sox ${wavfile} ${CroppedWavPath} trim 0 $duration
sox --i "./wav/KmoUAF8uto6Ebi-3AAAAAKQ1Nzs520__out16k_0.wav" ${wavfile} ${CroppedWavPath}
ffmpeg \
    -y \
    -loop 1 \
    -i "$jpgpath" \
    -i "$CroppedWavPath" \
    -c:a aac \
    -ab 112k \
    -c:v libx264 \
    -shortest \
    -strict \
    -2 "$mp4path"

"""
