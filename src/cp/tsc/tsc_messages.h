#ifndef __TSC_MESSAGES_H__
#define __TSC_MESSAGES_H__

//#include <boost/serialization/serialization.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <iostream>

#include "globals.h"

extern "C" {
#include "net.h"
}

struct TSC_msg {
  int64_t req_cyc_start;
  int64_t req_nsec_start;
  int64_t reply_cyc;
  int64_t reply_nsec_start;
  int64_t reply_nsec_end;
};

struct TSC_req_cyc {
  int64_t reply_cyc;
  int64_t reply_ns;
  uint32_t reply_aux;
};

struct MSG_register_request {
  char from_ip[MAX_IPV4_STR_SIZE + 1];
  long long tsc_freq;
};

struct MSG_register_accept {
  int port_number; // port number for TSC measurement
};

struct MSG_measure_req {
  char slave_from_ip[MAX_IPV4_STR_SIZE + 1];
  int slave_from_port;
  char slave_to_ip[MAX_IPV4_STR_SIZE + 1];
  int slave_to_port;
  int finished;
  int iter_per_measurement;
};

struct MSG_measure_result {
  char slave_from_ip[MAX_IPV4_STR_SIZE + 1];
  int slave_from_port;
  char slave_to_ip[MAX_IPV4_STR_SIZE + 1];
  int slave_to_port;
  int64_t rtt_ns;
  int64_t from_cyc;
  int64_t from_ns;
  int64_t to_cyc;
  int64_t to_ns;
  int64_t end_cyc;
  int64_t end_ns;
  int64_t from_aux;
  int64_t to_aux;
  int64_t end_aux;
  int finished;
};
  
class message {
    friend class boost::serialization::access;
    //...
    private:
      std::string from_ip;
    public:
      message() {
#if 1
        char tmpBuf[MAX_IPV4_STR_SIZE] = {0,};
        getIPDottedQuad(tmpBuf, (size_t)MAX_IPV4_STR_SIZE);
        from_ip = std::string(tmpBuf);
#else
        from_ip = std::string((size_t)MAX_IPV4_STR_SIZE, '\0');
        getIPDottedQuad(&from_ip[0], (size_t)(MAX_IPV4_STR_SIZE));
        data = static_cast<void*>(new std::string("mind game"));
#endif
      }
    // only required when using method 1 below
    // no real serialization required - specify a vestigial one
    template<class Archive>
    void serialize(Archive & ar, const unsigned int file_version){ /*ar & x;*/}
    virtual void speak() = 0;
    virtual std::string str() const = 0;
    std::string from() { return from_ip; }
    virtual ~message() = default;
};

class connect_req : public message {
    friend class boost::serialization::access;
    private:
      std::string msg = "connect_req";
    public:
    //int nr = 22;
      connect_req() = default;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int file_version){
        // method 1 : invoke base class serialization
        ar & boost::serialization::base_object<message>(*this);
        //ar & y;
        // method 2 : explicitly register base/derived relationship
        /*
        boost::serialization::void_cast_register<derived, base>(
            static_cast<derived *>(NULL),
            static_cast<base *>(NULL)
        );
        */
    }
    virtual void speak() { std::cout << msg << std::endl; }
    virtual std::string str() const { return msg; }
};

class register_req : public message {
  friend class boost::serialization::access;
  private:
    std::string msg = "register_req";
  public:
    register_req() = default;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int file_version){
      ar & boost::serialization::base_object<message>(*this);
    }
    virtual void speak() {}
    virtual std::string str() const { return msg; }
};

class register_accept : public message {
  friend class boost::serialization::access;
    std::string msg = "register_accept";
  public:
    int port_number;
    register_accept() = default;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int file_version){
      ar & boost::serialization::base_object<message>(*this);
    }
    virtual void speak() {}
    virtual std::string str() const { return msg; }
};

class measure_req : public message {
  friend class boost::serialization::access;
  private:
    std::string msg = "measure_req";
  public:
    measure_req() = default;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int file_version){
      ar & boost::serialization::base_object<message>(*this);
    }
    virtual void speak() {}
    virtual std::string str() const { return msg; }
};

class meas_req_accept : public message {
  friend class boost::serialization::access;
  private:
    std::string msg = "meas_req_accept";
  public:
    meas_req_accept() = default;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int file_version){
      ar & boost::serialization::base_object<message>(*this);
    }
    virtual void speak() {}
    virtual std::string str() const { return msg; }
};

#endif
