#ifndef INDENT_HXX
#define INDENT_HXX

#include <string>
#include <iostream>

namespace indent {
class Indenter {
  std::ostream& stream;
  const unsigned max_length;
  const unsigned indent;
  unsigned current;

 public:
  Indenter(std::ostream& stream, unsigned current, unsigned max_length,
           unsigned indent);

  friend Indenter& operator<<(Indenter& indenter, const std::string& word);
};
}

#endif
