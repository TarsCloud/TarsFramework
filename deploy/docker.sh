
if [ $# -lt 1 ]; then
    echo $0 version
    exit
fi

workdir=$(cd $(dirname $0); pwd)

strip ${workdir}/framework/servers/tars*/bin/tars*
docker rmi -f tarscloud/framework:$1
docker build --no-cache ${workdir}/. -t tarscloud/framework:$1

