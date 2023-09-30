
import java.lang.instrument.Instrumentation;


import java.net.InetAddress;
import java.io.File;
import java.io.FileOutputStream;
import java.io.PrintWriter;

import cp.swig.*;
import cp.java_agent.JavaAgentHandler_CP;

public class jagent 
{
  public static JavaAgentHandler_CP _handler;
  static {
    try {
      System.load("/usr/lib/libcloud_profiler.so");
    }
    catch(UnsatisfiedLinkError e) {
      System.err.println("\"cloud_profiler\" native code library failed to load.\n" + e);
      e.printStackTrace();
      System.exit(1);
    }
  }

  private static Instrumentation s_instrumentation = null;

  private static long _channelHandler = 0L;

  public static void premain(String options, Instrumentation instrumentation) {
    // Handle duplicate agents
    if (s_instrumentation != null) {
        return;
    }
    s_instrumentation = instrumentation;
    JavaAgentHandler_CP jagent_obj = new JavaAgentHandler_CP();
    _handler = jagent_obj;
    _channelHandler = cloud_profiler.installAgentHandler(jagent_obj, 500);
    // System.out.println("Channel Opened: with closure " + _channelHandler);
  }
}

