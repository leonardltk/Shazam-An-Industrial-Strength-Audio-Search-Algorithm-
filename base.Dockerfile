#1.0 build bmls, if you need
#FROM  harbor.bigo.sg/bigo_ai/infer/ci/bml-mini:dev
#COPY bigo-ml-service-mini/ /workspace
#WORKDIR /workspace
#RUN cd /workspace && sh build.sh

#2.0 Add Decoder if need
#FROM harbor.bigo.sg/bigo_ai/infer/proxy-sidecar-gpu:v418

#2.0 build runtime
FROM harbor.bigo.sg/bigo_ai/infer/flask

RUN pip3 install prometheus_client
