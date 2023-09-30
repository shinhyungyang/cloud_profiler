#!/usr/bin/python3
import sys
import os
from util import *

# Retrieve file list from the commandline arguments
# fileList = sys.argv[1::]
targetDir = None
fileList = sys.argv[1::]

if len(fileList) == 0:
  print('Usage: ./throughput_fpc.py "/dir/to/path"')
  sys.exit(1)

targetDir = fileList[0]
if os.path.isdir(targetDir) == False:
  fail(targetDir + 'is not a directory.')

fpcHandlerLogs = [root+'/'+filename for root,folders,files in os.walk(targetDir) for filename in files if 'FPC' in filename]
flHandlerLogs = [root+'/'+filename for root,folders,files in os.walk(targetDir) for filename in files if 'FIRSTLAST' in filename]

numTuples = cntTuplesFromFiles(fpcHandlerLogs)

minTS = minTSFromFiles(flHandlerLogs, False)
maxTS = maxTSFromFiles(flHandlerLogs, False)
elapsedTS = subtractTS(minTS, maxTS)
elapsedNanoseconds = float(elapsedTS[0]*NS_PER_S+elapsedTS[1])
#print('numTuples:',numTuples)
#print('Elapsed (seconds):', (elapsedNanoseconds/NS_PER_S))
print('Throughput:', numTuples/(elapsedNanoseconds/NS_PER_S))

