FROM centos:7

#ENV MIRROR http://mirrors.cloud.tencent.com
ENV MIRROR http://mirrors.cloud.tencent.com
ENV TARS_INSTALL /root/tars-install

COPY centos7_base.repo MariaDB.repo epel-7.repo /etc/yum.repos.d/

#RUN yum makecache fast; yum install -y yum-utils epel-release wget psmisc MariaDB-client telnet net-tools git
#RUN yum makecache fast; yum install -y yum-utils psmisc MariaDB-client net-tools wget unzip make gcc gcc-c++
RUN yum makecache fast; yum install -y yum-utils psmisc MariaDB-client net-tools wget unzip telnet

COPY web ${TARS_INSTALL}/web

#COPY nvm $HOME/.nvm

RUN  wget https://github.com/nvm-sh/nvm/archive/v0.35.1.zip;unzip v0.35.1.zip; cp -rf nvm-0.35.1 $HOME/.nvm

#RUN export NVM_DIR="$HOME/.nvm"; [ -s "$NVM_DIR/nvm.sh" ] && \. "$NVM_DIR/nvm.sh"; [ -s "$NVM_DIR/bash_completion" ] && \. "$NVM_DIR/bash_completion";export NVM_NODEJS_ORG_MIRROR=${MIRROR}/nodejs-release;nvm install v12.13.0 ;npm config set registry ${MIRROR}/npm/;npm install -g npm pm2;cd ${TARS_INSTALL}/web;npm prune;npm i --package-lock-only;npm audit fix
RUN echo 'NVM_DIR="$HOME/.nvm"; [ -s "$NVM_DIR/nvm.sh" ] && \. "$NVM_DIR/nvm.sh"; [ -s "$NVM_DIR/bash_completion" ] && \. "$NVM_DIR/bash_completion";' >> $HOME/.bashrc;

RUN source $HOME/.bashrc; export NVM_NODEJS_ORG_MIRROR=${MIRROR}/nodejs-release;nvm install v12.13.0 ;npm config set registry ${MIRROR}/npm/;npm install -g npm pm2;
RUN cd ${TARS_INSTALL}/web && . /root/.bashrc && npm --registry https://registry.npm.taobao.org install --quiet 
COPY framework ${TARS_INSTALL}/framework
COPY tools ${TARS_INSTALL}/tools
COPY docker-init.sh tars-install.sh ${TARS_INSTALL}/
