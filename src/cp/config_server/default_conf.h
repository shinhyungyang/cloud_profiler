#ifndef __DEFAULT_NET_CONF_H__
#define __DEFAULT_NET_CONF_H__

#include "cloud_profiler.h"
#include "net_conf.h"

#include <string>
#include <vector>

struct conf_table {
  std::string table_name;
  std::vector<conf_entry> entries;
  // conf_entry format:
  //   ch_name, ch_dotted_quad, ch_pid, ch_tid, ch_nr,
  //   handler-type, log_format, nr-of-parameters, values[]
};

conf_table test_0 = {
  "test_0",
  {
    { "identity          ", ".*", ".*", ".*", ".*", IDENTITY, ASCII, 0, {} },
    { "downsample       2", ".*", ".*", ".*", ".*", DOWNSAMPLE, ASCII, 1, {2} },
    { "downsample      10", ".*", ".*", ".*", ".*", DOWNSAMPLE, ASCII, 1, {10} },
    { "XoY 100      16384", ".*", ".*", ".*", ".*", XOY, ASCII, 0, {} },
    { "XoY 1000     32768", ".*", ".*", ".*", ".*", XOY, ASCII, 2, {1000, 32768} },
    { "XoY 2         1KiB", ".*", ".*", ".*", ".*", XOY, ASCII, 2, {2, 1024} },
    { "FirstLast      all", ".*", ".*", ".*", ".*", FIRSTLAST, ASCII, 0, {} },
    { "FirstLast        0", ".*", ".*", ".*", ".*", FIRSTLAST, ASCII, 0, {} },
    { "FirstLast        1", ".*", ".*", ".*", ".*", FIRSTLAST, ASCII, 0, {} },
    { "NULLH        logTS", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "NULLH      nologTS", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "FPC           10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { ".*", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} }
  }
};

conf_table test_1 = {
  "test_1",
  {
    { "identity          ", ".*", ".*", ".*", ".*", IDENTITY, ASCII, 0, {} },

    // Provide additional parameter, which will cause channel parameterization
    // to fail:
    { "downsample       2", ".*", ".*", ".*", ".*", DOWNSAMPLE, ASCII, 2, {2, 2} },

    { "downsample      10", ".*", ".*", ".*", ".*", DOWNSAMPLE, ASCII, 1, {10} },
    { "XoY 100      16384", ".*", ".*", ".*", ".*", XOY, ASCII, 0, {} },
    { "XoY 1000     32768", ".*", ".*", ".*", ".*", XOY, ASCII, 2, {1000, 32768} },
    { "XoY 2         1KiB", ".*", ".*", ".*", ".*", XOY, ASCII, 2, {2, 1024} },
    { "FirstLast      all", ".*", ".*", ".*", ".*", FIRSTLAST, ASCII, 0, {} },
    { "FirstLast        0", ".*", ".*", ".*", ".*", FIRSTLAST, ASCII, 0, {} },
    { "FirstLast        1", ".*", ".*", ".*", ".*", FIRSTLAST, ASCII, 0, {} },
    { ".*", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} }
  }
};


conf_table test_2 = {
  "test_2",
  {
    { "test_ch_1", "1.1.1.1", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "test_ch_1", "1.1.1.2", ".*", ".*", ".*", NULLH, ASCII, 1, {1} },
    { "test_ch_1", ".*", ".*", ".*", ".*", IDENTITY, ASCII, 1, {22} },
    { "test_ch_10", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 2, {22, 23} },
    { ".*", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} }
  }
};

conf_table test_3 = {
  "test_3",
  {
    { "test_ch_1", "1.1.1.1", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { ".*", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} }
  }
};

conf_table test_4 = {
  "test_4",
  {
    { "test_ch_1", "1.1.1.1", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
  }
};

// same as test0 except it uses BUFFER_IDENTITY
conf_table test_5 = {
  "test_5",
  {
    { "identity          ", ".*", ".*", ".*", ".*", IDENTITY, ASCII, 0, {} },
    { "downsample       2", ".*", ".*", ".*", ".*", DOWNSAMPLE, ASCII, 1, {2} },
    { "downsample      10", ".*", ".*", ".*", ".*", DOWNSAMPLE, ASCII, 1, {10} },
    { "XoY 100      16384", ".*", ".*", ".*", ".*", XOY, ASCII, 0, {} },
    { "XoY 1000     32768", ".*", ".*", ".*", ".*", XOY, ASCII, 2, {1000, 32768} },
    { "XoY 2         1KiB", ".*", ".*", ".*", ".*", XOY, ASCII, 2, {2, 1024} },
    { "FirstLast      all", ".*", ".*", ".*", ".*", FIRSTLAST, ASCII, 0, {} },
    { "FirstLast        0", ".*", ".*", ".*", ".*", FIRSTLAST, ASCII, 0, {} },
    { "FirstLast        1", ".*", ".*", ".*", ".*", FIRSTLAST, ASCII, 0, {} },
    { "buffer_identity   ", ".*", ".*", ".*", ".*", BUFFER_IDENTITY, BINARY, 0, {} },
    { ".*", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} }
  }
};

