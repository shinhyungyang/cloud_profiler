#!/usr/bin/python3

import argparse
from schedule import schedule_from_files

CMD_OPTIONS = [
  ['--measure_graph', 'FILE', 'measure graph file', 'meas_graph.csv'],
  ['--minrtt_log', 'FILE', 'min RTT log file', 'tsc_master_minrtt_log.csv'],
  ['--target_nodes', 'FILE', 'target node file', 'target_nodes.csv'],
]

def cmd_parser():
  parser = argparse.ArgumentParser(description='Run RTT scheduler')
  for opt in CMD_OPTIONS:
    parser.add_argument(opt[0], metavar=opt[1], help=opt[2], default=opt[3])

  return parser

if __name__ == '__main__':
  parser = cmd_parser()
  args = parser.parse_args()
  assignment = schedule_from_files(args)
