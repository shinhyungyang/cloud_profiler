package cp.java;

import java.util.Arrays;
import java.util.ArrayList;
import cp.swig.*;
import org.jctools.queues.SpscArrayQueue;
import org.apache.storm.tuple.Values;

public class JStrFactory {
  private ArrayList<Thread> thrs;                          // Thread
  private ArrayList<SpscArrayQueue<String>> s_queues;      // String queue
  private ArrayList<SpscArrayQueue<String[]>> sarr_queues; // String array queue
  private ArrayList<SpscArrayQueue<Values[]>> varr_queues; // Values array queue
  private ArrayList<SpscArrayQueue<Integer>>  alen_queues; // array-length queue
  private int NR_SOCKETS;
  private long MAX_ITERS;
  private int tuningKnob;
  private int queueIdx = 0; // only use in fetchJStr()

  private ArrayList<Long> arr_ch;  // temporary array for storing channel ids


  public JStrFactory() {
    CloudProfiler.init();
  }

  public void init(int nr_sockets, String [] addresses) {

    assert nr_sockets == addresses.length;

    NR_SOCKETS = nr_sockets;
    thrs = new ArrayList<>(NR_SOCKETS);
    s_queues = new ArrayList<>(NR_SOCKETS);

    for (int i = 0; i < NR_SOCKETS; ++i) {
      SpscArrayQueue<String> q = new SpscArrayQueue<>(Globals.QUEUE_SIZE);
      s_queues.add(q);
      thrs.add(new JStrThread(cloud_profiler.openZMQChannel(addresses[i]), q));
    }
  }

  public void init(int nr_sockets, long max_iter) {

    NR_SOCKETS = nr_sockets;
    MAX_ITERS = max_iter;
    thrs = new ArrayList<>(NR_SOCKETS);
    s_queues = new ArrayList<>(NR_SOCKETS);

    int quotient = (int)(MAX_ITERS / NR_SOCKETS);
    int remainder = (int)(MAX_ITERS - (NR_SOCKETS * quotient));

    for (int i = 0; i < NR_SOCKETS; ++i) {
      int iter = quotient;
      if (remainder > 0) { ++iter; --remainder; }
      SpscArrayQueue<String> q = new SpscArrayQueue<>(Globals.QUEUE_SIZE);
      s_queues.add(q);
      thrs.add(new TestStrThread(iter, q));
    }
  }

  public void init(int nr_sockets, String [] addresses, int tuning_knob) {

    assert nr_sockets == addresses.length;

    NR_SOCKETS = nr_sockets;
    tuningKnob = tuning_knob;
    thrs = new ArrayList<>(NR_SOCKETS);
    sarr_queues = new ArrayList<>(NR_SOCKETS);

    for (int i = 0; i < NR_SOCKETS; ++i) {
      SpscArrayQueue<String[]> q = new SpscArrayQueue<>(Globals.QUEUE_SIZE);
      sarr_queues.add(q);
      thrs.add(new BucketThread(cloud_profiler.openZMQChannel(addresses[i]), q,
            tuningKnob));
    }
  }

  public void init(int nr_sockets, long max_iter, int tuning_knob) {

    NR_SOCKETS = nr_sockets;
    MAX_ITERS = max_iter;
    tuningKnob = tuning_knob;
    thrs = new ArrayList<>(NR_SOCKETS);
    sarr_queues = new ArrayList<>(NR_SOCKETS);

    int quotient = (int)(MAX_ITERS / NR_SOCKETS);
    int remainder = (int)(MAX_ITERS - (NR_SOCKETS * quotient));

    for (int i = 0; i < NR_SOCKETS; ++i) {
      int iter = quotient;
      if (remainder > 0) { ++iter; --remainder; }
      SpscArrayQueue<String[]> q = new SpscArrayQueue<>(Globals.QUEUE_SIZE);
      sarr_queues.add(q);
      thrs.add(new TestBucketThread(iter, q, tuningKnob));
    }
  }

