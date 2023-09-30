import cp.swig.*;
import static cp.swig.cloud_profilerConstants.*;
// Below imports do not belong to the cloud_profiler.
import joptsimple.OptionParser;
import joptsimple.OptionSet;


class LogThread extends Thread {

  private int tid, nr_iterations, ptest_duration;
  private boolean netcnf;
  private static volatile boolean terminate = false;
  
  LogThread(int tid, int nr_iterations, int ptest_duration, boolean netcnf) {
    this.tid= tid;
    this.nr_iterations = nr_iterations;
    this.ptest_duration = ptest_duration;
    this.netcnf = netcnf;
  }

  public void run() {
    //System.out.println("Thread " +  tid + " running...");
    if (tid  == -1) {
      run_TimerThread();
    } else {
      run_LogThread();
    }
    //System.out.println("Thread " +  tid + " exiting.");
  }

  public void run_LogThread() {
    long ch0, ch1, ch2, ch3, ch4, ch5, ch6, ch7, ch8, ch10;
    if (netcnf) {
      ch0 = cloud_profiler.openChannel("identity          ",
                                            log_format.ASCII,
                                            handler_type.NET_CONF);
      ch1 = cloud_profiler.openChannel("downsample       2",
                                            log_format.ASCII,
                                            handler_type.NET_CONF);
      ch2 = cloud_profiler.openChannel("downsample      10",
                                            log_format.ASCII,
                                            handler_type.NET_CONF);
      ch3 = cloud_profiler.openChannel("XoY 100      16384",
                                            log_format.ASCII,
                                            handler_type.NET_CONF);
      ch4 = cloud_profiler.openChannel("XoY 1000     32768",
                                            log_format.ASCII,
                                            handler_type.NET_CONF);
      ch5 = cloud_profiler.openChannel("XoY 2         1KiB",
                                            log_format.ASCII,
                                            handler_type.NET_CONF);
      ch6 = cloud_profiler.openChannel("FirstLast      all",
                                            log_format.ASCII,
                                            handler_type.NET_CONF);
      ch7 = cloud_profiler.openChannel("FirstLast        0",
                                            log_format.ASCII,
                                            handler_type.NET_CONF);
      ch8 = cloud_profiler.openChannel("FirstLast        1",
                                            log_format.ASCII,
                                            handler_type.NET_CONF);
      ch10 = cloud_profiler.openChannel("buffer_identity   ",
                                            log_format.ASCII,
                                            handler_type.NET_CONF);
      if (! (cloud_profiler.isOpen(ch0) && cloud_profiler.isOpen(ch1) &&
             cloud_profiler.isOpen(ch2) && cloud_profiler.isOpen(ch3) &&
             cloud_profiler.isOpen(ch4) && cloud_profiler.isOpen(ch5) &&
             cloud_profiler.isOpen(ch6) && cloud_profiler.isOpen(ch7) &&
             cloud_profiler.isOpen(ch8) && cloud_profiler.isOpen(ch10))) {
        System.out.println("Warning: some channel(s) failed to open.");
        System.out.println("Check the cloud_profiler log file.");
      }
    } else {
      ch0 = cloud_profiler.openChannel("identity          ",
                                            log_format.ASCII,
                                            handler_type.IDENTITY);
      ch1 = cloud_profiler.openChannel("downsample       2",
                                            log_format.ASCII,
                                            handler_type.DOWNSAMPLE);
      ch2 = cloud_profiler.openChannel("downsample      10",
                                            log_format.ASCII,
                                            handler_type.DOWNSAMPLE);
      ch3 = cloud_profiler.openChannel("XoY 100      16384",
                                            log_format.ASCII,
                                            handler_type.XOY);
      ch4 = cloud_profiler.openChannel("XoY 1000     32768",
                                            log_format.ASCII,
                                            handler_type.XOY);
      ch5 = cloud_profiler.openChannel("XoY 2         1KiB",
                                            log_format.ASCII,
                                            handler_type.XOY);
      ch6 = cloud_profiler.openChannel("FirstLast      all",
                                            log_format.ASCII,
                                            handler_type.FIRSTLAST);
      ch7 = cloud_profiler.openChannel("FirstLast        0",
                                            log_format.ASCII,
                                            handler_type.FIRSTLAST);
      ch8 = cloud_profiler.openChannel("FirstLast        1",
                                            log_format.ASCII,
                                            handler_type.FIRSTLAST);
      ch10 = cloud_profiler.openChannel("buffer_identity   ",
                                            log_format.ASCII,
                                            handler_type.BUFFER_IDENTITY);

      if (! (cloud_profiler.isOpen(ch0) && cloud_profiler.isOpen(ch1) &&
             cloud_profiler.isOpen(ch2) && cloud_profiler.isOpen(ch3) &&
             cloud_profiler.isOpen(ch4) && cloud_profiler.isOpen(ch5) &&
             cloud_profiler.isOpen(ch6) && cloud_profiler.isOpen(ch7) &&
             cloud_profiler.isOpen(ch8) && cloud_profiler.isOpen(ch10))) {
        System.out.println("Warning: some channel(s) failed to open.");
        System.out.println("Check the cloud_profiler log file.");
      }

      if (-1 == cloud_profiler.parameterizeChannel(ch1, 0, 2)) {
        System.out.println("Could not parameterize channel 1.");
        System.exit(1);
      }

      if (-1 == cloud_profiler.parameterizeChannel(ch2, 0, 10)) {
        System.out.println("Could not parameterize channel 2.");
        System.exit(1);
      }

      if (-1 == cloud_profiler.parameterizeChannel(ch4, 1, 32768)) {
        System.out.println("Could not parameterize channel 4, parm 1");
        System.exit(1);
      }

      if (-1 == cloud_profiler.parameterizeChannel(ch4, 0, 1000)) {
        System.out.println("Could not parameterize channel 4, parm 0");
        System.exit(1);
      }

      if (-1 == cloud_profiler.parameterizeChannel(ch5, 1, "1KiB")) {
        System.out.println("Could not parameterize channel 5, parm 1");
        System.exit(1);
      }

      if (-1 == cloud_profiler.parameterizeChannel(ch5, 0, 2)) {
        System.out.println("Could not parameterize channel 5, parm 0");
        System.exit(1);
      }

    } // netcnf


    for (long iter = 0; iter < nr_iterations; iter++) {
      cloud_profiler.logTS(ch0, iter); 
      cloud_profiler.logTS(ch1, iter); 
      cloud_profiler.logTS(ch2, iter); 
      cloud_profiler.logTS(ch3, iter); 
      cloud_profiler.logTS(ch4, iter); 
      cloud_profiler.logTS(ch5, iter); 
      cloud_profiler.logTS(ch6, iter); 
      cloud_profiler.logTS(ch10, iter);
    }
    cloud_profiler.logTS(ch8, 0); //Only 1 tuple ("start") logged. 

    // Closing channels via poison pill or explicitly:
    cloud_profiler.logTS(ch0, -1); 
    cloud_profiler.logTS(ch1, -1); 
    cloud_profiler.closeChannel(ch1); // Closing twice receives warning in Log.
    cloud_profiler.closeChannel(ch2);
    cloud_profiler.logTS(ch3, -1); 
    cloud_profiler.logTS(ch4, -1); 
    cloud_profiler.logTS(ch5, -1); 
    cloud_profiler.logTS(ch6, -1); 
    cloud_profiler.logTS(ch7, -1);  // No tuple logged.
    cloud_profiler.logTS(ch8, -1); 
    cloud_profiler.logTS(ch10, -1);

    // Test whether channels are closed:
    for (long iter = 0; iter < 100; iter++) {
      cloud_profiler.logTS(ch0, iter); 
      cloud_profiler.logTS(ch1, iter); 
      cloud_profiler.logTS(ch2, iter); 
      cloud_profiler.logTS(ch10, iter);
    }

    //System.out.println("Start of PeriodicCounter Test");

    //
    // PeriodicCounter test:
    //
    String name_str = "PeriodicCounter_" + tid;
    long ch9 = cloud_profiler.openChannel(name_str,
                                          log_format.ASCII,
                                          handler_type.FASTPERIODICCOUNTER);
    switch (tid) {
      case 1:
        // Thread 1:
        while (terminate == false) {
          try {
            Thread.sleep(300);
          } catch(InterruptedException e){}
          cloud_profiler.logTS(ch9, 0); 
        }
        break;
      case 2: {
          // Thread 2:
          //
          // Log 60 events @ 20 events/s
          // Sleep for 3 seconds
          // Repeat: log at 20 events/s 
          int round = 1;
          while (terminate == false) {
            try {
              Thread.sleep(50);
            } catch(InterruptedException e){}
            cloud_profiler.logTS(ch9, 0); 
            if (round == 60) {
              try {
                Thread.sleep(3000);
              } catch(InterruptedException e){}
            }
            round++;
          }
        } 
        break;
      case 3: {
          // Thread 3:
          //
          // Log 61 events at 20 events/s;
          // Set counter period to 500ms;
          // Repeat: sleep (1ms <= x <= 200ms) -> log;
          int round = 0;
          while (terminate == false && round <= 60) {
            try {
              Thread.sleep(50);
            } catch(InterruptedException e){}
            cloud_profiler.logTS(ch9, 0); 
            if (round == 60) {
              // Reset period to 500ms:
              if (-1 == cloud_profiler.parameterizeChannel(ch9, 0, (long)(NANO_S_PER_S / 2))) {
                System.out.println("Could not parameterize channel 9, parm 0");
                System.exit(1);
              }
            }
            round++;
          }
          while (terminate == false) {
            int sl = 1 + (int)(Math.random() * 200);
            //System.out.println(sl);
            try {
              Thread.sleep(sl);
            } catch(InterruptedException e){}
            cloud_profiler.logTS(ch9, 0); 
          }
      }
      break;

      //TBD: thread 4

      case 5: {
          // Thread 5:
          //
          // Log once and sleep;
          boolean logged_once = false;
          while (terminate == false) {
            if (!logged_once) {
              cloud_profiler.logTS(ch9, 0); 
              logged_once = true;
            }
          }
        }
        break;

      //TBD: several testcases from the cp_cpptest missing here.

      case 13: {
        // Thread 13:
        //
        // Count the total number of events, by setting the period
        // to 10 days.
        if (-1 == cloud_profiler.parameterizeChannel(ch9, 0, 10 * NANO_S_PER_DAY)) {
          System.out.println("Could not parameterize channel 9, parm 0");
          System.exit(1);
        }
        while (terminate == false) {
          cloud_profiler.logTS(ch9, 0); 
        }
      }
      break;
      default:
        // Thread 0 and others:
        while (terminate == false) {
          cloud_profiler.logTS(ch9, 0); 
        }
    }
    cloud_profiler.logTS(ch9, -1); // Poison pill

    //System.out.println("End of LogThread!");
  }
   
