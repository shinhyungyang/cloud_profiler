import cp.swig.*;
// Below imports do not belong to the cloud_profiler.
import joptsimple.OptionParser;
import joptsimple.OptionSet;

import java.time.Instant;
import java.lang.System;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.io.Writer;
import java.util.ArrayList;
import java.util.List;


public class cp_jkernels {

  public static void loadCloudProfilerLibrary() {
    try {
      System.loadLibrary("cloud_profiler");
    }
    catch(UnsatisfiedLinkError e) {
      System.err.println("ERROR: \"cloud_profiler\" native code library failed to load.\n" + e);
      System.exit(1);
    }
  }

  public static void printResults(
    String   msg,
    timespec start,
    timespec end,
    int      iterations)
  {
    timespec diff = new timespec();
    timespec quotient = new timespec();
    cloud_profiler.subtractTS(diff, start, end);
    cloud_profiler.divideTS(quotient, diff, iterations);
    byte tmp1[] = new byte[255];
    byte tmp2[] = new byte[255];
    byte tmp3[] = new byte[255];
    byte tmp4[] = new byte[255];
    cloud_profiler.formatTS(start, tmp1, 255);
    System.out.println(msg + ":");
    for (int c = 0; c <= msg.length(); c++) {
      System.out.print("=");
    }
    System.out.println("\nStart:    " + new String(tmp1));
    cloud_profiler.formatTS(end, tmp2, 255);
    System.out.println("End:      " + new String(tmp2));
    String format = "%M:%S";
    cloud_profiler.formatTS(diff, tmp3, 255, format);
    System.out.println("Duration: " + new String(tmp3) + " mm:ss.ns");
    System.out.println("Calls:    " + iterations);
    if (quotient.getTv_sec() > 0) {
      cloud_profiler.formatTS(quotient, tmp4, 255, format);
      System.out.println("ns / timestamp: " + new String(tmp4) + " mm:ss.ns");
    } else {
      System.out.println("          " + quotient.getTv_nsec() + " ns per call.");
    }
  }

  public static void main(String argv[]) throws Exception {
    OptionParser parser = new OptionParser() {
      {
        accepts( "h" );
        accepts( "i" ).withOptionalArg().ofType( Integer.class )
          .describedAs( "number of iterations" ).defaultsTo (300000);
        accepts( "t" ).withOptionalArg().ofType( Integer.class )
          .describedAs( "number of threads" ).defaultsTo( 1 );
      }
    };   
    OptionSet options = parser.parse(argv);
    if (options.has("h")) {
      parser.printHelpOn( System.out );
      System.exit(0);
    }

    //
    // Load cloud_profiler library:
    // 
    loadCloudProfilerLibrary(); 

    int nr_iters = (int)options.valueOf( "i" );
    timespec start = new timespec();
    timespec end   = new timespec();

for (int run = 0; run < 3; run++) {

    // Run getTS() in tight loop:
    timespec ts    = new timespec();
    cloud_profiler.getTS(start);
    for(long iter = 0; iter < nr_iters; iter++) {
      cloud_profiler.getTS(ts);
    }
    cloud_profiler.getTS(end);
    printResults("getTS", start, end, nr_iters);

    // Run Instant.now() in tight loop: 
    Instant time_ts;
    cloud_profiler.getTS(start);
    for(long iter = 0; iter < nr_iters; iter++) {
      time_ts = Instant.now();
    }
    cloud_profiler.getTS(end);
    printResults("Instant.now", start, end, nr_iters);

    // Run System.currentTimeMillis in tight loop: 
    long cur_time;
    cloud_profiler.getTS(start);
    for(long iter = 0; iter < nr_iters; iter++) {
      cur_time = System.currentTimeMillis();
    }
    cloud_profiler.getTS(end);
    printResults("System.currentTimeMillis", start, end, nr_iters);

    // Run System.nanoTime in tight loop: 
    long cur_time2;
    cloud_profiler.getTS(start);
    for(long iter = 0; iter < nr_iters; iter++) {
      cur_time2 = System.nanoTime();
    }
    cloud_profiler.getTS(end);
    printResults("System.nanoTime", start, end, nr_iters);


    // Run logTS ID handler in tight loop: 
    long ch_test = cloud_profiler.openChannel("ch_test_id",
                                              log_format.ASCII,
                                              handler_type.IDENTITY);
    cloud_profiler.getTS(start);
    for(long iter = 0; iter < nr_iters; iter++) {
      cloud_profiler.logTS(ch_test, iter);
    }
    cloud_profiler.getTS(end);
    printResults("logTS ID handler", start, end, nr_iters);

    // Run logTS null handler in tight loop: 
    long ch_test_n = cloud_profiler.openChannel("ch_test_null",
                                                log_format.ASCII,
                                                handler_type.NULLH);
    cloud_profiler.getTS(start);
    for(long iter = 0; iter < nr_iters; iter++) {
      cloud_profiler.logTS(ch_test_n, iter);
    }
    cloud_profiler.getTS(end);
    printResults("logTS NULL handler", start, end, nr_iters);

    // Run nologTS null handler in tight loop: 
    long ch_test_nl = cloud_profiler.openChannel("ch_test_nolog",
                                                 log_format.ASCII,
                                                 handler_type.NULLH);
    cloud_profiler.getTS(start);
    for(long iter = 0; iter < nr_iters; iter++) {
      cloud_profiler.nologTS(ch_test_nl, iter);
    }
    cloud_profiler.getTS(end);
    printResults("nologTS NULL handler", start, end, nr_iters);


    // Run Java file I/O in tight loop: 
    try {
      File file = File.createTempFile("/tmp/foo", ".txt");
      FileWriter writer = new FileWriter(file);
      BufferedWriter bufferedWriter = new BufferedWriter(writer, 8000000);
      cloud_profiler.getTS(start);
        String somestr = "foo\n";
      for(long iter = 0; iter < nr_iters; iter++) {
        cur_time2 = System.nanoTime();
        bufferedWriter.write(String.valueOf(iter)+", " + String.valueOf(cur_time2)+"\n");
        bufferedWriter.flush();
      }
      bufferedWriter.close();
      cloud_profiler.getTS(end);
      printResults("Java file I/O ", start, end, nr_iters);
    } catch(Exception e) {

    } finally {

    }
   

}




    //
    // Log from several threads:
    //
    /*
    long ch0 = cloud_profiler.openChannel("foo bar 0",
                                          log_format.ASCII,
                                          handler_type.IDENTITY);
    cloud_profiler.logTS(ch0, 22);
    cloud_profiler.logTS(ch0, -1);

    int nr_threads = (int)options.valueOf( "t" );
    LogThread thrds[] = new LogThread[nr_threads];    
    for (int thr = 0; thr < nr_threads; thr++) {
      thrds[thr] = new LogThread(thr, (int)options.valueOf( "i" ));
      thrds[thr].start();
    }
    for (int thr = 0; thr < nr_threads; thr++) {
      thrds[thr].join();
      System.out.println("Thread " + thr + " joined");
    }
    */
 }
}

