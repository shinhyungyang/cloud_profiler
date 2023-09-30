#!/usr/bin/python3
from util import *
from operator import itemgetter
import collections

meas_graph='meas_graph.csv'
target_nodes='target_nodes.csv'
minrtt_log='tsc_master_minrtt_log.csv'

# A key-value dictionary which stores an RTT measurement list for each node
# to all peers
# od = collections.OrderedDict(loadRTTRecord(minrtt_log));
od = loadRTTRecord(minrtt_log)

# For each node, sort RTT measurement list in ascending order
for k,v in od.items():
  v.sort(key=lambda x: x[1])

d = loadMeasGraph(meas_graph)
od_meas_graph = collections.OrderedDict(
  sorted(d.items(), key=
    lambda x: len(x[1]), reverse=True))

assignment = collections.OrderedDict()
for pred,succ in od_meas_graph.items():
  top_k = len(succ)
  od = collections.OrderedDict(
    sorted(od.items(), key=
      lambda x: sum([ v for k,v in x[1][0:top_k] ])))

  # Pop a node with its peers which has the minimal sum of top_k RTTs
  target = od.popitem(False)
  t_node = target[0]
  t_peers = [peer for peer,rtt in target[1][0:top_k]]

  # For each actor, assign a unique node
  assignment[pred] = t_node
  for i in range(0, top_k):
    assignment[succ[i]] = t_peers[i]

  # Delete assigned peers from the ordered dictionary
  for p in t_peers:
    if p in od:
      del od[p]

print (assignment)
nodes = loadTargetNodes(target_nodes)
print (nodes)

new_od = collections.OrderedDict();
for n in nodes:
  if n in od:
    new_od[n] = [e for e in od[n]]

for k,v in new_od.items():
  v[:] = [x for x in v if x[0] in nodes]

printKeyAndValues(new_od)
