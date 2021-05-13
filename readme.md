# shash_feature

## Create docker
```bash
## Install nvidia docker like in
# https://docs.nvidia.com/datacenter/cloud-native/container-toolkit/install-guide.html#docker

sudo docker login harbor.bigo.sg
bigoai
Bigo1992

Image=shash-feature
ImageName=shash_feature_name

## Build && Enter
# sudo docker build . -t $Image && sudo docker run -it --gpus all --rm --name "$ImageName" "$Image" bash
sudo docker build . -t $Image && sudo docker run -it --rm --name "$ImageName" "$Image" bash
    sed -i "s,#force_color_prompt=yes,force_color_prompt=yes,g" ~/.bashrc
    source ~/.bashrc

## Re enter (if exited before)
sudo docker start $ImageName
sudo docker exec -it $ImageName /bin/bash

## Stop/Remove
ImageName=laughing_matsumoto && sudo docker stop $ImageName && sudo docker rm $ImageName
```

# Run in docker
```bash
bash build.sh
# ./build/server_feat -num_decoder 1 -num_hasher 1
```

## Copy To and From docker (to re-compile)
```bash
sudo docker ps -a
# CONTAINER ID   IMAGE         COMMAND    PORTS     NAMES
# xxxxxxxxxxxx   shash-feature "bash"               shash_feature_name
containerID=xxxxxxxxxxxx
sudo docker cp ./build.sh    $containerID:/workspace
sudo docker cp $containerID:/workspace/build/server_feat ./build/
```

# Testing
After running CI/CD
China (Hello)
    ApiHttp="http://hello-shash-feat.hello:80/api/hello-shash-feat"                 ## Within Cluster
    ApiHttp="http://infer-china.ingress.bigoml.cc:8080/api/hello/hello-shash-feat"  ## Out of Cluster
International (HelloYo)
    ApiHttp="http://hello-shash-feat.hello:80/api/hello-shash-feat"                 ## Within Cluster
    ApiHttp="http://infer-sg.ingress.bigoml.cc:8080/api/hello/hello-shash-feat"     ## Out of Cluster
```bash
conda activate Py3_debug
ApiHttp="http://infer-sg.ingress.bigoml.cc:8080/api/hello/hello-shash-feat" ## Out of Cluster
SrcFile="./data/single_query/wav.3.lst"; input_mode="audio"; OutFile="./data/single_query/out.wav.3.txt"
SrcFile="./data/single_query/mp4.3.lst"; input_mode="video"; OutFile="./data/single_query/out.mp4.3.txt"
# SrcFile="./data/multiple_query/query-sparksql-6107327.csv"; OutFile="./data/multiple_query/out_file.txt"
python single_query.both.py \
    --api $ApiHttp \
    --src_file $SrcFile \
    --out_file $OutFile \
    --input_mode $input_mode \
    --service_name "hello-shash-serv"
```



# What were copied over
```bash
## Charlie's 3rdparty libraries
git lfs clone https://git.sysop.bigo.sg/tanghaoyu/audio-feature "audio-feature-lfs"
CharlieDir=/data3/lootiangkuan/Projects/AudioHashing/audio-feature-lfs
cp -r $CharlieDir/3rdparty/{concurrentqueue,decode,ffmpeg} ./3rdparty

## 阳兴平's example of compiling with brpc
git clone --single-branch --branch "hello-audio" https://git.sysop.bigo.sg/infer-serving/brpc-deploy
BrpcDir=/data3/lootiangkuan/Projects/AudioHashing/brpc-deploy
cp -r $BrpcDir/* ./

## From local shash_cpp
ShashCppDir=/data3/lootiangkuan/Projects/AudioHashing/shash_cpp_new
mkdir -pv ./3rdparty/include/fftw-3.3.8
cp -rv $ShashCppDir/utils/fftw-3.3.8/api/fftw3.h ./3rdparty/include/fftw-3.3.8
cp -rv $ShashCppDir/AudioLL.* $ShashCppDir/Shash.* $ShashCppDir/fir_api.* $ShashCppDir/typedef.h ./src
cp -v $ShashCppDir/Shazam_hashing.cpp ./
```
