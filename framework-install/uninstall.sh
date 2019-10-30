workdir=$(cd $(dirname $0); pwd)


${workdir}/tars.sh stop

rm -rf ${workdir}/tarsnode_install/
rm -rf /usr/local/app/

cd ${workdir}/deploy

python deploy.py cleardb 

