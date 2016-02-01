#include <iostream>
#include <string>
#include "cpparse.hxx"

using namespace std;
using namespace cpparse;

int main(int argc, char** argv) {
  Parser parser("This program just parses command line arguments");
  auto& flag = parser.add_flag<>("flag", 'f', true).help("Activate flag");
  auto& number = parser.add_optargument<int>("num", 'n').help("Set a number");
  auto& string_arg =
      parser.add_argument<>("string").help("This is a required string");
  parser.parse(argc, argv);

  cout << "Flag was " << (flag.get() ? "" : "un") << "set, ";
  cout << "number was " << number.get() << ", and ";
  cout << "the string arg was \"" << string_arg.get() << '"' << endl;
}
