FROM ubuntu:20.04

RUN apt-get update && \ 
    apt-get install -y g++ libpng-dev make sudo && \ 
    rm -rf /var/lib/apt/lists/*

WORKDIR /home

ADD . .

RUN make

RUN apt-get remove g++ make -y

CMD [ "sudo", "./main" ]

# from alpine:3.12.1

# RUN set -ex && apk add --no-cache --update g++ libpng-dev make sudo

# WORKDIR /home

# ADD . .

# RUN make

# CMD [ "sudo", "./main" ]