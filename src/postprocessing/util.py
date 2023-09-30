#!/usr/bin/python3
import sys
import time
import os
import fnmatch
from itertools import chain

NS_PER_S = 1000000000

def fail(msg):
  print('ERROR: ' + msg)
  sys.exit(1)

def checkFile(f):
  if not os.path.isfile(f):
    fail('file ' + f + ' not found.')

def loadTS(f):
  """
  Opens file f and loads all tuples into a dictionary.
  The dictionary is returned to the caller.
  Tuple IDs are the keys into the dictionary. 
  A list of timestamps represented as int objects in ns resolution is the
  value type of the dictionary. Thus each tuple can occur multiple times in f.

  """
  checkFile(f)
  try:
    InFile = open(f, 'r')
  except IOERROR:
    fail('file ' + f + ' does not exist.') 
  d = dict()
  ctr = 0
  line = InFile.readline()
  while(line):
    #ctr += 1
    #if ctr % 10000 == 0:
    #  print(ctr) 
    slices = line.split(',')
    ts = [int(slices[1]), int(slices[2])]
    tuple_id = int(slices[0])
    if tuple_id in d:
      d[tuple_id].append(ts)
    else:
      d[tuple_id] = [ts] 
    line = InFile.readline()
  InFile.close()
  return d

def loadTS_FPC(f):
  """
  Opens an FPC log file f and loads all logging occurrences into a dictionary.
  The dictionary is returned to the caller.
  Counts of logging occurrence are the keys into the dictionary. 
  A list of timestamps represented as int objects in ns resolution is the
  value type of the dictionary. Thus each tuple can occur multiple times in f.

  """
  checkFile(f)
  try:
    InFile = open(f, 'r')
  except IOERROR:
    fail('file ' + f + ' does not exist.') 
  d = dict()
  ctr = 0
  line = InFile.readline()
  while(line):
    if line.startswith('#') == False:
      if ctr == 0:
        ctr += 1  # Do not include the first timestamp (the first non ^#)
        continue
      slices = line.split(',')
      cnt = int(slices[1])
      ts = [int(slices[2].strip().lstrip("[")), int(slices[3]), cnt]
      d[ctr] = [ts]
      ctr += 1
    line = InFile.readline()
  InFile.close()
  return d

def subtractTS(start, end):
  """
  Subtract two timestamps given in format [tv_sec, tv_nsec].
  """
  if end[1] - start[1] < 0:
    tv_sec  = end[0] - start[0] - 1
    tv_nsec = end[1] - start[1] + NS_PER_S
  else:
    tv_sec  = end[0] - start[0]
    tv_nsec = end[1] - start[1]
  return [tv_sec, tv_nsec] 


def subtractTSDict(d1, d2):
  """
  Subtract the time-stamps in dictionary d2 from the corresponding time-stamps
  in d1.  Return a dictionary with the differences. Time-stamps from d2 which do
  not have a counterpart in d1 will be ignored. 

  FIXME: for now, this function assumes that each tuple id has only one
         time-stamp in the dictionary.
  """
  assert isinstance(d1, dict)
  assert isinstance(d2, dict)
  ret = dict()
  for k, v in sorted(d2.items()):
    if k in d1:
      # Ok, tuple has a counter-part in d1:
      ret[k] = [subtractTS(v[0], d1[k][0])] 
  return ret


def dumpTSDict(d, f):
  assert isinstance(d, dict)
  try:
    OutFile = open(f, 'w')
  except IOERROR:
    fail('cannot open file ' + f + ' for writing.')
  for k, v in sorted(d.items()):
    OutFile.write(str(k))
    for ts in v:
      OutFile.write(', ' + str(ts[0]) + ', ' + str(ts[1]) + '\n')
  OutFile.close()


def printplus(obj):
  """
  Pretty-prints the object passed in.

  """
  # Dict
  if isinstance(obj, dict):
      for k, v in sorted(obj.items()):
        print(k, end='')
        for ts in v:
          print(',', ts[0], end=', ')
          print(ts[1])
  # List or tuple            
  elif isinstance(obj, list) or isinstance(obj, tuple):
    for x in obj:
      print(x)

  # Other
    else:
      print(obj)

def _minMaxTSinFiles(l, r, isFPC=False):
  """
  Get minimum or maximum TS from all TS in multiple cloud_profiler log files
  in list l. When r is False minimum TS is returned, otherwise maximum TS is
  returned.
  """
  [checkFile(f) for f in l]
  mins = []
  for f in l:
    if isFPC == False:
      d = loadTS(f)
      # get list of values in d and unlist each item in the list to sort TS with
      # tv_sec as the primary key and tv_nsec as the secondary key
    else:
      d = loadTS_FPC(f)

    for ts in sorted(list(chain.from_iterable(d.values())), key = lambda x: (x[0], x[1]), reverse=r):
      mins.append(ts)
      break
  if isFPC == False:
      for s, ns in sorted(mins, key = lambda x: (x[0], x[1]), reverse=r):
       #print('sorted:',mins)
        tv_sec = s
        tv_nsec = ns
        break
  else:
      for s, ns, cnt in sorted(mins, key = lambda x: (x[0], x[1]), reverse=r):
       #print('sorted:',mins)
        tv_sec = s
        tv_nsec = ns
        break
 #print(tv_sec,tv_nsec)
  return [tv_sec, tv_nsec]

def minTSFromFiles(l, isFPC=False):
  """
  Get the minimum TS from multiple cloud_profiler log files in list l
  """
  return _minMaxTSinFiles(l, False, isFPC)

def maxTSFromFiles(l, isFPC=False):
  """
  Get the maximum TS from multiple cloud_profiler log files in list l
  """
  return _minMaxTSinFiles(l, True, isFPC)

def cntTuplesFromFiles(l):
  """
  Get the total tuple counts from multiple FPC log files in list l
  """
  [checkFile(f) for f in l]
  mins = []
  count = 0
  for f in l:
    d = loadTS_FPC(f)
    for x in d.values():
      count += x[0][2]
  return count

def find(pattern, path):
    result = []
    for root, dirs, files in os.walk(path):
        for name in files:
            if fnmatch.fnmatch(name, pattern):
                result.append(os.path.join(root, name))
    return result

def wc_l(fname):
    i = 0
    with open(fname) as f:
        for i, l in enumerate(f, 1):
            pass
    return i

if __name__ == '__main__':
  d1 = loadTS('/tmp/t.txt')
  sub_d = subtractTSDict(d1, d1)
  dumpTSDict(sub_d, 'foo.txt')
