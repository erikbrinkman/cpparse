Cpparse (see-plus-parse)
========================

FIXME TODO Don't push this!
---------------------------

- Merge agg flag and flag, just have an option for how multiple arguments are specified.
- Find a way to get files to work, e.g. unique_ptr<istream>?



A posix style command line parser header for C++. Options are similar to
pythons argparse but built from the ground up to work with modern C++ style
guidelines.

An short program looks something like:
```cpp
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

  cout << "Flag was " << (flag.get() ? "" : "un" ) << "set, ";
  cout << "number was " << number.get() << ", and ";
  cout << "the string arg was \"" << string_arg.get() << '"' << endl;
}
```

When compiled `./readme_example -h` produces:
```
age: ./readme_example [-f] [-h] [-n <num>] <string>

This program just parses command line arguments

Positional Arguments:
  <string>              This is a required string

Optional Arguments:
  -f, --flag            Activate flag
  -h, --help            Show this help message and exit
  -n <num>, --num <num> Set a number
```

This is a work in progress because I was frustrated with the lack of nice C++
command line parsers, and wanted one that used modern nice syntax with type
safety. As a result, it is missing quite a few features that you might want for
your command line needs. I designed this header such that it should be possible
to add all of the desired functionality, but since it's not implemented yet,
there are obviously some open questions.

Unimplemented Features
----------------------

- Variable number of arguments This should probably be done with another
  `Option` derived class. I'm not quite sure how to handle three distinct
  cases:
    - Variable number of argument, something like 0+, 1+, 2-4 : should go in a
      vector
    - Fixed number of arguments : should go in an array to allow bound safety
      and compile time access
    - Fixed number of arguments with distinct types : should go in a tuple
- Counting Arguments / repeated option invocation Right now I'm thinking this
  should be done with a templated  enum that says how handle repeated
  arguments.  There are three right now `overwrite`, `append`, and `error`.
  Overwrite is the default behavior that just replaces content. Append will
  create a vector, and append the result instead of overwrite. For flags, this
  will only be defined for integral types, and will just count. Error will
  throw an error and exit if a flag is repeated.
- Add operator<< to Parser to allow printing args and values.
- Add non manual tests
- Would love to have a compile time error thrown if two arguments share a name or short_name
- Not sure if I want help to print in addition order or sorted. In order would
  probably be easiest with another vector.
- Allow long options with --option=value
- Implement difference between option name and argument name
- Allow updating the program name
- Add argparse style subparsers. Could be possible by making a Parser also an
  Option type, or doing something in between to take advantage of reused
  functionality.
- Add `choices` basically limited input that get validated against. Can
  probably do with initializer lists.
- Add tools to work with bash completion.
