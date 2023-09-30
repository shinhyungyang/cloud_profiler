package cp.java;

import org.jctools.queues.SpscArrayQueue;

public class SPSCQueue<E> extends SpscArrayQueue<E> {

  public SPSCQueue(int capacity) {
    super(capacity);
  }
}
