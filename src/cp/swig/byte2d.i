/* -----------------------------------------------------------------------------
 * byte2d.i
 *
 * Typemaps for two-dimensional byte arrays
 * ----------------------------------------------------------------------------- */

%typemap(jni) char **BYTE2D "jobjectArray"
%typemap(jtype) char **BYTE2D "byte[][]"
%typemap(jstype) char **BYTE2D "byte[][]"
%typemap(out) char **BYTE2D {
}
%typemap(javain) char **BYTE2D "$javainput"
%typemap(javaout) char **BYTE2D {
  return $jnicall;
}

%typemap(in) (char** byte2D) {
  jbyte* zmqBytes;
  jbyteArray jBytes;
  int i = 0;
  timeout_vars* tmo = getTimeoutVars(arg1, arg3);
  while (true) {
    tmo->next_timeout = elapsedIntervals(arg1) + 1;
    do {
      do {
        if (elapsedIntervals(arg1) >= tmo->next_timeout) {
          goto bucket_done;
        }
        zmq::message_t zmqmsg;
        zmqBytes = static_cast<jbyte*>(fetchZMQStrNB2(arg1, &zmqmsg));
        jsize nrByte = static_cast<jsize>(zmqmsg.size());
        if (nullptr == zmqBytes) {
          continue;
        }
        else {
          jBytes = JCALL1(NewByteArray, jenv, (jsize)nrByte);
          JCALL4(SetByteArrayRegion, jenv, jBytes, 0, (jsize)nrByte, zmqBytes);
          break;
        }
      } while (true) ;
      // add byte array to bucket:
      JCALL3(SetObjectArrayElement, jenv, $input, i, jBytes);
      JCALL1(DeleteLocalRef, jenv, jBytes);
    } while (++i < arg2) ;
bucket_done:
    if (i > 0) {
      break;
    }
  }
  return i;
}
