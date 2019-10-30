FROM centos:7

#RUN yum install -y yum-utils epel-release wget
#RUN wget -O /etc/yum.repos.d/CentOS-Base.repo http://mirrors.cloud.tencent.com/repo/centos7_base.repo
#RUN yum makecache fast

COPY centos7_base.repo /etc/yum.repos.d/
COPY MariaDB.repo /etc/yum.repos.d/
RUN yum makecache fast

#RUN yum install -y MySQL-python python-devel mariadb-libs psmisc python-pip MariaDB-client telnet net-tools make gcc gcc-c++
RUN yum install -y yum-utils epel-release wget mariadb-libs psmisc MariaDB-client telnet net-tools make gcc gcc-c++

COPY web /root/tars-install/web

#RUN wget -qO- https://raw.github.com/creationix/nvm/master/install.sh | sh
COPY nvm-install.sh .
RUN sh nvm-install.sh

RUN export NVM_DIR="$HOME/.nvm"; [ -s "$NVM_DIR/nvm.sh" ] && \. "$NVM_DIR/nvm.sh"; [ -s "$NVM_DIR/bash_completion" ] && \. "$NVM_DIR/bash_completion";export NVM_NODEJS_ORG_MIRROR=https://npm.taobao.org/mirrors/node;nvm install v12.13.0 ;npm config set registry http://mirrors.cloud.tencent.com/npm/;npm install -g npm;cd /root/tars-install/web;npm install

#COPY pip.conf ~/.pip/pip.conf

#RUN pip install -i http://mirrors.cloud.tencent.com/pypi/simple --trusted-host mirrors.cloud.tencent.com --upgrade pip;pip install  -i http://mirrors.cloud.tencent.com/pypi/simple --trusted-host mirrors.cloud.tencent.com  requests

#COPY deploy /root/tars-install/deploy
COPY framework /root/tars-install/framework
COPY tools /root/tars-install/tools
COPY centos-install.sh /root/tars-install/
COPY tars.sh /root/tars-install/
