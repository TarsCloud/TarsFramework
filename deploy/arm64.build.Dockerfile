FROM arm64v8/centos:7

RUN yum makecache && yum install -y wget unzip git \
    yum clean all && rm -rf /var/cache/yum

RUN wget https://github.com/nvm-sh/nvm/archive/v0.35.1.zip;unzip v0.35.1.zip; cp -rf nvm-0.35.1 $HOME/.nvm

RUN echo 'NVM_DIR="$HOME/.nvm"; [ -s "$NVM_DIR/nvm.sh" ] && \. "$NVM_DIR/nvm.sh"; [ -s "$NVM_DIR/bash_completion" ] && \. "$NVM_DIR/bash_completion";' >> $HOME/.bashrc;

RUN source $HOME/.bashrc && nvm install v12.13.0

RUN yum install -y yum-utils psmisc make net-tools gcc gcc-c++ telnet openssl-devel bison flex \
    yum clean all && rm -rf /var/cache/yum

# Install cmake for cpp
RUN mkdir -p /tmp/cmake/  \
    && cd /tmp/cmake \
    && curl -O https://tars-thirdpart-1300910346.cos.ap-guangzhou.myqcloud.com/src/cmake-3.19.7.tar.gz  \
    && tar xzf cmake-3.19.7.tar.gz \
    && cd cmake-3.19.7 \
    && ./configure  \
    && make -j4 && make install \
    && rm -rf /tmp/cmake

ENV TARS_INSTALL /usr/local/tars/cpp/deploy

COPY web ${TARS_INSTALL}/web

COPY . /data

RUN cd /data && mkdir -p build-tmp && cd build-tmp && cmake .. && make -j4 && make install && strip ${TARS_INSTALL}/framework/servers/tars*/bin/tars* && cd / && rm -rf /data

RUN ${TARS_INSTALL}/tar-server.sh

ENTRYPOINT [ "/usr/local/tars/cpp/deploy/docker-init.sh"]
