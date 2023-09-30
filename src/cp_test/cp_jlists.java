import cp.swig.*;
import static cp.swig.cloud_profilerConstants.*;
// Below imports do not belong to the cloud_profiler.
import joptsimple.OptionParser;
import joptsimple.OptionSet;

public class cp_jlists {

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
          .describedAs( "number of iterations" ).defaultsTo ( 10000000 );
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

    timespec start    = new timespec();
    timespec end      = new timespec();
    timespec diff     = new timespec();
    timespec quotient = new timespec();

    LongVec longvec       = new LongVec();
    StringVec strvec      = new StringVec();
    StrPtrVec strptrvec   = new StrPtrVec();
    StrPtrVec strptrvec2  = new StrPtrVec();

    System.out.println("Adding Java primitive type int as C++ long");
    System.out.println("into std::vector<long>");

    // Create a long vector list in a tight loop
    cloud_profiler.getTS(start);
    for(int iter = 0; iter < (int)options.valueOf( "i" ); iter++) {
      longvec.add(iter);
    }
    cloud_profiler.getTS(end);

    // Evaluate and print results:
    {
      cloud_profiler.subtractTS(diff, start, end);
      cloud_profiler.divideTS(quotient, diff, (int)options.valueOf( "i" ));

      byte tmp1[] = new byte[255];
      byte tmp2[] = new byte[255];
      byte tmp3[] = new byte[255];
      byte tmp4[] = new byte[255];
      cloud_profiler.formatTS(start, tmp1, 255);
      System.out.println("Start:          " + new String(tmp1));
      cloud_profiler.formatTS(end, tmp2, 255);
      System.out.println("End:            " + new String(tmp2));
      String format = "%M:%S";
      cloud_profiler.formatTS(diff, tmp3, 255, format);
      System.out.println("Duration:       " + new String(tmp3) + " mm:ss.ns");
      System.out.println("Insertions:     " + (int)options.valueOf( "i" ));

      //quotient.setTv_nsec(0);
      //quotient.getTv_nsec();
      if (quotient.getTv_sec() > 0) {
        cloud_profiler.formatTS(quotient, tmp4, 255, format);
        System.out.println("ns / insertion: " + new String(tmp4) + " mm:ss.ns\n");
      } else {
        System.out.println("                " + quotient.getTv_nsec() + " ns per call.\n"); 
      }
    }

    strvec.add("Adding Java String as C++ std::string");
    strvec.add("into std::vector<std::string>");
    strvec.add("with default SWIG-generated wrappers");
    cloud_profiler.printStringVector(strvec);

    // Create a std::string vector list in a tight loop
    cloud_profiler.getTS(start);
    for(long iter = 0; iter < (int)options.valueOf( "i" ); iter++) {
      strvec.add(new String(""+iter));
    }
    cloud_profiler.getTS(end);

    {
      cloud_profiler.subtractTS(diff, start, end);
      cloud_profiler.divideTS(quotient, diff, (int)options.valueOf( "i" ));

      byte tmp1[] = new byte[255];
      byte tmp2[] = new byte[255];
      byte tmp3[] = new byte[255];
      byte tmp4[] = new byte[255];
      cloud_profiler.formatTS(start, tmp1, 255);
      System.out.println("Start:          " + new String(tmp1));
      cloud_profiler.formatTS(end, tmp2, 255);
      System.out.println("End:            " + new String(tmp2));
      String format = "%M:%S";
      cloud_profiler.formatTS(diff, tmp3, 255, format);
      System.out.println("Duration:       " + new String(tmp3) + " mm:ss.ns");
      System.out.println("Insertions:     " + (int)options.valueOf( "i" ));

      if (quotient.getTv_sec() > 0) {
        cloud_profiler.formatTS(quotient, tmp4, 255, format);
        System.out.println("ns / insertion: " + new String(tmp4) + " mm:ss.ns\n");
      } else {
        System.out.println("                " + quotient.getTv_nsec() + " ns per call.\n"); 
      }
    }

    cloud_profiler.addToStrPtrVec(strptrvec, "Adding Java String as C++ std::string *");
    cloud_profiler.addToStrPtrVec(strptrvec, "into std::vector<std::string *>");
    cloud_profiler.addToStrPtrVec(strptrvec, "with default SWIG-generated wrappers");
    cloud_profiler.printStrPtrVector(strptrvec);

    cloud_profiler.getTS(start);
    for(long iter = 0; iter < (int)options.valueOf( "i" ); iter++) {
      cloud_profiler.addToStrPtrVec(strptrvec, ""+iter);
    }
    cloud_profiler.getTS(end);

    {
      cloud_profiler.subtractTS(diff, start, end);
      cloud_profiler.divideTS(quotient, diff, (int)options.valueOf( "i" ));

      byte tmp1[] = new byte[255];
      byte tmp2[] = new byte[255];
      byte tmp3[] = new byte[255];
      byte tmp4[] = new byte[255];
      cloud_profiler.formatTS(start, tmp1, 255);
      System.out.println("Start:          " + new String(tmp1));
      cloud_profiler.formatTS(end, tmp2, 255);
      System.out.println("End:            " + new String(tmp2));
      String format = "%M:%S";
      cloud_profiler.formatTS(diff, tmp3, 255, format);
      System.out.println("Duration:       " + new String(tmp3) + " mm:ss.ns");
      System.out.println("Insertions:     " + (int)options.valueOf( "i" ));

      if (quotient.getTv_sec() > 0) {
        cloud_profiler.formatTS(quotient, tmp4, 255, format);
        System.out.println("ns / insertion: " + new String(tmp4) + " mm:ss.ns\n");
      } else {
        System.out.println("                " + quotient.getTv_nsec() + " ns per call.\n"); 
      }
    }

    cloud_profiler.deleteStrPtrVector(strptrvec);

    strptrvec2.add(cloud_profiler.newString("Adding Java String as C++ std::string *"));
    strptrvec2.add(cloud_profiler.newString("into std::vector<std::string *>"));
    strptrvec2.add(cloud_profiler.newString("with customized typemap for new strings"));
    cloud_profiler.printStrPtrVector(strptrvec2);

    cloud_profiler.getTS(start);
    for(long iter = 0; iter < (int)options.valueOf( "i" ); iter++) {
      strptrvec2.add(cloud_profiler.newString(""+iter));
    }
    cloud_profiler.getTS(end);

    {
      cloud_profiler.subtractTS(diff, start, end);
      cloud_profiler.divideTS(quotient, diff, (int)options.valueOf( "i" ));

      byte tmp1[] = new byte[255];
      byte tmp2[] = new byte[255];
      byte tmp3[] = new byte[255];
      byte tmp4[] = new byte[255];
      cloud_profiler.formatTS(start, tmp1, 255);
      System.out.println("Start:          " + new String(tmp1));
      cloud_profiler.formatTS(end, tmp2, 255);
      System.out.println("End:            " + new String(tmp2));
      String format = "%M:%S";
      cloud_profiler.formatTS(diff, tmp3, 255, format);
      System.out.println("Duration:       " + new String(tmp3) + " mm:ss.ns");
      System.out.println("Insertions:     " + (int)options.valueOf( "i" ));

      if (quotient.getTv_sec() > 0) {
        cloud_profiler.formatTS(quotient, tmp4, 255, format);
        System.out.println("ns / insertion: " + new String(tmp4) + " mm:ss.ns\n");
      } else {
        System.out.println("                " + quotient.getTv_nsec() + " ns per call.\n"); 
      }
    }

    cloud_profiler.deleteStrPtrVector(strptrvec2);
 }
}
