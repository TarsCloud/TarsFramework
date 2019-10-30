
workdir=$(cd $(dirname $0); pwd)

case $1 in
    "build")
        if [ $# -lt 2 ]; then
            echo $0 build version
            exit
        fi
        strip ${workdir}/framework/build/deploy/tars*/bin/*
        strip ${workdir}/framework/build/deploy/tars*/*
        docker build ${workdir}/. -t tars-docker:$2
        ;;
    "run")
        if [ $# -lt 6 ]; then
            echo $0 run version mysql-host mysql-port mysql-root-password web-port
            exit
        fi
        docker run -p $6:3000 -e mysql.host="$3" -e mysql.root.port="4" -e mysql.root.password="$5"  tars-docker:$2  sh /root/tars-install/install.sh check
        ;;
    *)
        echo "$0 build version"
        echo "$0 run version mysql-host mysql-port mysql-root-password web-port"
esac