  public void initCStrBucket(int nr_sockets, long max_iter, int tuning_knob) {

    NR_SOCKETS = nr_sockets;
    MAX_ITERS = max_iter;
    tuningKnob = tuning_knob;
    thrs = new ArrayList<>(NR_SOCKETS);
    sarr_queues = new ArrayList<>(NR_SOCKETS);

    int quotient = (int)(MAX_ITERS / NR_SOCKETS);
    int remainder = (int)(MAX_ITERS - (NR_SOCKETS * quotient));

    for (int i = 0; i < NR_SOCKETS; ++i) {
      int iter = quotient;
      if (remainder > 0) { ++iter; --remainder; }
      SpscArrayQueue<String[]> q = new SpscArrayQueue<>(Globals.QUEUE_SIZE);
      sarr_queues.add(q);
      thrs.add(new CStrBucketThread(iter, q, tuningKnob));
    }
  }

  public void initZMQStrBucket(int nr_sockets, String [] addresses,
                               int tuning_knob) {

    assert nr_sockets == addresses.length;

    NR_SOCKETS = nr_sockets;
    tuningKnob = tuning_knob;
    thrs = new ArrayList<>(NR_SOCKETS);
    sarr_queues = new ArrayList<>(NR_SOCKETS);

    for (int i = 0; i < NR_SOCKETS; ++i) {
      SpscArrayQueue<String[]> q = new SpscArrayQueue<>(Globals.QUEUE_SIZE);
      sarr_queues.add(q);
      thrs.add(new ZMQStrBucketThread(
            cloud_profiler.openZMQBucketChannel(addresses[i]), q, tuningKnob));
    }
  }

  public void initZMQValBucket(int nr_sockets, String [] addresses,
                               int tuning_knob) {

    assert nr_sockets == addresses.length;

    NR_SOCKETS = nr_sockets;
    tuningKnob = tuning_knob;
    thrs = new ArrayList<>(NR_SOCKETS);
    varr_queues = new ArrayList<>(NR_SOCKETS);
    alen_queues = new ArrayList<>(NR_SOCKETS);

    for (int i = 0; i < NR_SOCKETS; ++i) {
      SpscArrayQueue<Values[]> v_q = new SpscArrayQueue<>(Globals.QUEUE_SIZE);
      SpscArrayQueue<Integer>  l_q = new SpscArrayQueue<>(Globals.QUEUE_SIZE);
      varr_queues.add(v_q);
      alen_queues.add(l_q);
      thrs.add(new ZMQValBucketThread(
            cloud_profiler.openZMQBucketChannel(addresses[i]), v_q, l_q,
            tuningKnob));
    }
  }

  public void initZVBucketOpt(int nr_sockets, String [] addresses,
                               int tuning_knob) {

    assert nr_sockets == addresses.length;

    ArrayList<ZVBucketOptThread> tempLstThrs = new ArrayList<>(NR_SOCKETS);

    NR_SOCKETS = nr_sockets;
    tuningKnob = tuning_knob;
    thrs = new ArrayList<>(NR_SOCKETS);
    varr_queues = new ArrayList<>(NR_SOCKETS);
    alen_queues = new ArrayList<>(NR_SOCKETS);
    arr_ch = new ArrayList<>(NR_SOCKETS);

    // Large allocations take place here:
    for (int i = 0; i < NR_SOCKETS; ++i) {
      SpscArrayQueue<Values[]> v_q = new SpscArrayQueue<>(Globals.QUEUE_SIZE);
      SpscArrayQueue<Integer>  l_q = new SpscArrayQueue<>(Globals.QUEUE_SIZE);
      varr_queues.add(v_q);
      alen_queues.add(l_q);
      ZVBucketOptThread thr = new ZVBucketOptThread(v_q, l_q, tuningKnob, i);
      thrs.add(thr);
      tempLstThrs.add(thr);
    }

    for (int i = 0; i < NR_SOCKETS; ++i) {
      long ch = cloud_profiler.openZMQBktOptChannel(addresses[i]);
      arr_ch.add(ch);
    }

    for (int i = 0; i < NR_SOCKETS; ++i) {
      tempLstThrs.get(i).setChannel(arr_ch.get(i));
      cloud_profiler.initTimeoutVars(arr_ch.get(i), NR_SOCKETS);
    }
  }

