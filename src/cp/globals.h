#ifndef __GLOBALS_H__
#define __GLOBALS_H__ // prevent against double-inclusion

#define CACHELINE_SIZE  64
#define MAX_ALLOC_NR_THREADS 4096
#define CONSOLE_LOG     false // Status log to console true/false
const char path_to_output[] = "/tmp";

#define S_PER_MIN    (60)
#define S_PER_HOUR   (60 * S_PER_MIN)
#define S_PER_DAY    (24 * S_PER_HOUR)

#define MILLI_S_PER_S   (1000UL)
#define MICRO_S_PER_S   (1000UL * MILLI_S_PER_S)
#define NANO_S_PER_S    (1000UL * MICRO_S_PER_S)
#define NANO_S_PER_MIN  (S_PER_MIN  * NANO_S_PER_S)
#define NANO_S_PER_HOUR (S_PER_HOUR * NANO_S_PER_S)
#define NANO_S_PER_DAY  (S_PER_DAY  * NANO_S_PER_S)

#define MAX_STR_SIZE (128)
#define MAX_IPV4_STR_SIZE (15)

const char conf_server_default_ip[] = "cp.elc.cs.yonsei.ac.kr";
const int  conf_server_default_port = 9100;
const char conf_server_max_param    = 7; // Max #parameters per conf request, reply

const char tsc_master_default_ip[] = "165.132.106.144";
const int  tsc_master_default_port = 9101;

const int  tsc_rtt_meas_default_port = 9102;

const int  tsc_command_default_port = 9103;

const int  fox_start_default_port = 9104;
// 9105 is used by DGFetcher
// 9106~9110 are used by DGServer

const int  cpu_util_spare = 50;
const int  cpu_util_chk_period = 1;

// 9105 is used by DGFetcher
// 9106~9110 are used by DGServer

#endif
