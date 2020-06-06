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
    && export NVM_NODEJS_ORG_MIRROR=${MIRROR}/nodejs-release && nvm install v12.13.0

COPY framework ${TARS_INSTALL}/framework
COPY tools ${TARS_INSTALL}/tools
COPY docker-init.sh tars-install.sh tar-server.sh web-install.sh mysql-tool ${TARS_INSTALL}/


RUN ${TARS_INSTALL}/tar-server.sh

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