  public void initZMQVal1QBkt(int nr_sockets, String [] addresses,
                               int tuning_knob) {

    assert nr_sockets == addresses.length;

    NR_SOCKETS = nr_sockets;
    tuningKnob = tuning_knob;
    thrs = new ArrayList<>(NR_SOCKETS);
    varr_queues = new ArrayList<>(NR_SOCKETS);

    for (int i = 0; i < NR_SOCKETS; ++i) {
      SpscArrayQueue<Values[]> v_q = new SpscArrayQueue<>(Globals.QUEUE_SIZE);
      varr_queues.add(v_q);
      thrs.add(new ZMQValBkt1QThread(
            cloud_profiler.openZMQBucketChannel(addresses[i]), v_q,
            tuningKnob));
    }
  }

  public boolean runFactory() {
    if (thrs.size() != NR_SOCKETS) return false;

    for (int i = 0; i < NR_SOCKETS; ++i) {
      thrs.get(i).start();
    }

    return true;
  }

  public String fetchJStr() {
    String jStr = null;
    if (1 == NR_SOCKETS) {
      do {
        jStr = s_queues.get(0).poll();
      } while ( jStr == null ) ;
    }
    else if (2 == NR_SOCKETS) {
      do {
        jStr = s_queues.get(queueIdx).poll();
        queueIdx = ((queueIdx == 0) ? 1 : 0);
      } while ( jStr == null ) ;
    }
    else if (2 < NR_SOCKETS) {
      do {
        jStr = s_queues.get(queueIdx).poll();
        ++queueIdx;
        if (queueIdx == NR_SOCKETS) queueIdx = 0;
      } while ( jStr == null ) ;
    }
    return jStr;
  }

  public String[] fetchBucket() {
    String[] bucket = null;
    if (1 == NR_SOCKETS) {
      do {
        bucket = sarr_queues.get(0).poll();
      } while ( bucket == null ) ;
    }
    else if (2 == NR_SOCKETS) {
      do {
        bucket = sarr_queues.get(queueIdx).poll();
        queueIdx = ((queueIdx == 0) ? 1 : 0);
      } while ( bucket == null ) ;
    }
    else if (2 < NR_SOCKETS) {
      do {
        bucket = sarr_queues.get(queueIdx).poll();
        ++queueIdx;
        if (queueIdx == NR_SOCKETS) queueIdx = 0;
      } while ( bucket == null ) ;
    }
    return bucket;
  }

  // Try to dequeue from all queues until a valid nr_filled is dequeued.
  // Then use the same queueIdx until a successful dequeue of vBucket.
  public void fetchVBucket(ValuesData data) {
    Values[] vBucket = null;
    Integer nr_filled = null;
    if (1 == NR_SOCKETS) {
      while (true) {
        if (null != (nr_filled = alen_queues.get(0).poll())) {
          while ( null == (vBucket = varr_queues.get(0).poll())) { ; }
          break;
        }
        else {
          continue;
        }
      }
    }
    else if (2 == NR_SOCKETS) {
      do {
        if (null != (nr_filled = alen_queues.get(queueIdx).poll())) {
          while ( null == (vBucket = varr_queues.get(queueIdx).poll())) { ; }
          queueIdx = ((queueIdx == 0) ? 1 : 0);
          break;
        }
        else {
          queueIdx = ((queueIdx == 0) ? 1 : 0);
          continue;
        }
      } while (true) ;
    }
    else if (2 < NR_SOCKETS) {
      do {
        if (null != (nr_filled = alen_queues.get(queueIdx).poll())) {
          while ( null == (vBucket = varr_queues.get(queueIdx).poll())) { ; }
          ++queueIdx;
          if (queueIdx == NR_SOCKETS) queueIdx = 0;
          break;
        }
        else {
          ++queueIdx;
          if (queueIdx == NR_SOCKETS) queueIdx = 0;
          continue;
        }
      } while (true) ;
    }
    data.vBucket = vBucket;
    data.nr_filled = nr_filled.intValue();
  }

