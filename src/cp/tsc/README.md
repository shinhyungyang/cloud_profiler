# Understanding current implementation of tsc_master and tsc_slave

## **tsc_master**

**tsc_master** coordinates RTT measurements of all participating **tsc_slave** pairs.

**tsc_master** accepts the following arguments:

```sh
./tsc_master --help
cloud_profiler tsc master
Usage:
  tsc_master [OPTION...]

      --cnf arg        configuration table name
  -c, --console        Write log-file output to console
  -p, --port arg       Port number
  -n, --nr_slaves arg  The number of slaves to subscribe to this tsc_master
  -h, --help           Usage information
```

## **tsc_slave**

One **tsc_slave** instance is necessary for each computing node.

**tsc_slave** accepts the following arguments:

```sh
./tsc_slave --help
cloud_profiler tsc slave
Usage:
  tsc_slave [OPTION...]

      --sv_ip arg  tsc master IP address
      --sv_p arg   tsc master port number
  -c, --console    Enagle log-file output to console
  -h, --help       Usage information
```

## Node-local configuration

### Running **tsc_master** and **tsc_slave**
In order to begin the measurement, a single **tsc_master** instance needs to 
be executed precedent to all **tsc_slave** instances.

```sh
# Shell instance #1
# Start tsc_master
./tsc_master -n 10
```

```sh
# Shell instance #2
# Start 10 tsc_slave instances
for i in {1..10}; do ./tsc_slave --sv_ip 127.0.0.1 & done
```

Note that **tsc_master** will accept 10 **tsc_slave** instances and the
measurment will begin only after required number of **tsc_slave** instances are
registered to the **tsc_master**.

### Measurement results

All log files are stored under /tmp:
```sh
ls /tmp/tsc*
/tmp/tsc_master:165.132.106.144:16030.txt
/tmp/tsc_master_minrtt_log.txt
/tmp/tsc_master_register_log.txt
/tmp/tsc_slave:165.132.106.144:16098.txt
/tmp/tsc_slave:165.132.106.144:16099.txt
/tmp/tsc_slave:165.132.106.144:16100.txt
/tmp/tsc_slave:165.132.106.144:16101.txt
/tmp/tsc_slave:165.132.106.144:16102.txt
/tmp/tsc_slave:165.132.106.144:16103.txt
/tmp/tsc_slave:165.132.106.144:16104.txt
/tmp/tsc_slave:165.132.106.144:16105.txt
/tmp/tsc_slave:165.132.106.144:16106.txt
/tmp/tsc_slave:165.132.106.144:16107.txt
```

Information of registered **tsc_slave** instances are logged in tsc_master_register_log.txt:
```sh
head /tmp/tsc_master_register_log.txt 
# slave_ip, slave_port, tsc_freq
165.132.106.144:9103,3312000000
165.132.106.144:9104,3312000000
165.132.106.144:9105,3312000000
165.132.106.144:9106,3312000000
165.132.106.144:9107,3312000000
165.132.106.144:9108,3312000000
165.132.106.144:9109,3312000000
165.132.106.144:9110,3312000000
165.132.106.144:9111,3312000000
```

Minimum RTT of all pairs of tsc_slave instances are logged in the tsc_mster_minrtt_log.txt:
```sh
head /tmp/tsc_master_minrtt_log.txt 
# slave_1_address (ip:port), slave_2_address (ip:port), min_rtt (ns)
165.132.106.144:9108,165.132.106.144:9111,26419
165.132.106.144:9109,165.132.106.144:9111,26519
165.132.106.144:9107,165.132.106.144:9103,26751
165.132.106.144:9110,165.132.106.144:9103,26762
165.132.106.144:9105,165.132.106.144:9110,26991
165.132.106.144:9105,165.132.106.144:9108,27017
165.132.106.144:9105,165.132.106.144:9107,27231
165.132.106.144:9108,165.132.106.144:9103,27244
165.132.106.144:9105,165.132.106.144:9106,27264
```

