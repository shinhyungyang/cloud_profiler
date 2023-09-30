#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <iostream>
#include <sstream>

using namespace boost::archive;
std::stringstream ss;

class animal
{
public:
  animal() = default;
  animal(int legs) : legs_{legs} {}
  virtual void speak() { std::cout << "animal\n"; }
  int legs() const { return legs_; }

private:
  friend class boost::serialization::access;

  template <typename Archive>
  void serialize(Archive &ar, const unsigned int version) { ar & legs_; }

  int legs_;
};

class bird : public animal
{
public:
  bird() = default;
  bird(int legs, bool can_fly) :
    animal{legs}, can_fly_{can_fly} {}
  bool can_fly() const { return can_fly_; }
  virtual void speak() { std::cout << "bird\n"; }

private:
  friend class boost::serialization::access;

  template <typename Archive>
  void serialize(Archive &ar, const unsigned int version)
  {
    ar & boost::serialization::base_object<animal>(*this);
    ar & can_fly_;
  }

  bool can_fly_;
};

void save()
{
  text_oarchive oa{ss};
  bird penguin{2, false};
  oa << penguin;
}

void load()
{
  text_iarchive ia{ss};
  //bird penguin;
  animal penguin;
  ia >> penguin;
  //std::cout << penguin.legs() << '\n';
  //std::cout << std::boolalpha << penguin.can_fly() << '\n';
  penguin.speak(); 
}

int main()
{
  save();
  load();
}
