#!/usr/bin/env bash

TMUX_CMD=;
TMUX_SESSION="gce";
HVMUSER="elclab_streaming";
ZONE="asia-east1-a";

HVMHOME="/home/${HVMUSER}"

tmux has-session -t ${TMUX_SESSION} 2>/dev/null

if [ $? -ne 0 ]
then
  tmux new-session -s ${TMUX_SESSION} -d
fi

#RUNNING_INSTANCES="$(gcloud compute instances list | grep RUNNING | grep "streaming-group-0-" | gawk '{print $1}')"
RUNNING_INSTANCES="`for ii in \`seq -f %02g 1 29;\`; do printf \"streaming-group-0-00%s \" \"${ii}\"; done`"


#for HVM in $(echo ${RUNNING_INSTANCES})
#do
#  HVM_CMD="mkdir -p ${HVMHOME}/git/cloud_profiler;"
#  TMUX_CMD="gcloud compute ssh ${HVMUSER}@${HVM} --command \"${HVM_CMD}\" --zone ${ZONE};"
#  tmpfile=`mktemp`
#  echo "${TMUX_CMD}" > ${tmpfile}
#  chmod 744 ${tmpfile}
#  tmux new-window -t ${TMUX_SESSION} "${tmpfile}; rm ${tmpfile};"
#done
#
#echo "Wait for 5 seconds"
#sleep 5

TARGET_FILE="${HOME}/Downloads/build_rel.tar.gz"
TARGET_FILE="${TARGET_FILE} ${HOME}/Downloads/src.tar.gz"
DEST_DIR="${HVMHOME}/git/cloud_profiler/"

hvmid="0"
for HVM in $(echo ${RUNNING_INSTANCES})
do
  gcloud compute scp ${TARGET_FILE} ${HVMUSER}@${HVM}:${DEST_DIR} --scp-flag="-pr" &
  pids[${hvmid}]=$!
  hvmid="$((${hvmid}+1))"
done

for pid in ${pids[*]}
do
  wait ${pid}
done

for HVM in $(echo ${RUNNING_INSTANCES})
do
  HVM_CMD="cd ${HVMHOME}/git/cloud_profiler;"
  HVM_CMD="${HVM_CMD} rm -rf build_rel/ src/;"
  HVM_CMD="${HVM_CMD} tar xf build_rel.tar.gz;"
  HVM_CMD="${HVM_CMD} tar xf src.tar.gz;"
  TMUX_CMD="gcloud compute ssh ${HVMUSER}@${HVM} --command \"${HVM_CMD}\" --zone ${ZONE};"
  tmpfile=`mktemp`
  echo "${TMUX_CMD}" > ${tmpfile}
  chmod 744 ${tmpfile}
  tmux new-window -t ${TMUX_SESSION} "${tmpfile}; rm ${tmpfile};"
done

tmux attach
