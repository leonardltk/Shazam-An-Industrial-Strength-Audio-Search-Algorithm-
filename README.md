# shash_serv

## Create docker
```bash
sudo docker login harbor.bigo.sg
bigoai
Bigo1992

Image=audioban-shash
ImageName=audioban_shash_name

## Build && Enter
sudo docker build . -t $Image && sudo docker run -it --rm --name "$ImageName" "$Image" bash
    sed -i "s,#force_color_prompt=yes,force_color_prompt=yes,g" ~/.bashrc
    source ~/.bashrc

## Re enter (if exited before)
sudo docker start $ImageName
sudo docker exec -it $ImageName /bin/bash

## Stop/Remove
sudo docker stop $ImageName
sudo docker rm $ImageName
```

# Build
Build first then send it into docker to compile
```bash
bash build.sh
```

## Copy To docker (after re-compiled)
```bash
sudo docker ps -a
# CONTAINER ID   IMAGE             NAMES
# xxxxxxxxxxxx   audioban-shash    audioban_shash_name
containerID=xxxxxxxxxxxx
sudo docker cp ./build/audio_ban_shash    $containerID:/bin
```

# To run within docker
```bash
./bin/audio_ban_shash 80
```