  // TODO: not completely non-blocking
  public boolean fetchVBucketNB(ValuesData data) {
    boolean bRet = false;
    Values[] vBucket = null;
    Integer nr_filled = null;
    if (1 == NR_SOCKETS) {
      if (null != (nr_filled = alen_queues.get(0).poll())) {
        while ( null == (vBucket = varr_queues.get(0).poll())) { ; }
        bRet = true;
      }
      else {
        bRet = false;
      }

    }
    else if (2 == NR_SOCKETS) {
      if (null != (nr_filled = alen_queues.get(queueIdx).poll())) {
        while ( null == (vBucket = varr_queues.get(queueIdx).poll())) { ; }
        queueIdx = ((queueIdx == 0) ? 1 : 0);
        bRet = true;
      }
      else {
        queueIdx = ((queueIdx == 0) ? 1 : 0);
        bRet = false;
      }
    }
    else if (2 < NR_SOCKETS) {
      if (null != (nr_filled = alen_queues.get(queueIdx).poll())) {
        while ( null == (vBucket = varr_queues.get(queueIdx).poll())) { ; }
        ++queueIdx;
        if (queueIdx == NR_SOCKETS) queueIdx = 0;
        bRet = true;
      }
      else {
        ++queueIdx;
        if (queueIdx == NR_SOCKETS) queueIdx = 0;
        bRet = false;
      }
    }

    if (bRet) {
      data.vBucket = vBucket;
      data.nr_filled = nr_filled.intValue();
    }

    return bRet;
  }

  public Values[] fetchV1QBucket() {
    Values[] bucket = null;
    if (1 == NR_SOCKETS) {
      do {
        bucket = varr_queues.get(0).poll();
      } while ( bucket == null ) ;
    }
    else if (2 == NR_SOCKETS) {
      do {
        bucket = varr_queues.get(queueIdx).poll();
        queueIdx = ((queueIdx == 0) ? 1 : 0);
      } while ( bucket == null ) ;
    }
    else if (2 < NR_SOCKETS) {
      do {
        bucket = varr_queues.get(queueIdx).poll();
        ++queueIdx;
        if (queueIdx == NR_SOCKETS) queueIdx = 0;
      } while ( bucket == null ) ;
    }
    return bucket;
  }

  private static class JStrThread extends Thread {
    private long channel;
    private SpscArrayQueue<String> queue; // allow access from child classes

    public JStrThread(long ch, SpscArrayQueue<String> q) {
      channel = ch;
      queue = q;
    }

    @Override
    public void run() {
      while (true) {
        while (!queue.offer(cloud_profiler.fetchZMQStr(channel))) { ; }
      }
    }
  }

  private static class TestStrThread extends Thread {
    private long maxIterations;
    private SpscArrayQueue<String> queue; // allow access from child classes

    public TestStrThread(long iter, SpscArrayQueue<String> q) {
      maxIterations = iter;
      queue = q;
    }

    @Override
    public void run() {
      for (long i = 0; i < maxIterations; ++i) {
        String str = "{\"user_id\": \"06461f9b-ed9b-45f6-b944-76224da5cd95\","
                   + " \"page_id\": \"d890b51d-46ad-49db-95ff-fae3255712d9\","
                   + " \"ad_id\": \"b9f90277-6c53-4b45-a329-fcd2338f11e7\","
                   + " \"ad_type\": \"mail\","
                   + " \"event_type\": \"view\","
                   + " \"event_time\": \"1534761527997\","
                   + " \"tuple_idx\": \"1\","
                   + " \"ip_address\": \"10.140.0.121\"}";
        while (!queue.offer(str)) { ; }
      }
    }
  }

  private static class BucketThread extends Thread {
    private long channel;
    private int tuningKnob;
    private SpscArrayQueue<String[]> queue; // allow access from child classes

    public BucketThread(long ch, SpscArrayQueue<String[]> q, int t_knb) {
      channel = ch;
      tuningKnob = t_knb;
      queue = q;
    }

