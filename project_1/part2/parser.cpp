// Adapted from 
// http://stackoverflow.com/questions/17465061/how-to-parse-space-separated-floats-in-c-quickly/17479702#17479702
#include "funcs.h"

namespace qi = boost::spirit::qi;

int parse_i2(std::string file_name, data_i2 &data )
{
  boost::iostreams::mapped_file mmap(
      file_name, 
      boost::iostreams::mapped_file::readonly);
  auto f = mmap.const_data();
  auto l = f + mmap.size();

  using namespace qi;
  bool ok = phrase_parse(f,l,(int_ > int_) % eol, blank, data);
  if (ok)   
    //std::cout << "parse success\n";
    ;
  else 
    std::cerr << "parse failed: '" << std::string(f,l) << "'\n";

  if (f!=l) //std::cerr << "trailing unparsed: '" << std::string(f,l) << "'\n";
    ;
  errno = 0;
  char* next = nullptr;
  int2 tmp;
  while (errno == 0 && f && f<(l-12) ) {
    tmp.x = strtod(f, &next); f = next;
    tmp.y = strtod(f, &next); f = next;
    data.push_back(tmp);
  }

  //std::cout << "data.size():   " << data.size() << "\n";

  return 0;
}

int parse_i3(std::string file_name, data_i3 &data )
{
  boost::iostreams::mapped_file mmap(
      file_name, 
      boost::iostreams::mapped_file::readonly);
  auto f = mmap.const_data();
  auto l = f + mmap.size();

  using namespace qi;
  bool ok = phrase_parse(f,l,(int_ > int_ > int_) % eol, blank, data);
  if (ok)   
    //std::cout << "parse success\n";
    ;
  else 
    std::cerr << "parse failed: '" << std::string(f,l) << "'\n";

  if (f!=l) //std::cerr << "trailing unparsed: '" << std::string(f,l) << "'\n";
    ;
  errno = 0;
  char* next = nullptr;
  int3 tmp;
  while (errno == 0 && f && f<(l-12) ) {
    tmp.ID = strtod(f, &next); f = next;
    tmp.degree = strtod(f, &next); f = next;
    tmp.part = strtod(f, &next); f = next;
    data.push_back(tmp);
  }

  //std::cout << "data.size():   " << data.size() << "\n";

  return 0;
}