// configuration table for fox_test
conf_table fox_test = {
  "fox_test",
  {
    { "fox_start", ".*", ".*", ".*", ".*", FOX_START, ASCII, 0, {} },
    { "fox_end", ".*", ".*", ".*", ".*", FOX_END, ASCII, 5, {0,0,0,0,0} },
    { "ODROIDN2   GPIORTT", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "ODROIDN2   GPIORTT", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
  //{ "ODROIDN2   GPIORTT", "10.0.0.1", ".*", ".*", ".*", GPIORTT, ASCII, 3, {2, 1800000, 1} },
  //{ "ODROIDN2   GPIORTT", "10.0.0.2", ".*", ".*", ".*", GPIORTT, ASCII, 3, {2, 1800000, 0} },
    { "PartitionManager_NET_CONF", ".*", ".*", ".*", ".*", FOX_START, ASCII, 0, {} },
    { "CampaignProcessor_NET_CONF", ".*", ".*", ".*", ".*", FOX_END, ASCII, 5, {0,0,0,0,0} },
    { "fox_end", ".*", ".*", ".*", ".*", FOX_END, ASCII, 5, {0,0,0,0,0} },
    { "identity          ", ".*", ".*", ".*", ".*", IDENTITY, ASCII, 0, {} },
    { "downsample       2", ".*", ".*", ".*", ".*", DOWNSAMPLE, ASCII, 1, {2} },
    { "downsample      10", ".*", ".*", ".*", ".*", DOWNSAMPLE, ASCII, 1, {10} },
    { "XoY 100      16384", ".*", ".*", ".*", ".*", XOY, ASCII, 0, {} },
    { "XoY 1000     32768", ".*", ".*", ".*", ".*", XOY, ASCII, 2, {1000, 32768} },
    { "XoY 2         1KiB", ".*", ".*", ".*", ".*", XOY, ASCII, 2, {2, 1024} },
    { "FirstLast      all", ".*", ".*", ".*", ".*", FIRSTLAST, ASCII, 0, {} },
    { "FirstLast        0", ".*", ".*", ".*", ".*", FIRSTLAST, ASCII, 0, {} },
    { "FirstLast        1", ".*", ".*", ".*", ".*", FIRSTLAST, ASCII, 0, {} },
    { ".*", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
  }
};

conf_table streaming_benchmarks = {
  "streaming_benchmarks",
  {
    // Source actor
    { "KafkaSpout          FIRSTLAST", ".*", ".*", ".*", ".*", FIRSTLAST, ASCII, 0, {} },
    { "KafkaSpout                XOY", ".*", ".*", ".*", ".*", XOY, ASCII, 2, {1, 256} },
    { "KafkaSpout                FPC", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "KafkaSpout           IDENTITY", ".*", ".*", ".*", ".*", BUFFER_IDENTITY_COMP, ZSTD, 0, {} },
    { "KafkaSpout           FOXSTART", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "KafkaSpout            RESERVE", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "KafkaFetcher        FIRSTLAST", ".*", ".*", ".*", ".*", FIRSTLAST, ASCII, 0, {} },
    { "KafkaFetcher              XOY", ".*", ".*", ".*", ".*", XOY, ASCII, 2, {1, 256} },
    { "KafkaFetcher              FPC", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "KafkaFetcher         IDENTITY", ".*", ".*", ".*", ".*", BUFFER_IDENTITY_COMP, ZSTD, 0, {} },
    { "KafkaFetcher         FOXSTART", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "KafkaFetcher          RESERVE", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "KafkaRDDIterator    FIRSTLAST", ".*", ".*", ".*", ".*", FIRSTLAST, ASCII, 0, {} },
    { "KafkaRDDIterator          XOY", ".*", ".*", ".*", ".*", XOY, ASCII, 2, {1, 256} },
    { "KafkaRDDIterator          FPC", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "KafkaRDDIterator     IDENTITY", ".*", ".*", ".*", ".*", BUFFER_IDENTITY_COMP, ZSTD, 0, {} },
    { "KafkaRDDIterator     FOXSTART", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "KafkaRDDIterator      RESERVE", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },

    // Source actor for internal data generators
    { "DataGeneratorSpout  FIRSTLAST", ".*", ".*", ".*", ".*", FIRSTLAST, ASCII, 0, {} },
    { "DataGeneratorSpout        XOY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "DataGeneratorSpout   FPC_10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "DataGeneratorSpout   IDENTITY", ".*", ".*", ".*", ".*", BUFFER_IDENTITY_COMP, ZSTD, 0, {} },
    { "DataGeneratorSpout   ID_START", ".*", ".*", ".*", ".*", BUFFER_IDENTITY_COMP, ZSTD, 0, {} },
    { "DataGeneratorSpout     ID_END", ".*", ".*", ".*", ".*", BUFFER_IDENTITY_COMP, ZSTD, 0, {} },
  //{ "DataGeneratorSpout   FOXSTART", ".*", ".*", ".*", ".*", FOX_START, ASCII, 0, {} },
    { "DataGeneratorSpout   FOXSTART", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "DataGeneratorSpout    RESERVE", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    // Internal data generator
    { "DataGenerator        IDENTITY", ".*", ".*", ".*", ".*", BUFFER_IDENTITY_COMP, ZSTD, 0, {} },
    { "DataGenerator             XOY", ".*", ".*", ".*", ".*", XOY, ASCII, 2, {1, 256} },
    { "DataGenerator             FPC", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "DataGenerator        FPC_10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    // External data generator
    { "DGServer              OVERRUN", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "DGServer              BLOCKED", ".*", ".*", ".*", ".*", BUFFER_IDENTITY_COMP, ZSTD, 0, {} },
    { "DGServer           BLKFPC10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "DGServer             FPC_10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    // Empty Sink Source Actor
    { "DGClient             FPC_10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    // ZMQ Sink Source Actor
    { "ZMQSinkSpout         FPC_10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    // CPP ZMQ Server/Client
    { "CPPDGServer          FPC_10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "CPPDGServer        BLKFPC10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "CPPDGServer           OVERRUN", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "CPPDGClient          FPC_10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "CPPDGClient          IDENTITY", ".*", ".*", ".*", ".*", BUFFER_IDENTITY_COMP, ZSTD, 0, {} },
    { "CPPDGSink            FPC_10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "CPPDGSink            IDENTITY", ".*", ".*", ".*", ".*", BUFFER_IDENTITY_COMP, ZSTD, 0, {} },
    { "SwigCPPDGClient      FPC_10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "SwigCPPDGClient     FIRSTLAST", ".*", ".*", ".*", ".*", FIRSTLAST, ASCII, 0, {} },
    { "SwigCPPDGClient      FOXSTART", ".*", ".*", ".*", ".*", FOX_START, ASCII, 0, {} },
    { "SwigCPPDGClient     ZMQSTRLEN", ".*", ".*", ".*", ".*", BUFFER_IDENTITY_COMP, ZSTD, 0, {} },
    { "SwigCPPDGClient      IDENTITY", ".*", ".*", ".*", ".*", BUFFER_IDENTITY_COMP, ZSTD, 0, {} },
    { "SwigCPPDGSink        FPC_10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "SwigCPPDGServer      FPC_10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "SwigCPPDGServer    BLKFPC10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "SwigCPPDGServer       OVERRUN", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "SwigCPPDGServer      IDENTITY", ".*", ".*", ".*", ".*", BUFFER_IDENTITY_COMP, ZSTD, 0, {} },
    { "JavaClient           FPC_10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "JStrClient           FPC_10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "JStrClient           IDENTITY", ".*", ".*", ".*", ".*", BUFFER_IDENTITY_COMP, ZSTD, 0, {} },
    { "ConsumeThr           FPC_10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "ZVBucketOptThread    FPC_10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "ZVBucketOptThread    IDENTITY", ".*", ".*", ".*", ".*", BUFFER_IDENTITY_COMP, ZSTD, 0, {} },

    { "ZVBSinkOpt3         FIRSTLAST", ".*", ".*", ".*", ".*", FIRSTLAST, ASCII, 0, {} },
    { "ZVBSinkOpt3               XOY", ".*", ".*", ".*", ".*", XOY, ASCII, 2, {1, 256} },
    { "ZVBSinkOpt3          FPC_10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "ZVBSinkOpt3          IDENTITY", ".*", ".*", ".*", ".*", BUFFER_IDENTITY_COMP, ZSTD, 0, {} },
    { "ZVBSinkOpt3           RESERVE", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },

    // Intermediate actor for Spark
    { "Iterator00           IDENTITY", ".*", ".*", ".*", ".*", BUFFER_IDENTITY_COMP, ZSTD, 0, {} },
    { "Iterator00                XOY", ".*", ".*", ".*", ".*", XOY, ASCII, 2, {1, 256} },
    { "Iterator00                FPC", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },

    // DeserializeBolt
    { "DeserializeBolt      IDENTITY", ".*", ".*", ".*", ".*", BUFFER_IDENTITY_COMP, ZSTD, 0, {} },
    { "DeserializeBolt      ID_START", ".*", ".*", ".*", ".*", BUFFER_IDENTITY_COMP, ZSTD, 0, {} },
    { "DeserializeBolt        ID_END", ".*", ".*", ".*", ".*", BUFFER_IDENTITY_COMP, ZSTD, 0, {} },
    { "DeserializeBolt           XOY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "DeserializeBolt      FPC_10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "Iterator01           IDENTITY", ".*", ".*", ".*", ".*", BUFFER_IDENTITY_COMP, ZSTD, 0, {} },
    { "Iterator01                XOY", ".*", ".*", ".*", ".*", XOY, ASCII, 2, {1, 256} },
    { "Iterator01                FPC", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },

    // EventFilterBolt
    { "EventFilterBolt      IDENTITY", ".*", ".*", ".*", ".*", BUFFER_IDENTITY_COMP, ZSTD, 0, {} },
    { "EventFilterBolt      ID_START", ".*", ".*", ".*", ".*", BUFFER_IDENTITY_COMP, ZSTD, 0, {} },
    { "EventFilterBolt        ID_END", ".*", ".*", ".*", ".*", BUFFER_IDENTITY_COMP, ZSTD, 0, {} },
    { "EventFilterBolt           XOY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "EventFilterBolt      FPC_10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "Iterator02           IDENTITY", ".*", ".*", ".*", ".*", BUFFER_IDENTITY_COMP, ZSTD, 0, {} },
    { "Iterator02                XOY", ".*", ".*", ".*", ".*", XOY, ASCII, 2, {1, 256} },
    { "Iterator02                FPC", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },

    // EventProjectionBolt
    { "EventProjectionBolt  IDENTITY", ".*", ".*", ".*", ".*", BUFFER_IDENTITY_COMP, ZSTD, 0, {} },
    { "EventProjectionBolt  ID_START", ".*", ".*", ".*", ".*", BUFFER_IDENTITY_COMP, ZSTD, 0, {} },
    { "EventProjectionBolt    ID_END", ".*", ".*", ".*", ".*", BUFFER_IDENTITY_COMP, ZSTD, 0, {} },
    { "EventProjectionBolt       XOY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "EventProjectionBolt  FPC_10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "StreamProject        IDENTITY", ".*", ".*", ".*", ".*", BUFFER_IDENTITY_COMP, ZSTD, 0, {} },
    { "StreamProject             XOY", ".*", ".*", ".*", ".*", XOY, ASCII, 2, {1, 256} },
    { "StreamProject             FPC", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "Iterator03           IDENTITY", ".*", ".*", ".*", ".*", BUFFER_IDENTITY_COMP, ZSTD, 0, {} },
    { "Iterator03                XOY", ".*", ".*", ".*", ".*", XOY, ASCII, 2, {1, 256} },
    { "Iterator03                FPC", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },

    // For testing Spark w/o Redis I/O
    { "Iterator03_sink     FIRSTLAST", ".*", ".*", ".*", ".*", FIRSTLAST, ASCII, 0, {} },
    { "Iterator03_sink           XOY", ".*", ".*", ".*", ".*", XOY, ASCII, 2, {1, 256} },
    { "Iterator03_sink           FPC", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "Iterator03_sink      IDENTITY", ".*", ".*", ".*", ".*", BUFFER_IDENTITY_COMP, ZSTD, 0, {} },
    { "Iterator03_sink        FOXEND", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },

    // RedisJoinBolt
    { "RedisJoinBolt        IDENTITY", ".*", ".*", ".*", ".*", BUFFER_IDENTITY_COMP, ZSTD, 0, {} },
    { "RedisJoinBolt        ID_START", ".*", ".*", ".*", ".*", BUFFER_IDENTITY_COMP, ZSTD, 0, {} },
    { "RedisJoinBolt          ID_END", ".*", ".*", ".*", ".*", BUFFER_IDENTITY_COMP, ZSTD, 0, {} },
    { "RedisJoinBolt             XOY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "RedisJoinBolt        FPC_10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "Iterator04           IDENTITY", ".*", ".*", ".*", ".*", BUFFER_IDENTITY_COMP, ZSTD, 0, {} },
    { "Iterator04                XOY", ".*", ".*", ".*", ".*", XOY, ASCII, 2, {1, 256} },
    { "Iterator04                FPC", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },

    // CampaignProcessor
    { "CampaignProcessor    IDENTITY", ".*", ".*", ".*", ".*", BUFFER_IDENTITY_COMP, ZSTD, 0, {} },
    { "CampaignProcessor    ID_START", ".*", ".*", ".*", ".*", BUFFER_IDENTITY_COMP, ZSTD, 0, {} },
    { "CampaignProcessor      ID_END", ".*", ".*", ".*", ".*", BUFFER_IDENTITY_COMP, ZSTD, 0, {} },
    { "CampaignProcessor         XOY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "CampaignProcessor    FPC_10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "CampaignProcessor   FIRSTLAST", ".*", ".*", ".*", ".*", FIRSTLAST, ASCII, 0, {} },
  //{ "CampaignProcessor      FOXEND", ".*", ".*", ".*", ".*", FOX_END, ASCII, 5, {0,0,0,0,0} },
    { "CampaignProcessor      FOXEND", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "Iterator05           IDENTITY", ".*", ".*", ".*", ".*", BUFFER_IDENTITY_COMP, ZSTD, 0, {} },
    { "Iterator05                XOY", ".*", ".*", ".*", ".*", XOY, ASCII, 2, {1, 256} },
    { "Iterator05                FPC", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },

    { "Iterator06          FIRSTLAST", ".*", ".*", ".*", ".*", FIRSTLAST, ASCII, 0, {} },
    { "Iterator06                XOY", ".*", ".*", ".*", ".*", XOY, ASCII, 2, {1, 256} },
    { "Iterator06                FPC", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "Iterator06           IDENTITY", ".*", ".*", ".*", ".*", BUFFER_IDENTITY_COMP, ZSTD, 0, {} },
    { "Iterator06             FOXEND", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },

    { ".*", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} }
  }
};

conf_table streaming_benchmarks_noid = {
  "streaming_benchmarks_noid",
  {
    // Source actor
    { "KafkaSpout          FIRSTLAST", ".*", ".*", ".*", ".*", FIRSTLAST, ASCII, 0, {} },
    { "KafkaSpout                XOY", ".*", ".*", ".*", ".*", XOY, ASCII, 2, {1, 256} },
    { "KafkaSpout                FPC", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "KafkaSpout           IDENTITY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "KafkaSpout           FOXSTART", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "KafkaSpout            RESERVE", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "KafkaFetcher        FIRSTLAST", ".*", ".*", ".*", ".*", FIRSTLAST, ASCII, 0, {} },
    { "KafkaFetcher              XOY", ".*", ".*", ".*", ".*", XOY, ASCII, 2, {1, 256} },
    { "KafkaFetcher              FPC", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "KafkaFetcher         IDENTITY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "KafkaFetcher         FOXSTART", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "KafkaFetcher          RESERVE", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "KafkaRDDIterator    FIRSTLAST", ".*", ".*", ".*", ".*", FIRSTLAST, ASCII, 0, {} },
    { "KafkaRDDIterator          XOY", ".*", ".*", ".*", ".*", XOY, ASCII, 2, {1, 256} },
    { "KafkaRDDIterator          FPC", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "KafkaRDDIterator     IDENTITY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "KafkaRDDIterator     FOXSTART", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "KafkaRDDIterator      RESERVE", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },

    // Source actor for internal data generators
    { "DataGeneratorSpout  FIRSTLAST", ".*", ".*", ".*", ".*", FIRSTLAST, ASCII, 0, {} },
    { "DataGeneratorSpout        XOY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "DataGeneratorSpout   FPC_10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "DataGeneratorSpout   IDENTITY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "DataGeneratorSpout   ID_START", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "DataGeneratorSpout     ID_END", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
  //{ "DataGeneratorSpout   FOXSTART", ".*", ".*", ".*", ".*", FOX_START, ASCII, 0, {} },
    { "DataGeneratorSpout   FOXSTART", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "DataGeneratorSpout    RESERVE", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    // Internal data generator
    { "DataGenerator        IDENTITY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "DataGenerator             XOY", ".*", ".*", ".*", ".*", XOY, ASCII, 2, {1, 256} },
    { "DataGenerator             FPC", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "DataGenerator        FPC_10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    // External data generator
    { "DGServer              OVERRUN", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "DGServer              BLOCKED", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "DGServer           BLKFPC10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "DGServer             FPC_10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    // Empty Sink Source Actor
    { "DGClient             FPC_10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    // ZMQ Sink Source Actor
    { "ZMQSinkSpout         FPC_10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    // CPP ZMQ Server/Client
    { "CPPDGServer          FPC_10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "CPPDGServer        BLKFPC10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "CPPDGServer           OVERRUN", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "CPPDGClient          FPC_10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "CPPDGClient          IDENTITY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "CPPDGSink            FPC_10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "CPPDGSink            IDENTITY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "SwigCPPDGClient      FPC_10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "SwigCPPDGClient     FIRSTLAST", ".*", ".*", ".*", ".*", FIRSTLAST, ASCII, 0, {} },
    { "SwigCPPDGClient      FOXSTART", ".*", ".*", ".*", ".*", FOX_START, ASCII, 0, {} },
    { "SwigCPPDGClient     ZMQSTRLEN", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "SwigCPPDGClient      IDENTITY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "SwigCPPDGSink        FPC_10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "SwigCPPDGServer      FPC_10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "SwigCPPDGServer    BLKFPC10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "SwigCPPDGServer       OVERRUN", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "SwigCPPDGServer      IDENTITY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "JavaClient           FPC_10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "JStrClient           FPC_10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "JStrClient           IDENTITY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "ConsumeThr           FPC_10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "ZVBucketOptThread    FPC_10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "ZVBucketOptThread    IDENTITY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },

    { "ZVBSinkOpt3         FIRSTLAST", ".*", ".*", ".*", ".*", FIRSTLAST, ASCII, 0, {} },
    { "ZVBSinkOpt3               XOY", ".*", ".*", ".*", ".*", XOY, ASCII, 2, {1, 256} },
    { "ZVBSinkOpt3          FPC_10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "ZVBSinkOpt3          IDENTITY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "ZVBSinkOpt3           RESERVE", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },

    // Intermediate actor for Spark
    { "Iterator00           IDENTITY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "Iterator00                XOY", ".*", ".*", ".*", ".*", XOY, ASCII, 2, {1, 256} },
    { "Iterator00                FPC", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },

    // DeserializeBolt
    { "DeserializeBolt      IDENTITY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "DeserializeBolt      ID_START", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "DeserializeBolt        ID_END", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "DeserializeBolt           XOY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "DeserializeBolt      FPC_10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "Iterator01           IDENTITY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "Iterator01                XOY", ".*", ".*", ".*", ".*", XOY, ASCII, 2, {1, 256} },
    { "Iterator01                FPC", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },

    // EventFilterBolt
    { "EventFilterBolt      IDENTITY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "EventFilterBolt      ID_START", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "EventFilterBolt        ID_END", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "EventFilterBolt           XOY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "EventFilterBolt      FPC_10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "Iterator02           IDENTITY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "Iterator02                XOY", ".*", ".*", ".*", ".*", XOY, ASCII, 2, {1, 256} },
    { "Iterator02                FPC", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },

    // EventProjectionBolt
    { "EventProjectionBolt  IDENTITY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "EventProjectionBolt  ID_START", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "EventProjectionBolt    ID_END", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "EventProjectionBolt       XOY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "EventProjectionBolt  FPC_10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "StreamProject        IDENTITY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "StreamProject             XOY", ".*", ".*", ".*", ".*", XOY, ASCII, 2, {1, 256} },
    { "StreamProject             FPC", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "Iterator03           IDENTITY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "Iterator03                XOY", ".*", ".*", ".*", ".*", XOY, ASCII, 2, {1, 256} },
    { "Iterator03                FPC", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },

    // For testing Spark w/o Redis I/O
    { "Iterator03_sink     FIRSTLAST", ".*", ".*", ".*", ".*", FIRSTLAST, ASCII, 0, {} },
    { "Iterator03_sink           XOY", ".*", ".*", ".*", ".*", XOY, ASCII, 2, {1, 256} },
    { "Iterator03_sink           FPC", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "Iterator03_sink      IDENTITY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "Iterator03_sink        FOXEND", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },

    // RedisJoinBolt
    { "RedisJoinBolt        IDENTITY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "RedisJoinBolt        ID_START", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "RedisJoinBolt          ID_END", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "RedisJoinBolt             XOY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "RedisJoinBolt        FPC_10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "Iterator04           IDENTITY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "Iterator04                XOY", ".*", ".*", ".*", ".*", XOY, ASCII, 2, {1, 256} },
    { "Iterator04                FPC", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },

    // CampaignProcessor
    { "CampaignProcessor    IDENTITY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "CampaignProcessor    ID_START", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "CampaignProcessor      ID_END", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "CampaignProcessor         XOY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "CampaignProcessor    FPC_10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "CampaignProcessor   FIRSTLAST", ".*", ".*", ".*", ".*", FIRSTLAST, ASCII, 0, {} },
  //{ "CampaignProcessor      FOXEND", ".*", ".*", ".*", ".*", FOX_END, ASCII, 5, {0,0,0,0,0} },
    { "CampaignProcessor      FOXEND", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "Iterator05           IDENTITY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "Iterator05                XOY", ".*", ".*", ".*", ".*", XOY, ASCII, 2, {1, 256} },
    { "Iterator05                FPC", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },

    { "Iterator06          FIRSTLAST", ".*", ".*", ".*", ".*", FIRSTLAST, ASCII, 0, {} },
    { "Iterator06                XOY", ".*", ".*", ".*", ".*", XOY, ASCII, 2, {1, 256} },
    { "Iterator06                FPC", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "Iterator06           IDENTITY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "Iterator06             FOXEND", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },

    { ".*", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} }
  }
};

conf_table streaming_benchmarks_test = {
  "streaming_benchmarks_test",
  {
    // Source actor for internal data generators
    { "DataGeneratorSpout  FIRSTLAST", ".*", ".*", ".*", ".*", FIRSTLAST, ASCII, 0, {} },
    { "DataGeneratorSpout        XOY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "DataGeneratorSpout   FPC_10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "DataGeneratorSpout   IDENTITY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
  //{ "DataGeneratorSpout   FOXSTART", ".*", ".*", ".*", ".*", FOX_START, ASCII, 0, {} },
    { "DataGeneratorSpout   FOXSTART", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
  //{ "DataGeneratorSpout   FOXSTART", ".*", ".*", ".*", ".*", BUFFER_IDENTITY_COMP, ZSTD, 0, {} },
    { "DataGeneratorSpout    RESERVE", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    // External data generator
    { "DGServer              OVERRUN", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "DGServer              BLOCKED", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "DGServer           BLKFPC10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "DGServer             FPC_10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    // Empty Sink Source Actor
    { "DGClient             FPC_10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    // CPP ZMQ Server/Client
    { "CPPDGServer          FPC_10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "CPPDGServer        BLKFPC10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "CPPDGServer           OVERRUN", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "CPPDGClient          FPC_10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "CPPDGClient          IDENTITY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "CPPDGSink            FPC_10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "CPPDGSink            IDENTITY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "SwigCPPDGClient      FPC_10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "SwigCPPDGClient     FIRSTLAST", ".*", ".*", ".*", ".*", FIRSTLAST, ASCII, 0, {} },
    { "SwigCPPDGClient      FOXSTART", ".*", ".*", ".*", ".*", FOX_START, ASCII, 0, {} },
    { "SwigCPPDGClient     ZMQSTRLEN", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "SwigCPPDGClient      IDENTITY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "SwigCPPDGSink        FPC_10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "SwigCPPDGServer      FPC_10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "SwigCPPDGServer    BLKFPC10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "SwigCPPDGServer       OVERRUN", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "SwigCPPDGServer      IDENTITY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "ZVBucketOptThread    FPC_10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "ZVBucketOptThread    IDENTITY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    // DeserializeBolt
    { "DeserializeBolt      IDENTITY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },

    { "DeserializeBolt           XOY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "DeserializeBolt      FPC_10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
  //{ "DeserializeBolt       RESERVE", ".*", ".*", ".*", ".*", BUFFER_IDENTITY_COMP, ZSTD, 0, {} },
  //{ "DeserializeBolt       RESERVE", ".*", ".*", ".*", ".*", BUFFER_IDENTITY, BINARY_TSC, 0, {} },
    { "DeserializeBolt       RESERVE", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
  //{ "DeserializeBolt      RESERVE2", ".*", ".*", ".*", ".*", BUFFER_IDENTITY_COMP, ZSTD, 0, {} },
  //{ "DeserializeBolt      RESERVE2", ".*", ".*", ".*", ".*", BUFFER_IDENTITY, BINARY_TSC, 0, {} },
    { "DeserializeBolt      RESERVE2", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    // EventFilterBolt
    { "EventFilterBolt      IDENTITY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "EventFilterBolt           XOY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "EventFilterBolt      FPC_10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    // EventProjectionBolt
    { "EventProjectionBolt  IDENTITY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "EventProjectionBolt       XOY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "EventProjectionBolt  FPC_10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    // RedisJoinBolt
    { "RedisJoinBolt        IDENTITY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "RedisJoinBolt             XOY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "RedisJoinBolt        FPC_10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    // CampaignProcessor
    { "CampaignProcessor    IDENTITY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "CampaignProcessor         XOY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "CampaignProcessor    FPC_10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "CampaignProcessor   FIRSTLAST", ".*", ".*", ".*", ".*", FIRSTLAST, ASCII, 0, {} },
  //{ "CampaignProcessor      FOXEND", ".*", ".*", ".*", ".*", FOX_END, ASCII, 5, {0,0,0,0,0} },
  //{ "CampaignProcessor      FOXEND", ".*", ".*", ".*", ".*", BUFFER_IDENTITY, BINARY_TSC, 0, {} },
  //{ "CampaignProcessor      FOXEND", ".*", ".*", ".*", ".*", BUFFER_IDENTITY_COMP, ZSTD, 0, {} },
    { "CampaignProcessor      FOXEND", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
  //{ "CampaignProcessor     RESERVE", ".*", ".*", ".*", ".*", BUFFER_IDENTITY_COMP, ZSTD, 0, {} },
  //{ "CampaignProcessor     RESERVE", ".*", ".*", ".*", ".*", BUFFER_IDENTITY, BINARY_TSC, 0, {} },
    { "CampaignProcessor     RESERVE", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
  //{ "CampaignProcessor    RESERVE2", ".*", ".*", ".*", ".*", BUFFER_IDENTITY_COMP, ZSTD, 0, {} },
  //{ "CampaignProcessor    RESERVE2", ".*", ".*", ".*", ".*", BUFFER_IDENTITY, BINARY_TSC, 0, {} },
    { "CampaignProcessor    RESERVE2", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "Iterator05           IDENTITY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "Iterator05                XOY", ".*", ".*", ".*", ".*", XOY, ASCII, 2, {1, 256} },
    { "Iterator05                FPC", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
  ////
  //{ "DataGeneratorSpout   ID_START", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "DataGeneratorSpout   ID_START", ".*", ".*", ".*", ".*", BUFFER_IDENTITY_COMP, ZSTD, 0, {} },
  //{ "DataGeneratorSpout   ID_START", ".*", ".*", ".*", ".*", BUFFER_IDENTITY, BINARY_TSC, 0, {} },

  //{ "DataGeneratorSpout     ID_END", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "DataGeneratorSpout     ID_END", ".*", ".*", ".*", ".*", BUFFER_IDENTITY_COMP, ZSTD, 0, {} },
  //{ "DataGeneratorSpout     ID_END", ".*", ".*", ".*", ".*", BUFFER_IDENTITY, BINARY_TSC, 0, {} },

  //{ "DeserializeBolt      ID_START", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "DeserializeBolt      ID_START", ".*", ".*", ".*", ".*", BUFFER_IDENTITY_COMP, ZSTD, 0, {} },
  //{ "DeserializeBolt      ID_START", ".*", ".*", ".*", ".*", BUFFER_IDENTITY, BINARY_TSC, 0, {} },

  //{ "DeserializeBolt        ID_END", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "DeserializeBolt        ID_END", ".*", ".*", ".*", ".*", BUFFER_IDENTITY_COMP, ZSTD, 0, {} },
  //{ "DeserializeBolt        ID_END", ".*", ".*", ".*", ".*", BUFFER_IDENTITY, BINARY_TSC, 0, {} },
  //{ "DeserializeBolt        ID_END", ".*", ".*", ".*", ".*", BUFFER_IDENTITY, BINARY, 0, {} },
  //{ "DeserializeBolt        ID_END", ".*", ".*", ".*", ".*", IDENTITY, ASCII, 0, {} },

  //{ "EventFilterBolt      ID_START", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "EventFilterBolt      ID_START", ".*", ".*", ".*", ".*", BUFFER_IDENTITY_COMP, ZSTD, 0, {} },
  //{ "EventFilterBolt      ID_START", ".*", ".*", ".*", ".*", BUFFER_IDENTITY, BINARY_TSC, 0, {} },

  //{ "EventFilterBolt        ID_END", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "EventFilterBolt        ID_END", ".*", ".*", ".*", ".*", BUFFER_IDENTITY_COMP, ZSTD, 0, {} },
  //{ "EventFilterBolt        ID_END", ".*", ".*", ".*", ".*", BUFFER_IDENTITY, BINARY, 0, {} },

  //{ "EventProjectionBolt  ID_START", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "EventProjectionBolt  ID_START", ".*", ".*", ".*", ".*", BUFFER_IDENTITY_COMP, ZSTD, 0, {} },
  //{ "EventProjectionBolt  ID_START", ".*", ".*", ".*", ".*", BUFFER_IDENTITY, BINARY_TSC, 0, {} },

  //{ "EventProjectionBolt    ID_END", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "EventProjectionBolt    ID_END", ".*", ".*", ".*", ".*", BUFFER_IDENTITY_COMP, ZSTD, 0, {} },
  //{ "EventProjectionBolt    ID_END", ".*", ".*", ".*", ".*", BUFFER_IDENTITY, BINARY_TSC, 0, {} },

  //{ "RedisJoinBolt        ID_START", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "RedisJoinBolt        ID_START", ".*", ".*", ".*", ".*", BUFFER_IDENTITY_COMP, ZSTD, 0, {} },
  //{ "RedisJoinBolt        ID_START", ".*", ".*", ".*", ".*", BUFFER_IDENTITY, BINARY_TSC, 0, {} },

  //{ "RedisJoinBolt          ID_END", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "RedisJoinBolt          ID_END", ".*", ".*", ".*", ".*", BUFFER_IDENTITY_COMP, ZSTD, 0, {} },
  //{ "RedisJoinBolt          ID_END", ".*", ".*", ".*", ".*", BUFFER_IDENTITY, BINARY_TSC, 0, {} },

  //{ "CampaignProcessor    ID_START", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "CampaignProcessor    ID_START", ".*", ".*", ".*", ".*", BUFFER_IDENTITY_COMP, ZSTD, 0, {} },
  //{ "CampaignProcessor    ID_START", ".*", ".*", ".*", ".*", BUFFER_IDENTITY, BINARY_TSC, 0, {} },

  //{ "CampaignProcessor      ID_END", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "CampaignProcessor      ID_END", ".*", ".*", ".*", ".*", BUFFER_IDENTITY_COMP, ZSTD, 0, {} },
  //{ "CampaignProcessor      ID_END", ".*", ".*", ".*", ".*", BUFFER_IDENTITY, BINARY_TSC, 0, {} },

    { ".*", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} }
  }
};

conf_table streaming_benchmarks_nuh = {
  "streaming_benchmarks_nuh",
  {
    // Source actor
    { "KafkaSpout          FIRSTLAST", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "KafkaSpout                XOY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "KafkaSpout                FPC", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "KafkaSpout           IDENTITY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "KafkaSpout           FOXSTART", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "KafkaSpout            RESERVE", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "KafkaFetcher        FIRSTLAST", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "KafkaFetcher              XOY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "KafkaFetcher              FPC", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "KafkaFetcher         IDENTITY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "KafkaFetcher         FOXSTART", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "KafkaFetcher          RESERVE", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "KafkaRDDIterator    FIRSTLAST", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "KafkaRDDIterator          XOY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "KafkaRDDIterator          FPC", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "KafkaRDDIterator     IDENTITY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "KafkaRDDIterator     FOXSTART", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "KafkaRDDIterator      RESERVE", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },

    // Source actor for internal data generators
    { "DataGeneratorSpout  FIRSTLAST", ".*", ".*", ".*", ".*", FIRSTLAST, ASCII, 0, {} },
    { "DataGeneratorSpout        XOY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "DataGeneratorSpout   FPC_10MS", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "DataGeneratorSpout   IDENTITY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
  //{ "DataGeneratorSpout   FOXSTART", ".*", ".*", ".*", ".*", FOX_START, ASCII, 0, {} },
    { "DataGeneratorSpout   FOXSTART", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "DataGeneratorSpout    RESERVE", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    // Internal data generator
    { "DataGenerator        IDENTITY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "DataGenerator             XOY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "DataGenerator             FPC", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "DataGenerator        FPC_10MS", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    // External data generator
    { "DGServer              OVERRUN", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "DGServer              BLOCKED", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "DGServer           BLKFPC10MS", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "DGServer             FPC_10MS", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    // Empty Sink Source Actor
    { "DGClient             FPC_10MS", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    // ZMQ Sink Source Actor
    { "ZMQSinkSpout         FPC_10MS", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    // CPP ZMQ Server/Client
    { "CPPDGServer          FPC_10MS", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "CPPDGServer        BLKFPC10MS", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "CPPDGServer           OVERRUN", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "CPPDGClient          FPC_10MS", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "CPPDGSink            FPC_10MS", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "SwigCPPDGClient      FPC_10MS", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "SwigCPPDGClient     FIRSTLAST", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "SwigCPPDGClient      FOXSTART", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "SwigCPPDGClient     ZMQSTRLEN", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "SwigCPPDGSink        FPC_10MS", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "SwigCPPDGServer      FPC_10MS", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "SwigCPPDGServer    BLKFPC10MS", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "SwigCPPDGServer       OVERRUN", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "JavaClient           FPC_10MS", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "JStrClient           FPC_10MS", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "ConsumeThr           FPC_10MS", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },

    // Intermediate actor for Spark
    { "Iterator00           IDENTITY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "Iterator00                XOY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "Iterator00                FPC", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },

    // DeserializeBolt
    { "DeserializeBolt      IDENTITY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "DeserializeBolt           XOY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "DeserializeBolt      FPC_10MS", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "Iterator01           IDENTITY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "Iterator01                XOY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "Iterator01                FPC", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },

    // EventFilterBolt
    { "EventFilterBolt      IDENTITY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "EventFilterBolt           XOY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "EventFilterBolt      FPC_10MS", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "Iterator02           IDENTITY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "Iterator02                XOY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "Iterator02                FPC", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },

    // EventProjectionBolt
    { "EventProjectionBolt  IDENTITY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "EventProjectionBolt       XOY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "EventProjectionBolt  FPC_10MS", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "StreamProject        IDENTITY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "StreamProject             XOY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "StreamProject             FPC", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "Iterator03           IDENTITY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "Iterator03                XOY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "Iterator03                FPC", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },

    // For testing Spark w/o Redis I/O
    { "Iterator03_sink     FIRSTLAST", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "Iterator03_sink           XOY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "Iterator03_sink           FPC", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "Iterator03_sink      IDENTITY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "Iterator03_sink        FOXEND", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },

    // RedisJoinBolt
    { "RedisJoinBolt        IDENTITY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "RedisJoinBolt             XOY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "RedisJoinBolt        FPC_10MS", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "Iterator04           IDENTITY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "Iterator04                XOY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "Iterator04                FPC", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },

    // CampaignProcessor
    { "CampaignProcessor    IDENTITY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "CampaignProcessor         XOY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "CampaignProcessor    FPC_10MS", ".*", ".*", ".*", ".*", FASTPERIODICCOUNTER, ASCII, 0, {} },
    { "CampaignProcessor   FIRSTLAST", ".*", ".*", ".*", ".*", FIRSTLAST, ASCII, 0, {} },
    { "CampaignProcessor      FOXEND", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "Iterator05           IDENTITY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "Iterator05                XOY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "Iterator05                FPC", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },

    { "Iterator06          FIRSTLAST", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "Iterator06                XOY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "Iterator06                FPC", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "Iterator06           IDENTITY", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },
    { "Iterator06             FOXEND", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} },

    { ".*", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} }
  }
};

conf_table gpio_test = {
  "gpio_test",
  {
    { "ODROIDN2   GPIORTT", "10.0.0.1", ".*", ".*", ".*", GPIORTT, ASCII, 3, {2, 1800000, 1} },
    { "ODROIDN2   GPIORTT", "10.0.0.2", ".*", ".*", ".*", GPIORTT, ASCII, 3, {2, 1800000, 0} },
    { ".*", ".*", ".*", ".*", ".*", NULLH, ASCII, 0, {} }
  }
};

std::vector<conf_table> conf_tables = {
  // The first entry is the default table:
  test_0,
  test_1,
  test_2,
  test_3,
  test_4,
  test_5,
  fox_test,
  streaming_benchmarks,
  streaming_benchmarks_noid,
  streaming_benchmarks_test,
  streaming_benchmarks_nuh,
  gpio_test,
};

#endif
