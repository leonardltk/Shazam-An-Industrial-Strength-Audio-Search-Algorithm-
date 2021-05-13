#FROM harbor.bigo.sg/bigo_ai/infer/kafka-consumer-base
FROM  harbor.bigo.sg/bigo_ai/infer/kafka-consumer-base:pb3
# FROM  harbor.bigo.sg/bigo_ai/infer/kafka-consumer-base:v1.0
COPY ./ /workspace
WORKDIR /workspace
RUN sh build.sh && mkdir build/conf

# FROM ubuntu:16.04
# COPY  --from=0 /usr/lib/x86_64-linux-gnu/libgflags.so.2 \
#       /usr/lib/libprotobuf.so.24 \
#       /lib/x86_64-linux-gnu/libcrypto.so.1.0.0 \
#       /lib/x86_64-linux-gnu/libz.so.1 \
#       /usr/lib/x86_64-linux-gnu/libleveldb.so.1 \
#       /lib/x86_64-linux-gnu/libssl.so.1.0.0    \
#       /usr/lib/x86_64-linux-gnu/libx264.so.148  \
#       /usr/lib/x86_64-linux-gnu/libsnappy.so.1  /usr/lib/ 
# COPY --from=0 /workspace /workspace

WORKDIR /workspace/build

# CMD ["./kafka-consumer-cpp", "-usercode_in_pthread=true", "-config_file=../conf/configmap/hello/hello-audio-LL.conf"]
# CMD ["./kafka-consumer-cpp", "-usercode_in_pthread=true", "-config_file=../conf/configmap/hello/hello-audio-dev.conf"]

# CMD ["./kafka-consumer-cpp", "-usercode_in_pthread=true", "-config_file=../conf/configmap/helloyo/helloyo-audio-LL.conf"]
CMD ["./kafka-consumer-cpp", "-usercode_in_pthread=true", "-config_file=../conf/configmap/helloyo/helloyo-audio-dev.conf"]
