#include <string>
#include <iostream>

#include "indent.hxx"

namespace indent {

Indenter::Indenter(std::ostream& stream_, unsigned current_,
                   unsigned max_length_, unsigned indent_)
    : stream(stream_),
      max_length(max_length_),
      indent(indent_),
      current(current_) {}

Indenter& operator<<(Indenter& indenter, const std::string& word) {
  if (indenter.current > indenter.indent &&
      indenter.current + word.size() + 1 >= indenter.max_length) {
    indenter.stream << '\n';
    for (unsigned i = 0; i < indenter.indent; i++) {
      indenter.stream << ' ';
    }
    indenter.current = indenter.indent;
  }
  if (indenter.current > indenter.indent) {
    indenter.stream << ' ';
    indenter.current++;
  }
  indenter.stream << word;
  indenter.current += word.size();
  return indenter;
}
}
