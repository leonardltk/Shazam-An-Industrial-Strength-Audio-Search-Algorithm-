#1.0 Dev image
FROM harbor.bigo.sg/bigo_ai/infer/ci/brpc-builder:like_core

RUN apt-get update && \
    apt-get install -y libfftw3-dev libgflags-dev libprotobuf-dev libleveldb-dev libssl-dev libx264-dev && \
    apt-get install -y libsndfile-dev && \
    apt-get clean

COPY ./3rdparty /workspace/3rdparty
COPY ./data /workspace/data
COPY ./proto /workspace/proto
COPY ./include /workspace/include
COPY ./proto /workspace/include
COPY ./src /workspace/src
COPY CMakeLists.txt build.sh /workspace/
# to see chinese characters
RUN LANG=en_US.UTF-8

WORKDIR /workspace

## Get the necessary files
# RUN . path_shash.sh "AllDatabase_crop" "KnownPositiveNegatives"

## Database Files
# COPY AllDatabase_crop.tar /workspace/
# RUN tar -xf AllDatabase_crop.tar 

## Query Files
# COPY KnownPositiveNegatives.333.tar KnownPositiveNegatives.485.tar /workspace/
# RUN tar -xf KnownPositiveNegatives.333.tar && \
#   tar -xf KnownPositiveNegatives.485.tar && \
#   mv wav/KnownPositiveNegatives/2Million/* wav/KnownPositiveNegatives/14Million/* wav/KnownPositiveNegatives && \
#   rm -r wav/KnownPositiveNegatives/2Million wav/KnownPositiveNegatives/14Million

# RUN sh build.sh


#2.0 Runtime image
# FROM harbor.bigo.sg/bigo_ai/infer/ci/brpc-builder:like_core
# RUN apt-get update && apt-get install gdb -y
# COPY 3rdparty/brpc/lib/* /app/
# COPY 3rdparty/decode_video/lib/* /app/
# COPY 3rdparty/bigo_ml_service/lib/* /app/
# COPY ./model /app
# COPY --from=0 /workspace/build/model_inference /app
# ENV LD_LIBRARY_PATH $LD_LIBRARY_PATH:./:/app:/app/lib
# WORKDIR /app
# CMD ["./model_inference", "-connect_timeout_as_unreachable=100", "-log_connected=true", "-port=80", "-service_name=like-core"]
