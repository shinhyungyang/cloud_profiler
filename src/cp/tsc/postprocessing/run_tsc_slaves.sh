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

RUNNING_INSTANCES="$(gcloud compute instances list | grep RUNNING | grep "streaming-group-0-" | grep -v "streaming-group-0-0010" | gawk '{print $1}')"
#RUNNING_INSTANCES="streaming-group-0-0011 streaming-group-0-0012 streaming-group-0-0013 streaming-group-0-0014 streaming-group-0-0015 streaming-group-0-0016 streaming-group-0-0017 streaming-group-0-0018 streaming-group-0-0019 streaming-group-0-0020"

#for HVM in $(echo ${RUNNING_INSTANCES})
#do
#  HVM_CMD="for i in \`seq 1 10\`; do ${HVMHOME}/git/cloud_profiler/build_rel/bin/tsc_slave --sv_ip ${TSC_MASTER_IP} & done"
#  TMUX_CMD="gcloud compute ssh ${HVMUSER}@${HVM} --command=\"${HVM_CMD}\" --zone=${ZONE};"
#  tmpfile=`mktemp`
#  echo "${TMUX_CMD}" > ${tmpfile}
#  chmod 744 ${tmpfile}
#  tmux new-window -t ${TMUX_SESSION} "${tmpfile} >> /tmp/gcloud_log.txt; rm ${tmpfile};"
#done

TSC_MASTER_IP="10.140.0.110"

TARGET_FILE=""
DEST_DIR="/tmp"

hvmid="0"
for HVM in $(echo ${RUNNING_INSTANCES})
do
  #HVM_CMD="for i in \`seq 1 10\`; do ${HVMHOME}/git/cloud_profiler/build_rel/bin/tsc_slave --sv_ip ${TSC_MASTER_IP} & done"
  HVM_CMD="${HVMHOME}/git/cloud_profiler/build_rel/bin/tsc_slave --sv_ip ${TSC_MASTER_IP}"
  tmpfile=`mktemp`
  echo '#!/usr/bin/env bash' > ${tmpfile}
  echo 'PATH=/opt/jvm/jdk1.8.0_74/bin:/opt/jvm/jdk1.8.0_74/jre/bin:/opt/maven/3.3.9/bin:/home/shyang/bin:/opt/rh/devtoolset-4/root/usr/bin:${PATH}' >> ${tmpfile}
  echo 'export PATH' >> ${tmpfile}
  echo 'export LD_LIBRARY_PATH=/opt/rh/devtoolset-4/root/usr/lib64:/opt/gcc/5.4.0/lib64:/opt/papi/5.4.1/lib:/opt/boost/1.61.0/lib:/opt/zeromq/4.1.4/lib' >> ${tmpfile}
  echo "${HVM_CMD}" >> ${tmpfile}
  chmod 744 ${tmpfile}

  TARGET_FILE="${tmpfile}"
  gcloud compute scp ${TARGET_FILE} ${HVMUSER}@${HVM}:${DEST_DIR} --scp-flag="-pr" &
  pids[${hvmid}]=$!
  bins[${hvmid}]=${tmpfile}
  hvmid="$((${hvmid}+1))"
done

for pid in ${pids[*]}
do
  wait ${pid}
done

iter="0"
for HVM in $(echo ${RUNNING_INSTANCES})
do
  HVM_CMD="${bins[${iter}]}"
  TMUX_CMD="gcloud compute ssh ${HVMUSER}@${HVM} --command=\"${HVM_CMD};\" --zone=${ZONE};"
  tmpfile=`mktemp`
  echo "${TMUX_CMD}" > ${tmpfile}
  chmod 744 ${tmpfile}
  tmux new-window -t ${TMUX_SESSION} "${tmpfile}; rm ${tmpfile};"

  iter="$((${iter}+1))"
done
