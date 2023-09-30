import cp.swig.*;
import cp.java_agent.*;
import static cp.swig.cloud_profilerConstants.*;
// Below imports do not belong to the cloud_profiler.
import joptsimple.OptionParser;
import joptsimple.OptionSet;


public class cp_jstring extends JavaAgentHandler_CP {

  public static void loadCloudProfilerLibrary() {
    try {
      System.loadLibrary("cloud_profiler");
    }
    catch(UnsatisfiedLinkError e) {
      System.err.println("ERROR: \"cloud_profiler\" native code library failed to load.\n" + e);
      System.exit(1);
    }
  }

  public static final String sample_campaign_1 = "{\"user_id\": \"06461f9b-ed9b-45f6-b944-76224da5cd95\", \"page_id\": \"d890b51d-46ad-49db-95ff-fae3255712d9\", \"ad_id\": \"b9f90277-6c53-4b45-a329-fcd2338f11e7\", \"ad_type\": \"mail\", \"event_type\": \"view\", \"event_time\": \"1534761527997\", \"tuple_idx\": \"1\", \"ip_address\": \"10.140.0.121\"}";

  @Override
  public String get_jstring() {
    return sample_campaign_1;
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
        accepts( "s" ).withOptionalArg().ofType( Boolean.class )
          .describedAs( "run as a server").defaultsTo( true );
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

    cp_jstring mainClass = new cp_jstring();


    StringVec strvec = new StringVec();
    long tcp_ch;
    if ((Boolean)options.valueOf ( "s" ) == false) {
      strvec.add("127.0.0.1:9101");
      tcp_ch = cloud_profiler.openTCPChannel(false, strvec, 1);
    } else {
      strvec.add("tcp://*:9101");
      tcp_ch = cloud_profiler.openTCPChannel(true, strvec, 1);
    }

    // Run getTS() in tight loop:
    timespec start    = new timespec();
    timespec end      = new timespec();
    timespec diff     = new timespec();
    timespec quotient = new timespec();
    timespec ts       = new timespec();
    cloud_profiler.getTS(start);

    if ((Boolean)options.valueOf ( "s" ) == false) {
      for(long iter = 0; iter < (int)options.valueOf( "i" ); iter++) {
        cloud_profiler.recvStr(tcp_ch);
      //cloud_profiler.getTS(end);
      }
    } else {
      cloud_profiler.emit(mainClass, tcp_ch, 35, 35000, 0, 1000000000, 1000000);
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

    // Close open channels
    cloud_profiler.closeTCPChannel(tcp_ch);
 }
}
