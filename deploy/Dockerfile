FROM centos:7

#ENV MIRROR http://mirrors.cloud.tencent.com
ENV MIRROR http://mirrors.cloud.tencent.com
ENV TARS_INSTALL /root/tars-install

COPY centos7_base.repo MariaDB.repo epel-7.repo /etc/yum.repos.d/
#COPY MariaDB.repo /etc/yum.repos.d/
#COPY epel-7.repo /etc/yum.repos.d/

RUN yum makecache fast

#RUN yum install -y yum-utils epel-release wget mariadb-libs psmisc MariaDB-client telnet net-tools make gcc gcc-c++
RUN yum install -y yum-utils epel-release wget psmisc MariaDB-client telnet net-tools

COPY web ${TARS_INSTALL}/web

# RUN wget -qO- https://raw.github.com/creationix/nvm/master/install.sh | sh
COPY nvm-install.sh .
RUN sh nvm-install.sh

#RUN export NVM_DIR="$HOME/.nvm"; [ -s "$NVM_DIR/nvm.sh" ] && \. "$NVM_DIR/nvm.sh"; [ -s "$NVM_DIR/bash_completion" ] && \. "$NVM_DIR/bash_completion";export NVM_NODEJS_ORG_MIRROR=${MIRROR}/nodejs-release;nvm install v12.13.0 ;npm config set registry ${MIRROR}/npm/;npm install -g npm pm2;cd ${TARS_INSTALL}/web;npm install
RUN export NVM_DIR="$HOME/.nvm"; [ -s "$NVM_DIR/nvm.sh" ] && \. "$NVM_DIR/nvm.sh"; [ -s "$NVM_DIR/bash_completion" ] && \. "$NVM_DIR/bash_completion";export NVM_NODEJS_ORG_MIRROR=${MIRROR}/nodejs-release;nvm install v12.13.0 ;npm config set registry ${MIRROR}/npm/;npm install -g npm pm2;cd ${TARS_INSTALL}/web;npm prune;npm audit fix

COPY framework ${TARS_INSTALL}/framework
COPY tools ${TARS_INSTALL}/tools
COPY docker-init.sh tars-install.sh ${TARS_INSTALL}/