# pull source and build docker auto in docker hub
FROM ubuntu:20.04

WORKDIR /root/

ENV DEBIAN_FRONTEND=noninteractive
# Install
RUN apt update 
RUN apt install -y mysql-client build-essential unzip make flex bison net-tools wget cmake psmisc telnet iputils-ping vim \
    && apt install -y libprotobuf-dev libprotobuf-c-dev \
    && apt install -y zlib1g-dev curl libssl-dev \
    && apt install -y npm \
    && npm install -g pm2 n \
    && n install v16.13.0 \
    && apt-get clean

# Get and install nodejs
RUN rm /bin/sh && ln -s /bin/bash /bin/sh

ENV TARS_INSTALL /usr/local/tars/cpp/deploy

COPY web ${TARS_INSTALL}/web

COPY . /data

RUN cd /data && rm -rf build-tmp && mkdir -p build-tmp && cd build-tmp && cmake .. && make -j4 && make install && strip ${TARS_INSTALL}/framework/servers/tars*/bin/tars* && cd / && rm -rf /data

RUN ${TARS_INSTALL}/tar-server.sh

ENTRYPOINT [ "/usr/local/tars/cpp/deploy/docker-init.sh"]

#web
EXPOSE 3000
#tarslog
EXPOSE 18993
#tarspatch
EXPOSE 18793
#tarsqueryproperty
EXPOSE 18693
#tarsconfig
EXPOSE 18193
#tarsnotify
EXPOSE 18593
#tarsproperty
EXPOSE 18493
#tarsquerystat
EXPOSE 18393
#tarsstat
EXPOSE 18293
#tarsAdminRegistry
EXPOSE 12000
#tarsnode
EXPOSE 19385
#tarsregistry
EXPOSE 17890
EXPOSE 17891
