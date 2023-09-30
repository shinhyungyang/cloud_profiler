package cp.java;

import com.esotericsoftware.kryo.Kryo;
import com.esotericsoftware.kryo.io.Input;
import com.esotericsoftware.kryo.io.Output;

import cp.swig.*;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

import java.util.concurrent.atomic.AtomicBoolean;

import java.util.ArrayList;
import java.util.Arrays;

import org.apache.storm.tuple.Values;

public class JObjFactory {
  private ArrayList<Thread> thrs;                          // Thread
  private ArrayList<SPSCQueue<Values[]>> varr_queues; // Values array queue
  private ArrayList<SPSCQueue<Integer>>  alen_queues; // array-length queue
  private ArrayList<SPSCQueue<Values[]>> back_queues; // Returning queue
  private int NR_SOCKETS;
  private long MAX_ITERS;
  private int tuningKnob;
  private int queueIdx = 0; // only use in fetchVBucketNB()
  private int oldQueueIdx = 0; // only use in fetchVBucketNB()
  Values[] vBucketFilled = null; // only use in fetchVBucketNB()

  private ArrayList<Long> arr_ch;  // temporary array for storing channel ids

  private SharedVariables sharedVariables = null;

  public JObjFactory() {
    CloudProfiler.init();
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
    back_queues = new ArrayList<>(NR_SOCKETS);
    arr_ch = new ArrayList<>(NR_SOCKETS);

    sharedVariables = new SharedVariables();

    // Large allocations take place here:
    for (int i = 0; i < NR_SOCKETS; ++i) {
      SPSCQueue<Values[]> v_q = new SPSCQueue<>(Globals.QUEUE_SIZE);
      SPSCQueue<Integer>  l_q = new SPSCQueue<>(Globals.QUEUE_SIZE);
      SPSCQueue<Values[]> b_q = new SPSCQueue<>(Globals.QUEUE_SIZE + 1);
      varr_queues.add(v_q);
      alen_queues.add(l_q);
      back_queues.add(b_q);
      ZVBucketOptThread thr = new ZVBucketOptThread(v_q, l_q, b_q,
          sharedVariables, tuningKnob, i);
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

  public boolean runFactory() {
    if (thrs.size() != NR_SOCKETS) return false;

    for (int i = 0; i < NR_SOCKETS; ++i) {
      thrs.get(i).start();
    }

    return true;
  }

  // TODO: not completely non-blocking
  public boolean fetchVBucketNB(ValuesData data) {
    boolean bRet = false;
  //Values[] vBucketFilled = null;
    Integer nr_filled = null;

    if (vBucketFilled != null) {
      while (!back_queues.get(oldQueueIdx).offer(vBucketFilled)) { ; }
      vBucketFilled = null;
    }

    if (1 == NR_SOCKETS) {
      if (null != (nr_filled = alen_queues.get(0).poll())) {
        while ( null == (vBucketFilled = varr_queues.get(0).poll())) { ; }
        bRet = true;
      }
      else {
        bRet = false;
      }

    }
    else if (2 == NR_SOCKETS) {
      if (null != (nr_filled = alen_queues.get(queueIdx).poll())) {
        while ( null == (vBucketFilled = varr_queues.get(queueIdx).poll())) { ; }
        oldQueueIdx = queueIdx;
        queueIdx = ((queueIdx == 0) ? 1 : 0);
        bRet = true;
      }
      else {
        oldQueueIdx = queueIdx;
        queueIdx = ((queueIdx == 0) ? 1 : 0);
        bRet = false;
      }
    }
    else if (2 < NR_SOCKETS) {
      if (null != (nr_filled = alen_queues.get(queueIdx).poll())) {
        while ( null == (vBucketFilled = varr_queues.get(queueIdx).poll())) { ; }
        oldQueueIdx = queueIdx;
        ++queueIdx;
        if (queueIdx == NR_SOCKETS) queueIdx = 0;
        bRet = true;
      }
      else {
        oldQueueIdx = queueIdx;
        ++queueIdx;
        if (queueIdx == NR_SOCKETS) queueIdx = 0;
        bRet = false;
      }
    }

    if (bRet) {
      data.vBucket = vBucketFilled;
      data.nr_filled = nr_filled.intValue();
    }
    else {
      sharedVariables.markUnderflow();
    }

    return bRet;
  }

  private static class SharedVariables {
    private static final AtomicBoolean bufferOverflow = new AtomicBoolean(false);
    private static final AtomicBoolean bufferUnderflow = new AtomicBoolean(false);

    public void markOverflow() {
      // Atomically sets the value to the given updated value if the current
      // value == the expected value.
      if (true == bufferOverflow.compareAndSet(false, true)) {
        StackTraceElement l = new Exception().getStackTrace()[0];
        cloud_profiler.logFail(
            l.getFileName(),
            l.getLineNumber(),
            "Bucket queue is full"
            );
      }
    }

    public void markUnderflow() {
      if (true == bufferOverflow.compareAndSet(false, true)) {
        StackTraceElement l = new Exception().getStackTrace()[0];
        cloud_profiler.logFail(
            l.getFileName(),
            l.getLineNumber(),
            "Bucket queue is empty"
            );
      }
    }
  }

  private static class ZVBucketOptThread extends Thread {
    private long channel;
    private int tuningKnob;
    private int thread_id;
    private SPSCQueue<Values[]> values_q;
    private SPSCQueue<Integer>  length_q;
    private SPSCQueue<Values[]> back_q;
    private byte[][] byteArrayBucket;
    private Values[][] valBuckets;
    private Kryo kryo;
    private Input input;
    private long ch_fpc;
    private long ch_bid;
    private SharedVariables sharedVars;

    public ZVBucketOptThread(SPSCQueue<Values[]> v_q, // Values Queue
                             SPSCQueue<Integer> l_q,  // Length Queue
                             SPSCQueue<Values[]> b_q, // Back Queue
                             SharedVariables s_v,     // Shared Variables
                             int t_knb, int tid) {
      tuningKnob = t_knb;
      values_q = v_q;
      length_q = l_q;
      back_q = b_q;
      thread_id = tid;
      sharedVars = s_v;

      byteArrayBucket = new byte[tuningKnob][];
      valBuckets = new Values[Globals.QUEUE_SIZE + 1][tuningKnob];

      // Initialize the back queue with Values array references
      for (int i = 0; i < Globals.QUEUE_SIZE + 1; ++i) {
        Values[] v_b = valBuckets[i];
        back_q.offer(v_b);
      }

      kryo = new Kryo();
      kryo.register(String.class);
      kryo.register(Integer.class);
      kryo.register(Long.class);
      input = new Input();
    }

    public void setChannel(long ch) {
      channel = ch;
    }

    @Override
    public void run() {
      int nr_filled = 0, b_idx = 0;
      //int  arr_idx = 0;

      String cls = this.getClass().getSimpleName();
      ch_fpc = cloud_profiler.openChannel(String.format("%-19s%10s", cls, "FPC_10MS"), log_format.ASCII, handler_type.NET_CONF);
      ch_bid = cloud_profiler.openChannel(String.format("%-19s%10s", cls, "IDENTITY"), log_format.ASCII, handler_type.NET_CONF);
      if (!(cloud_profiler.isOpen(ch_fpc) && cloud_profiler.isOpen(ch_bid))) {
        System.out.println("Warning: some channel(s) failed to open.");
        System.out.println("Check the cloud_profiler log file.");
      }

      while (true) {
        nr_filled = cloud_profiler.fetchObjBktOpt(channel, tuningKnob,
                                                  thread_id, byteArrayBucket);
        Values[] vBucket = null;
        while ( null == (vBucket = back_q.poll())) { ; }
        // wrong way
        //Values[] vBucket = valBuckets[arr_idx];

        for (b_idx = 0; b_idx < nr_filled; ++b_idx) {
          input.setBuffer(byteArrayBucket[b_idx]);
          vBucket[b_idx] = new Values(0, 0, 0, 0, kryo.readClassAndObject(input));

      //// Tuple Duplication Debugging
      //////
      //final ByteBuffer buffer = ByteBuffer.wrap(byteArrayBucket[b_idx]);
      //buffer.order(ByteOrder.LITTLE_ENDIAN);
      //buffer.position(0);
      //buffer.put(byteArrayBucket[b_idx]);
      //long tupleIdx = ((ByteBuffer)buffer.position(0)).getLong();
      //
      //vBucket[b_idx] = new Values(0, 0, 0, 0, byteArrayBucket[b_idx]);
        cloud_profiler.logTS(ch_fpc, 0L);
      //cloud_profiler.logTS(ch_bid, tupleIdx);
      ////
        }

        Integer intRef = Integer.valueOf(nr_filled);
        if (!length_q.offer(intRef)) {
          sharedVars.markOverflow();
          while (!length_q.offer(intRef)) { ; }
        }
        while (!values_q.offer(vBucket)) { ; }
        // after a successful enqueue, arr_idx will increase or wrap-around
      //arr_idx++;
      //if (arr_idx > Globals.QUEUE_SIZE) arr_idx = 0;
      }
    }
  }
}
