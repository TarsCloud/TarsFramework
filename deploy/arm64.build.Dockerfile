FROM arm64v8/centos:7

RUN yum install -y yum-utils psmisc net-tools gcc gcc-c++ make wget unzip telnet \
    yum clean all && rm -rf /var/cache/yum

RUN wget https://github.com/nvm-sh/nvm/archive/v0.35.1.zip;unzip v0.35.1.zip; cp -rf nvm-0.35.1 $HOME/.nvm

RUN echo 'NVM_DIR="$HOME/.nvm"; [ -s "$NVM_DIR/nvm.sh" ] && \. "$NVM_DIR/nvm.sh"; [ -s "$NVM_DIR/bash_completion" ] && \. "$NVM_DIR/bash_completion";' >> $HOME/.bashrc;

RUN source $HOME/.bashrc && nvm install v12.13.0

RUN curl -O https://tars-thirdpart-1300910346.cos.ap-guangzhou.myqcloud.com/src/helm-v3.5.2-linux-amd64.tar.gz

# Install cmake for cpp
RUN mkdir -p /tmp/cmake/  \
    && cd /tmp/cmake \
    && curl -O https://tars-thirdpart-1300910346.cos.ap-guangzhou.myqcloud.com/src/cmake-3.19.7.tar.gz  \
    && tar xzf cmake-3.19.7.tar.gz \
    && cd cmake-3.19.7 \
    && ./configure  \
    && make -j4 && make install \
    && rm -rf /tmp/cmake


RUN yum install git -y && yum clean all && rm -rf /var/cache/yum

RUN cd /data && mkdir -p build-tmp && cd build-tmp && cmake .. && make -j4 && make install

ENV TARS_INSTALL /usr/local/tars/cpp/deploy

RUN web ${TARS_INSTALL}/web

ENTRYPOINT [ "/usr/local/tars/cpp/deploy/docker-init.sh"]
