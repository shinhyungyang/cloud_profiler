package cp.java;

import java.util.Random;

public class Util {

  private static Random rand;
  
  static {
    rand = new Random(System.nanoTime());
  }

  public static String randomIdentity() {
    return String.format("%04X-%04X", rand.nextInt(), rand.nextInt());
  }
}
