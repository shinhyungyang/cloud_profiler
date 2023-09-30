#!/usr/bin/env python3

import util
"""
sanity_check.py

It reads the count of tuples that are either emitted or not emitted from each
actor in Yahoo streaming benchmark topology.
"""

ACTORS = {
    'PartitionManager': [''],
    'DeserializeBolt': ['_EMITTED'],
    'EventFilterBolt': ['_EMITTED', '_NOTEMITTED'],
    'EventProjectionBolt': ['_EMITTED'],
    'RedisJoinBolt': ['_EMITTED', '_NOTEMITTED'],
    'CampaignProcessor': ['_EMITTED'],
}

TARGET_DIR='/home/shyang/GoogleDrive/shinhyungyang/logs/20180517_02/'

for actor,emitoptions in ACTORS.items():
    for emitoption in emitoptions:
        f_list = util.find('cloud_profiler*'+actor+'*SANITYCHECK'+emitoption+'*txt', TARGET_DIR)
        line_count_accum = 0
        for fpath in f_list:
            line_count = util.wc_l(fpath)
            line_count_accum += line_count
            print(fpath.replace(TARGET_DIR,'') + ': ' + str(line_count))
        print('TOTAL: ' + str(line_count_accum))

