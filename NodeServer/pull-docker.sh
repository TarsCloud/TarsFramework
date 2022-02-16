#!/bin/bash

function LOG_ERROR() {
  msg=$(date +%Y-%m-%d" "%H:%M:%S)
  msg="${msg} $*"
  echo -e "\033[31m $msg \033[0m"
}

function LOG_WARNING() {
  msg=$(date +%Y-%m-%d" "%H:%M:%S)
  msg="${msg} $*"
  echo -e "\033[33m $msg \033[0m"
}

function LOG_INFO() {
  echo -e "\033[31m $* \033[0m"
}

function LOG_DEBUG() {
  msg=$(date +%Y-%m-%d" "%H:%M:%S)
  msg="${msg} $*"
  echo -e "\033[40;37m $msg \033[0m"
}

#  image.c_str(),
#  patchImage.registry.c_str()
#  patchImage.username.c_str()
#  patchImage.password.c_str()

if (($# < 3)); then
  LOG_INFO "Usage: $0 <image> <username> <password>"
  exit 255
fi

DockerImage="$1"
DockerUser="$2"
DockerPassword="$3"

echo "---------------------Environment---------------------------------"
echo "DockerImage:      "$DockerImage
echo "DockerUser:       "$DockerUser
echo "DockerPassword:   "$DockerPassword

# login
if ! docker login -u "${DockerUser}" -p "${DockerPassword}" "${DockerImage}"; then
  LOG_ERROR "docker login to ${DockerImage} failed!"
  exit 255
fi

echo "docker pull ${DockerImage}"

if ! docker pull ${DockerImage}; then
  echo "docker pull error"
  exit 255
fi

# not change
echo "docker pull succ:  ${DockerImage}"
