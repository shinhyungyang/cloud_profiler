#!/usr/bin/python3
import sys
import os
from util import *

# Retrieve file list from the commandline arguments
# fileList = sys.argv[1::]

sourceActors = ['/tmp/head.txt']
sinkActors = ['/tmp/tail.txt']

minTS = minTSFromFiles(sourceActors)
maxTS = maxTSFromFiles(sinkActors)
elapsedTS = subtractTS(minTS, maxTS) # startTS, endTS in an order
elapsedNanoseconds = float(elapsedTS[0]*NS_PER_S+elapsedTS[1])
numTuples = 20000000
#numTuples = sum(1 for line in open('/tmp/cloud_profiler:165.132.106.144:28380:28420:ch1:a:RandSpout.txt'))
print('Elapsed (ns):', elapsedNanoseconds)
print('Tuples:', numTuples)
print('Throughput:', numTuples/(elapsedNanoseconds/NS_PER_S))
