#ifndef __SERIALIZATION_H__
#define __SERIALIZATION_H__

#include <iostream>
#include "tsc_messages.h"

std::string tsc_serialize(message * m);
void tsc_deserialize(std::string str, message ** m);

#endif
