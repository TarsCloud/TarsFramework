FROM centos/systemd

#ENV MIRROR http://mirrors.cloud.tencent.com
ENV MIRROR http://mirrors.cloud.tencent.com
ENV TARS_INSTALL /root/tars-install

COPY centos7_base.repo MariaDB.repo epel-7.repo /etc/yum.repos.d/

RUN yum makecache fast; yum install -y yum-utils psmisc MariaDB-client net-tools wget unzip telnet

COPY web ${TARS_INSTALL}/web

RUN wget https://github.com/nvm-sh/nvm/archive/v0.35.1.zip;unzip v0.35.1.zip; cp -rf nvm-0.35.1 $HOME/.nvm

RUN echo 'NVM_DIR="$HOME/.nvm"; [ -s "$NVM_DIR/nvm.sh" ] && \. "$NVM_DIR/nvm.sh"; [ -s "$NVM_DIR/bash_completion" ] && \. "$NVM_DIR/bash_completion";' >> $HOME/.bashrc;

RUN source $HOME/.bashrc; export NVM_NODEJS_ORG_MIRROR=${MIRROR}/nodejs-release;nvm install v12.13.0 ;npm config set registry ${MIRROR}/npm/;npm install -g npm pm2;cd ${TARS_INSTALL}/web; npm install;cd ${TARS_INSTALL}/web/demo;npm install

COPY framework ${TARS_INSTALL}/framework
COPY tools ${TARS_INSTALL}/tools
COPY docker-init.sh tars-install.sh tar-server.sh ${TARS_INSTALL}/

RUN ${TARS_INSTALL}/tar-server.sh

# RUN ls ${TARS_INSTALL}framework/servers/
RUN mkdir -p ${TARS_INSTALL}/web/files; cp -rf ${TARS_INSTALL}/framework/servers/*.tgz ${TARS_INSTALL}/web/files/
RUN rm -rf ${TARS_INSTALL}/framework/servers/*.tgz

ENTRYPOINT [ "/root/tars-install/docker-init.sh"]

EXPOSE 3000
EXPOSE 3001