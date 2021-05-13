FROM harbor.bigo.sg/bigo_ai/infer/likee-audio-audit:serv

COPY audio_ban_shash /app
COPY hash_sysnc.py /app
COPY service_script.conf /app
COPY redis_config.yml /app 

RUN md5sum /app/audio_ban_shash
RUN mkdir /etc/shash_bak
RUN mkdir /etc/shash

ADD model/Info /etc/shash
ADD model/Labels /etc/shash

WORKDIR /app

CMD ["./audio_ban_shash", "80", "-service_name=hello-shash-serv"]
# CMD ["supervisord","-c","./service_script.conf"]
