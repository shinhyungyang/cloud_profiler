/*
 
Boost serialization:

https://www.boost.org/doc/libs/1_38_0/libs/serialization/doc/serialization.html#derivedpointers
https://theboostcpplibraries.com/boost.serialization-class-hierarchies
g++ -g e2.cpp -lboost_serialization

blog: http://www.ocoudert.com/blog/2011/07/09/a-practical-guide-to-c-serialization/

Google protocol buffers:

https://developers.google.com/protocol-buffers/docs/overview

pos and cons:
https://stackoverflow.com/questions/1061169/boost-serialization-vs-google-protocol-buffers
*/

#include <sstream>
//#include <boost/serialization/serialization.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/export.hpp>

class base {
    friend class boost::serialization::access;
    //...
    public:
      base() = default;
      base(int x)  {x = x;}
      int x;
    // only required when using method 1 below
    // no real serialization required - specify a vestigial one
    template<class Archive>
    void serialize(Archive & ar, const unsigned int file_version){ ar & x;}
    virtual void speak() { std::cout << "base\n"; }
};

class derived : public base {
    friend class boost::serialization::access;
    public:
      derived() = default;
      derived(int x, int y) { x = x; y = y; }
      int y;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int file_version){
        // method 1 : invoke base class serialization
        ar & boost::serialization::base_object<base>(*this);
	ar & y;
        // method 2 : explicitly register base/derived relationship
	/*
        boost::serialization::void_cast_register<derived, base>(
            static_cast<derived *>(NULL),
            static_cast<base *>(NULL)
        );
	*/
    }
    virtual void speak() { std::cout << "derived\n"; }
};

BOOST_CLASS_EXPORT_GUID(derived, "derived")

main(){
    base *d = new derived(1,2);
    std::stringstream ss;
    boost::archive::text_oarchive oa(ss);
    oa & d;
    boost::archive::text_iarchive ia(ss);
    base *b;
    ia >> b; 
    b->speak();
}
