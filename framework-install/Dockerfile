FROM centos:7

#ENV MIRROR http://mirrors.cloud.tencent.com
ENV MIRROR http://mirrors.cloud.tencent.com
ENV TARS_INSTALL /root/tars-install

COPY centos7_base.repo /etc/yum.repos.d/
COPY MariaDB.repo /etc/yum.repos.d/
RUN yum makecache fast

RUN yum install -y yum-utils epel-release wget mariadb-libs psmisc MariaDB-client telnet net-tools make gcc gcc-c++

COPY web ${TARS_INSTALL}/web

# RUN wget -qO- https://raw.github.com/creationix/nvm/master/install.sh | sh
COPY nvm-install.sh .
RUN sh nvm-install.sh

RUN export NVM_DIR="$HOME/.nvm"; [ -s "$NVM_DIR/nvm.sh" ] && \. "$NVM_DIR/nvm.sh"; [ -s "$NVM_DIR/bash_completion" ] && \. "$NVM_DIR/bash_completion";export NVM_NODEJS_ORG_MIRROR=${MIRROR}/nodejs-release;nvm install v12.13.0 ;npm config set registry ${MIRROR}/npm/;npm install -g npm pm2;cd ${TARS_INSTALL}/web;npm install

#COPY pip.conf ~/.pip/pip.conf

#RUN pip install -i http://mirrors.cloud.tencent.com/pypi/simple --trusted-host mirrors.cloud.tencent.com --upgrade pip;pip install  -i http://mirrors.cloud.tencent.com/pypi/simple --trusted-host mirrors.cloud.tencent.com  requests

#COPY deploy /root/tars-install/deploy
COPY framework ${TARS_INSTALL}/framework
COPY tools ${TARS_INSTALL}/tools
COPY docker-init.sh ${TARS_INSTALL}/
COPY tars.sh ${TARS_INSTALL}/
COPY tars-install.sh ${TARS_INSTALL}/