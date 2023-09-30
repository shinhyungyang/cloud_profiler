package cp.java_agent;
import cp.swig.*;

import java.lang.management.GarbageCollectorMXBean;
import java.lang.management.ManagementFactory;
import java.util.List;

// C++ JavaAgentHandler class is extended in Java as JavaAgentHandler_CP
public class JavaAgentHandler_CP extends JavaAgentHandler {
  public JavaAgentHandler_CP() {
    Runtime.getRuntime().addShutdownHook(new Thread() {
      @Override
      public void run() {
        cloud_profiler.uninstallAgentHandler();
      }
    });
  }
  @Override
  public void callback() {
  }
  @Override
  public void null_callback() {
  }
  @Override
  public String gc_sampler() {
    final List<GarbageCollectorMXBean> gcBeans = ManagementFactory.getGarbageCollectorMXBeans();
    final int numberOfGCs = gcBeans.size();
    StringBuilder _builder = new StringBuilder();
    for (int i=0; i<numberOfGCs; ++i) {
      final GarbageCollectorMXBean _bean = gcBeans.get(i);
      _builder.append( _bean.getName() + ","
                     + _bean.getCollectionCount() + ","
                     + _bean.getCollectionTime() + ",");
    }
    return _builder.toString();
  }
  @Override
  public String get_jstring() {
    return "";
  }
  @Override
  public String get_tuple(long idx) {
    return "";
  }
  @Override
  protected void finalize() {
  }
}