    @Override
    public void run() {
      while (true) {
        String[] bucket = new String[tuningKnob];
        for (int b_idx = 0; b_idx < tuningKnob; ++b_idx) {
          String s = cloud_profiler.fetchZMQStr(channel);
          bucket[b_idx] = s;
        }
        while (!queue.offer(bucket)) { ; }
      }
    }
  }

  private static class TestBucketThread extends Thread {
    private long nr_buckets;
    private int tuningKnob;
    private SpscArrayQueue<String[]> queue; // allow access from child classes

    public TestBucketThread(long iter, SpscArrayQueue<String[]> q, int t_knb) {
      tuningKnob = t_knb;
      nr_buckets = iter / tuningKnob;
      queue = q;
    }

    @Override
    public void run() {
      for (long i = 0; i < nr_buckets; ++i) {
        String[] bucket = new String[tuningKnob];
        for (int b_idx = 0; b_idx < tuningKnob; ++b_idx) {
          String s = "{\"user_id\": \"06461f9b-ed9b-45f6-b944-76224da5cd95\","
                   + " \"page_id\": \"d890b51d-46ad-49db-95ff-fae3255712d9\","
                   + " \"ad_id\": \"b9f90277-6c53-4b45-a329-fcd2338f11e7\","
                   + " \"ad_type\": \"mail\","
                   + " \"event_type\": \"view\","
                   + " \"event_time\": \"1534761527997\","
                   + " \"tuple_idx\": \"1\","
                   + " \"ip_address\": \"10.140.0.121\"}";
          bucket[b_idx] = s;
        }
        while (!queue.offer(bucket)) { ; }
      }
    }
  }

  private static class CStrBucketThread extends Thread {
    private long nr_buckets;
    private int tuningKnob;
    private SpscArrayQueue<String[]> queue;

    public CStrBucketThread(long iter, SpscArrayQueue<String[]> q, int t_knb) {
      tuningKnob = t_knb;
      nr_buckets = iter / tuningKnob;
      queue = q;
    }

    @Override
    public void run() {
      for (long i = 0; i < nr_buckets; ++i) {
        String[] bucket = new String[tuningKnob];
        cloud_profiler.fetchCStrBucket(bucket);
        while (!queue.offer(bucket)) { ; }
      }
    }
  }

  private static class ZMQStrBucketThread extends Thread {
    private long channel;
    private int tuningKnob;
    private SpscArrayQueue<String[]> queue;

    public ZMQStrBucketThread(long ch, SpscArrayQueue<String[]> q, int t_knb) {
      channel = ch;
      tuningKnob = t_knb;
      queue = q;
    }

    @Override
    public void run() {
      while (true) {
        // https://docs.oracle.com/javase/specs/jls/se8/html/jls-4.html
        // JLS 4.12.5 Initial Values of Variables:
        // Every variable in a program must have a value before its value is
        // used:
        // * Each class variable, instance variable, or array component is
        //   initialized with a default value when it is created (15.9,
        //   15.10.2):
        //   * For all reference types (4.3), the default value is null.
        String[] bucket = new String[tuningKnob];
        cloud_profiler.fetchZMQBucket(channel, tuningKnob, bucket);
        while (!queue.offer(bucket)) { ; }
      }
    }
  }

  private static class ZMQValBucketThread extends Thread {
    private long channel;
    private int tuningKnob;
    private SpscArrayQueue<Values[]> values_q;
    private SpscArrayQueue<Integer>  length_q;

    public ZMQValBucketThread(long ch, SpscArrayQueue<Values[]> v_q,
                              SpscArrayQueue<Integer> l_q, int t_knb) {
      channel = ch;
      tuningKnob = t_knb;
      values_q = v_q;
      length_q = l_q;
    }

