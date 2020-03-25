FROM centos/systemd

#ENV MIRROR http://mirrors.cloud.tencent.com
ENV MIRROR http://mirrors.cloud.tencent.com
ENV TARS_INSTALL /usr/local/tars/cpp/deploy/

COPY centos7_base.repo /etc/yum.repos.d/

RUN mkdir -p ${TARS_INSTALL} \
    && yum makecache fast \
    && yum install -y yum-utils psmisc net-tools wget unzip telnet \
    yum clean all && rm -rf /var/cache/yum

COPY web ${TARS_INSTALL}/web

RUN wget https://github.com/nvm-sh/nvm/archive/v0.35.1.zip;unzip v0.35.1.zip; cp -rf nvm-0.35.1 $HOME/.nvm

RUN echo 'NVM_DIR="$HOME/.nvm"; [ -s "$NVM_DIR/nvm.sh" ] && \. "$NVM_DIR/nvm.sh"; [ -s "$NVM_DIR/bash_completion" ] && \. "$NVM_DIR/bash_completion";' >> $HOME/.bashrc;

RUN source $HOME/.bashrc \
    && export NVM_NODEJS_ORG_MIRROR=${MIRROR}/nodejs-release && nvm install v12.13.0 \
    && npm config set registry ${MIRROR}/npm/ && npm install -g npm pm2 \
    && cd ${TARS_INSTALL}/web && npm install \
    && cd ${TARS_INSTALL}/web/demo && npm install

COPY framework ${TARS_INSTALL}/framework
COPY tools ${TARS_INSTALL}/tools
# COPY docker-init.sh tars-install.sh tar-server.sh ${TARS_INSTALL}/
COPY docker-init.sh tars-install.sh web-install.sh ${TARS_INSTALL}/

# RUN ${TARS_INSTALL}/tar-server.sh

RUN mkdir -p /data/tars/app_log \
    && mkdir -p /data/tars/web_log \
    && mkdir -p /data/tars/demo_log \
    && mkdir -p /data/tars/patchs \
    && mkdir -p /data/tars/tarsnode-data \
    && mkdir -p /usr/local/app/tars/ \
    && mkdir -p /usr/local/app/web/ \
    && mkdir -p /usr/local/app/web/demo/ \
    && mkdir -p /usr/local/app/tars/tarsnode \
    && ln -s /data/tars/app_log /usr/local/app/tars/app_log \
    && ln -s /data/tars/web_log /usr/local/app/web/log \
    && ln -s /data/tars/demo_log /usr/local/app/web/demo/log \
    && ln -s /data/tars/patchs /usr/local/app/patchs \
    && ln -s /data/tars/tarsnode-data /usr/local/app/tars/tarsnode/data

ENTRYPOINT [ "/usr/local/tars/cpp/deploy/docker-init.sh"]

#web
EXPOSE 3000
#user system
EXPOSE 3001
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
tarsquerystat
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