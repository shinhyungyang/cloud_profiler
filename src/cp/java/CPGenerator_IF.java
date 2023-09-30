package cp.java;

public interface CPGenerator_IF {
  /**
   * Returns the byte array of a new tuple serialized.  It does not add a tuple
   * index.
   *
   * @return  the byte array
   */
  public byte[] getBytes();

  /**
   * Returns the byte array of a new tuple serialized.  It requires a tuple
   * index.
   *
   * @param   tupleIndex  the index of the new tuple
   *
   * @return  the byte array
   */
  public byte[] getBytes(long tupleIndex);
}
