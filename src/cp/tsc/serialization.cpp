#include "serialization.h"
#include "tsc_messages.h"
#include <iostream>
#include <sstream>

std::string tsc_serialize(message * m) {
  std::ostringstream oss;
  boost::archive::text_oarchive oa(oss);
  oa & m;
  return oss.str();
}

void tsc_deserialize(std::string str, message ** m) {
  std::istringstream iss(str);
  boost::archive::text_iarchive ia(iss);
  ia >> *m;
}