    @Override
    public void run() {
      String[] strBucket = new String[tuningKnob];
      Values[][] valBuckets = new Values[Globals.QUEUE_SIZE + 1][tuningKnob];
      int nr_filled = 0, b_idx = 0, arr_idx = 0;

      while (true) {
        nr_filled = cloud_profiler.fetchZMQBucket(channel, tuningKnob, strBucket);
        Values[] vBucket = valBuckets[arr_idx];
        for (b_idx = 0; b_idx < nr_filled; ++b_idx) {
          vBucket[b_idx] = new Values(0, 0, 0, 0, strBucket[b_idx]);
          strBucket[b_idx] = null;
        }
        // TODO: create Integer objects here
        while (!length_q.offer(Integer.valueOf(nr_filled))) { ; }
        while (!values_q.offer(vBucket)) { ; }
        // after a successful enqueue, arr_idx will increase or wrap-around
        arr_idx++;
        if (arr_idx > Globals.QUEUE_SIZE) arr_idx = 0;
      }
    }
  }

  private static class ZVBucketOptThread extends Thread {
    private long channel;
    private int tuningKnob;
    private int thread_id;
    private SpscArrayQueue<Values[]> values_q;
    private SpscArrayQueue<Integer>  length_q;
    private String[] strBucket;
    private Values[][] valBuckets;

    public ZVBucketOptThread(SpscArrayQueue<Values[]> v_q,
                             SpscArrayQueue<Integer> l_q, int t_knb, int tid) {
      tuningKnob = t_knb;
      values_q = v_q;
      length_q = l_q;
      thread_id = tid;

      strBucket = new String[tuningKnob];
      valBuckets = new Values[Globals.QUEUE_SIZE + 1][tuningKnob];
    }

    public void setChannel(long ch) {
      channel = ch;
    }

    @Override
    public void run() {
      int nr_filled = 0, b_idx = 0, arr_idx = 0;

      while (true) {
        nr_filled = cloud_profiler.fetchZMQBktOpt(channel, tuningKnob,
                                                  thread_id, strBucket);

        Values[] vBucket = valBuckets[arr_idx];
        for (b_idx = 0; b_idx < nr_filled; ++b_idx) {
          vBucket[b_idx] = new Values(0, 0, 0, 0, strBucket[b_idx]);
        }
        Integer intRef = Integer.valueOf(nr_filled);
        while (!length_q.offer(intRef)) { ; }
        while (!values_q.offer(vBucket)) { ; }
        // after a successful enqueue, arr_idx will increase or wrap-around
        arr_idx++;
        if (arr_idx > Globals.QUEUE_SIZE) arr_idx = 0;
      }
    }
  }

  private static class ZMQValBkt1QThread extends Thread {
    private long channel;
    private int tuningKnob;
    private SpscArrayQueue<Values[]> values_q;

    public ZMQValBkt1QThread(long ch, SpscArrayQueue<Values[]> v_q, int t_knb) {
      channel = ch;
      tuningKnob = t_knb;
      values_q = v_q;
    }

    @Override
    public void run() {
      String[] strBucket = new String[tuningKnob];
      Values[][] valBuckets = new Values[Globals.QUEUE_SIZE + 1][tuningKnob];
      int nr_filled = 0, b_idx = 0, arr_idx = 0;

      while (true) {
        nr_filled = cloud_profiler.fetchZMQBucket(channel, tuningKnob, strBucket);
        Values[] vBucket = valBuckets[arr_idx];
        for (b_idx = 0; b_idx < nr_filled; ++b_idx) {
          vBucket[b_idx] = new Values(0, 0, 0, 0, strBucket[b_idx]);
          strBucket[b_idx] = null;
        }
        while (!values_q.offer(vBucket)) { ; }
        // after a successful enqueue, arr_idx will increase or wrap-around
        arr_idx++;
        if (arr_idx > Globals.QUEUE_SIZE) arr_idx = 0;
      }
    }
  }

  private static class SwigStrThread extends Thread {
    private long maxIterations;
    private SpscArrayQueue<String> queue;

    public SwigStrThread(long iter, SpscArrayQueue<String> q) {
      maxIterations = iter;
      queue = q;
    }

    @Override
    public void run() {
      for (long i = 0; i < maxIterations; ++i) {
        String str = cloud_profiler.fetchCStr();
        while (!queue.offer(str)) { ; }
      }
    }
  }
}
