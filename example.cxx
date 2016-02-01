#include <iostream>
#include <string>
#include "cpparse.hxx"

using namespace std;
using namespace cpparse;  // Contains all of the parsing objects

int main(int argc, char** argv) {
  // Creates a local parser. These are designed to be stack variables
  Parser parser("This is a test program with a description!", true);
  // They also take a description (default empty) that will be printed when
  // help is printed, and a boolean (default true) for whether to include a
  // help option.

  // First lets add a simple boolean flag
  auto& bool_flag = parser.add_flag<>("bool", 'b', true);
  // Flags take no options they just update the state. The first argument is
  // the long name (required) followed by an optional short name, and the value
  // to update the flag to if used. Finally, you specify a default value, which
  // is the default constructor if unspecified.

  // Flags default to boolean, but are templated on the type of object they
  // return. Here is a string version without a short name but with a default
  // value
  auto& string_flag = parser.add_flag<string>("str", "set", "unset");

  // The real usefulness of parsing is to take optional arguments.
  auto& double_opt = parser.add_optargument<double>("double", 'd');
  // This takes a default and a converter function for how to interpret the
  // string. Any type that overloads the >> operator has a default converter
  // function.

  // Finally, positional arguments can also be added. They function similarly
  // to optional arguments only you can't specify a short name.
  auto& int_arg = parser.add_argument<int>("int")  //
                      .help("This integer is required but unused");
  // All argument types also support adding help text with the `help` method.

  // Parse the inputs
  parser.parse(argc, argv);

  // All of the functions to add arguments returned reference to mutable
  // argument objects. In addition to ways to update the arguments, each
  // contains a `get` method to get the value. Before parsing this will be the
  // default, after it will be whatever was specified.
  cout << "Boolean flag  : " << boolalpha << bool_flag.get() << '\n';
  cout << "String flag   : " << string_flag.get() << '\n';
  cout << "Double option : " << double_opt.get() << '\n';
  cout << "Int argument  : " << int_arg.get() << endl;
}
