import sys
import time
import os
import fnmatch
from itertools import chain
import re

def fail(msg):
  print('ERROR: ' + msg)
  sys.exit(1)

def checkFile(f):
  if not os.path.isfile(f):
    fail('file ' + f + ' not found.')

def loadRTTRecord(f):
  """
  Opens file f and loads RTT records into a dictionary.
  The dictionary is returned to the caller.

  """
  checkFile(f)
  try:
    InFile = open(f, 'r')
  except IOERROR:
    fail('file ' + f + ' does not exist.')
  d = dict()
  ctr = 0
  line = InFile.readline()
  while (line):
    if line.startswith('#') == False:
      slices = [x.strip() for x in line.split(',')]
      slave_from = slices[0]
      slave_to = slices[1]
      rtt = int(slices[2])
      if slave_from in d:
        d[slave_from].append([slave_to, rtt])
      else:
        d[slave_from] = [[slave_to, rtt]]
    line = InFile.readline()
    ctr += 1
  InFile.close()
  return d

def printKeyAndValues(d):
  print ('len: ', len(d), sep='', end='\n')
  for k,v in d.items():
    print(k, end='\n')
    for item in v:
      print("  ", item, sep='', end='\n')

def loadMeasGraph(f):
  """
  Opens file f and loads the measurement graph into a dictionary.
  The dictionary is returned to the caller.

  """
  checkFile(f)
  try:
    InFile = open(f, 'r')
  except IOERROR:
    fail('file ' + f + ' does not exist.')
  d = dict()
  ctr = 0
  line = InFile.readline()
  while (line):
    if line.startswith('#') == False:
      slices = [x.strip().strip('[]') for x in line.split(',', 1)]
      d[slices[0]] = [x.strip() for x in slices[1].split(',')]
    line = InFile.readline()
    ctr += 1
  InFile.close()
  return d

def loadTargetNodes(f):
  """
  Opens file f and loads target nodes to do RTT-based assignment.
  A list of nodes are returned to the caller.

  """
  checkFile(f)
  try:
    InFile = open(f, 'r')
  except IOERROR:
    fail('file ' + f + ' does not exist.')
  l = []
  ctr = 0
  line = InFile.readline()
  while (line):
    if line.startswith('#') == False:
      slices = [x.strip() for x in line.split(',')]
      l.append(slices[0])
    line = InFile.readline()
    ctr += 1
  InFile.close()
  return l

