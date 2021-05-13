# Deploy shash query server

## Create docker
```bash
sudo docker login harbor.bigo.sg
bigoai
Bigo1992

Image=hello_shash_serv
ImageName=dreamy_bhaskara

## Build
sudo docker build . -t $Image

## Enter
sudo docker run -it "$Image" bash
sed -i "s,#force_color_prompt=yes,force_color_prompt=yes,g" ~/.bashrc
source ~/.bashrc

## Re enter (if exited before)
sudo docker start $ImageName
sudo docker exec -it $ImageName /bin/bash

## Stop/Remove
sudo docker ps -a
sudo docker stop $ImageName
sudo docker rm $ImageName
```

## Run server
```bash
./audio_ban_shash 80 -service_name=hello-shash-serv
```

## Copy To docker (to re-compile)
```bash
sudo docker ps -a
# CONTAINER ID   IMAGE           	PORTS     NAMES
# 4c03aade18ec   flask_shash_serv			  blissful_lichterman
containerID=4c03aade18ec
sudo docker cp ./lib    $containerID:/
```


# What were copied over
```bash
## My pre-built audio_ban_shash
SSDir=/data3/lootiangkuan/Projects/AudioHashing/shash_serv
cp -v $SSDir/build/audio_ban_shash ./
cp -v $SSDir/conf/* ./model/
```
