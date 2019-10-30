TARS_PATH=/usr/local/app/tars

TARS=(tarsregistry tarsAdminRegistry tarsnode tarsnotify tarsconfig  tarslog tarspatch  tarsproperty  tarsqueryproperty  tarsquerystat tarsstat)

case $1 in
    "start")
        for var in ${TARS[@]};
        do
            sh ${TARS_PATH}/${var}/util/start.sh
            sleep 1
        done
        pid=`ps -ef | grep tars-node-web | grep -v grep | awk -F' ' '{print $2}'`
        if echo $pid | grep -q '[^0-9]'
        then
            cd /usr/local/app/web/; npm run start 
        fi
        ;;
    "stop")
        for var in ${TARS[@]};
        do
            killall -9 ${var}
        done
        pid=`ps -ef | grep tars-node-web | grep -v grep | awk -F' ' '{print $2}'`
        if [ "$pid" != "" ]; then
            kill -9 $pid
        fi
        ;;
    *)
        echo "$0 start|stop"
esac