class LogThread extends Thread {

  private int ID, nr_iterations;
  
  LogThread(int ID, int nr_iterations) {
    this.ID = ID;
    this.nr_iterations = nr_iterations;
  }
   
  public void run() {
    System.out.println("Thread " +  ID + " running...");
    long ch0 = cloud_profiler.openChannel("identity      ",
                                          log_format.ASCII,
                                          handler_type.IDENTITY);
    long ch1 = cloud_profiler.openChannel("downsample   2",
                                          log_format.ASCII,
                                          handler_type.DOWNSAMPLE);
    long ch2 = cloud_profiler.openChannel("downsample  10",
                                          log_format.ASCII,
                                          handler_type.DOWNSAMPLE);
    long ch3 = cloud_profiler.openChannel("XoY 100  16384",
                                          log_format.ASCII,
                                          handler_type.XOY);
    long ch4 = cloud_profiler.openChannel("XoY 1000 32768",
                                          log_format.ASCII,
                                          handler_type.XOY);
    long ch5 = cloud_profiler.openChannel("XoY 2     1KiB",
                                          log_format.ASCII,
                                          handler_type.XOY);

    if(-1 == cloud_profiler.parameterizeChannel(ch1, 0, 2)) {
      System.out.println("Could not parameterize channel 1.");
      System.exit(1);
    }

    if(-1 == cloud_profiler.parameterizeChannel(ch2, 0, 10)) {
      System.out.println("Could not parameterize channel 2.");
      System.exit(1);
    }

    if(-1 == cloud_profiler.parameterizeChannel(ch4, 1, 32768)) {
      System.out.println("Could not parameterize channel 4, parm 1");
      System.exit(1);
    }

    if(-1 == cloud_profiler.parameterizeChannel(ch4, 0, 1000)) {
      System.out.println("Could not parameterize channel 4, parm 0");
      System.exit(1);
    }

    if(-1 == cloud_profiler.parameterizeChannel(ch5, 1, "1KiB")) {
      System.out.println("Could not parameterize channel 5, parm 1");
      System.exit(1);
    }

    if(-1 == cloud_profiler.parameterizeChannel(ch5, 0, 2)) {
      System.out.println("Could not parameterize channel 5, parm 0");
      System.exit(1);
    }



    for (long iter = 0; iter < nr_iterations; iter++) {
      cloud_profiler.logTS(ch0, iter); 
      cloud_profiler.logTS(ch1, iter); 
      cloud_profiler.logTS(ch2, iter); 
      cloud_profiler.logTS(ch3, iter); 
      cloud_profiler.logTS(ch4, iter); 
      cloud_profiler.logTS(ch5, iter); 
    }

    // Poison pill to close channel:
    cloud_profiler.logTS(ch0, -1); 
    cloud_profiler.logTS(ch1, -1); 
    cloud_profiler.logTS(ch2, -1); 
    cloud_profiler.logTS(ch3, -1); 
    cloud_profiler.logTS(ch4, -1); 
    cloud_profiler.logTS(ch5, -1); 
 
    System.out.println("Thread " +  ID + " exiting.");
   }
}