  public void run_TimerThread() {
    if (ptest_duration <= 0) {
      System.err.println("ERROR: Timer thread: invalid argument\n");
      System.exit(1);
    }
    // Announce the test:
    System.out.println("PeriodicCounter test for " + ptest_duration + " seconds:");
    // Spend time and print progress:
    for (int secs = 0; secs < ptest_duration; secs++) {
      System.out.print(".");
      try{
        Thread.sleep(1000);
      }catch(InterruptedException e){}
    }
    System.out.println("");
    // Announce we're done:
    terminate = true;
  }

}


public class cp_jtest {

  public static void loadCloudProfilerLibrary() {
    try {
      System.loadLibrary("cloud_profiler");
    }
    catch(UnsatisfiedLinkError e) {
      System.err.println("ERROR: \"cloud_profiler\" native code library failed to load.\n" + e);
      System.exit(1);
    }
  }

  public static void main(String argv[]) throws Exception {
    boolean netcnf = false; 
    OptionParser parser = new OptionParser() {
      {
        accepts( "h" );
        accepts( "n" );
        accepts( "d" ).withOptionalArg().ofType( Integer.class )
          .describedAs( "duration >= 5s of periodic counter test" ).defaultsTo ( 5 );
        accepts( "i" ).withOptionalArg().ofType( Integer.class )
          .describedAs( "number of iterations" ).defaultsTo ( 200000 );
        accepts( "t" ).withOptionalArg().ofType( Integer.class )
          .describedAs( "number of threads" ).defaultsTo( 1 );
      }
    };   
    OptionSet options = parser.parse(argv);
    if (options.has("h")) {
      parser.printHelpOn( System.out );
      System.exit(0);
    }

    if (options.has("n")) {
      netcnf = true;
    }

    if ((int)options.valueOf ( "d" ) < 5) {
      System.out.println("WARNING: testcases may exceed specified test duration d of " +
                          (int)options.valueOf ( "d" ) + " seconds!");
    }

    //
    // Load cloud_profiler library:
    // 
    loadCloudProfilerLibrary(); 

    // Run getTS() in tight loop:
    timespec start    = new timespec();
    timespec end      = new timespec();
    timespec diff     = new timespec();
    timespec quotient = new timespec();
    timespec ts       = new timespec();
    cloud_profiler.getTS(start);
    for(long iter = 0; iter < (int)options.valueOf( "i" ); iter++) {
      cloud_profiler.getTS(ts);
    }
    cloud_profiler.getTS(end);

    // Evaluate and print results:
    cloud_profiler.subtractTS(diff, start, end);
    cloud_profiler.divideTS(quotient, diff, (int)options.valueOf( "i" ));
    byte tmp1[] = new byte[255];
    byte tmp2[] = new byte[255];
    byte tmp3[] = new byte[255];
    byte tmp4[] = new byte[255];
    cloud_profiler.formatTS(start, tmp1, 255);
    System.out.println("Start:    " + new String(tmp1));
    cloud_profiler.formatTS(end, tmp2, 255);
    System.out.println("End:      " + new String(tmp2));
    String format = "%M:%S";
    cloud_profiler.formatTS(diff, tmp3, 255, format);
    System.out.println("Duration: " + new String(tmp3) + " mm:ss.ns");
    System.out.println("Calls:    " + (int)options.valueOf( "i" ));
    //quotient.setTv_nsec(0);
    //quotient.getTv_nsec();
    if (quotient.getTv_sec() > 0) {
      cloud_profiler.formatTS(quotient, tmp4, 255, format);
      System.out.println("ns / timestamp: " + new String(tmp4) + " mm:ss.ns");
    } else {
      System.out.println("          " + quotient.getTv_nsec() + " ns per call."); 
    }
    
    //
    // Log from several threads:
    //
    long ch0 = cloud_profiler.openChannel("foo bar 0",
                                          log_format.ASCII,
                                          handler_type.IDENTITY);
    cloud_profiler.logTS(ch0, 22);
    cloud_profiler.logTS(ch0, -1);

    int nr_threads = (int)options.valueOf( "t" );
    LogThread thrds[] = new LogThread[nr_threads + 1];    
    for (int thr = 0; thr < nr_threads; thr++) {
      thrds[thr] = new LogThread(thr,
                                 (int)options.valueOf( "i" ),
                                 (int)options.valueOf ( "d" ),
                                 netcnf);
      thrds[thr].start();
    }
    // Start the timer thread:
      thrds[nr_threads] = new LogThread(-1,
                                        (int)options.valueOf( "i" ),
                                        (int)options.valueOf ( "d" ),
                                        netcnf);
      thrds[nr_threads].start();
    for (int thr = 0; thr < nr_threads; thr++) {
      thrds[thr].join();
    }
    thrds[nr_threads].join();
 }
}
