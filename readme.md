# Deploy shash feat extraction server

## Create docker
```bash
sudo docker login harbor.bigo.sg
bigoai
Bigo1992

Image=kafka-shash
ImageName=kafka_name

Image=kafka-shash-updatedbranch
ImageName=kafka_name_updatedbranch

## Build &  Enter
# sudo docker build . -t $Image
# sudo docker run -it "$Image" bash
sudo docker build . -t $Image && sudo docker run -it --rm --name "$ImageName" "$Image" bash
    sed -i "s,#force_color_prompt=yes,force_color_prompt=yes,g" ~/.bashrc
    source ~/.bashrc
    ./kafka-consumer-cpp -usercode_in_pthread=true -config_file=../conf/configmap/helloyo/helloyo-audio-dev.conf

## Re enter (if exited before)
sudo docker start $ImageName
sudo docker exec -it $ImageName /bin/bash

## Stop/Remove
sudo docker ps -a
sudo docker stop $ImageName
sudo docker rm $ImageName
```

## Copy To docker (to re-compile)
```bash
sudo docker ps -a
# CONTAINER ID   IMAGE        PORTS     NAMES
# 9265d40a9ce2   kafka-shash            kafka_name
containerID=20ea099489a7

sudo docker cp ./proto/submodule.proto                  $containerID:/workspace/proto
sudo docker cp ./src/submodule/audio_audit_shash.cpp    $containerID:/workspace/src/submodule
sudo docker cp ./src/submodule/audio_audit_shash.h      $containerID:/workspace/src/submodule
sudo docker cp ./conf/configmap/helloyo/helloyo-audio-dev.conf    $containerID:/workspace/conf/configmap/helloyo

# sudo docker cp ./conf/configmap/hello/hello-audio-LL.conf    $containerID:/workspace/conf/configmap/hello
# sudo docker cp ./src/dispatcher.cpp     $containerID:/workspace/src
# sudo docker cp ./src/dispatcher.h       $containerID:/workspace/src
# sudo docker cp ./src/submodule/audio_audit_shash.h      $containerID:/workspace/src
# sudo docker cp ./src/submodule/audio_audit_shash.cpp    $containerID:/workspace/src

```

## Run server
```bash
## hello
# For alpha testing, use LL conf
# For beta testing, use dev conf
./kafka-consumer-cpp -usercode_in_pthread=true -config_file=../conf/configmap/hello/hello-audio-LL.conf
./kafka-consumer-cpp -usercode_in_pthread=true -config_file=../conf/configmap/hello/hello-audio-dev.conf
## helloyo
# For alpha testing, use LL conf
# For beta testing, use dev conf
# ./kafka-consumer-cpp -usercode_in_pthread=true -config_file=../conf/configmap/helloyo/helloyo-audio-LL.conf

./kafka-consumer-cpp -usercode_in_pthread=true -config_file=../conf/configmap/helloyo/helloyo-audio-dev.conf
```


# What were copied over
```bash
## Charlie's deployed kafka-yhash stuffs
cp -v ../kafka-yhash/src/submodule/audio_audit.* ./src/submodule/
cp -v ../kafka-yhash/src/dispatcher.* ./src/
cp -v ../kafka-yhash/src/module.* ./src/
cp -v ../kafka-yhash/src/alarm_metric.* ./src/
cp -rv ../kafka-yhash/alarm ./
```

# Deployment
```
Please test locally and move to dev environment,
Do not deploy to online environment!
It will affect all online services.
```

