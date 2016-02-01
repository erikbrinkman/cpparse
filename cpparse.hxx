#ifndef CPPARSE_HXX
#define CPPARSE_HXX

#include <memory>
#include <map>
#include <vector>
#include <string>

namespace cpparse {

// Default conversion of strings to types
template <typename T>
T read(const std::string& input);

template <>
std::string read<std::string>(const std::string& input);

// Option classes and supporting classes
// Option is an ABC that allows easy storage of all types
template <char C>
struct ArgReader;
template <char C>
class Option;
template <char C, typename T>
class Flag;
template <char C, typename T>
class Argument;

// Parser object
// This controls all of the parsing, and is the main point of api entry
template <char C = '-'>  // How to signify options
class Parser {
  std::map<std::string, std::unique_ptr<Option<C>>> options;
  std::map<char, Option<C>*> short_options;
  std::vector<std::unique_ptr<Option<C>>> arguments;

  std::string program_name;
  std::string description;

  void enroll_option(Option<C>* option);
  void enroll_argument(Option<C>* argument);

 public:
  // Classes that overload << to allow easy formatting of help and usage in
  // streams
  class UsageFormatter;
  class HelpFormatter;

  // Only constructor
  Parser(const std::string& description = "", bool enable_help = true);

  // Add a flag (no arguments) with a short name
  template <typename T = bool>
  Flag<C, T>& add_flag(const std::string& name, char short_name, T constant,
                       T def = T());

  // Add a flag (no arguments) without a short name
  template <typename T = bool>
  Flag<C, T>& add_flag(const std::string& name, T constant, T def = T());

  // Add an optional argument (one arg) not required
  template <typename T = std::string>
  Argument<C, T>& add_optargument(
      const std::string& name, char short_name, T def = T(),
      const std::function<T(const std::string&)> converter = read<T>);

  // Add an optional argument (one arg) not required
  template <typename T = std::string>
  Argument<C, T>& add_optargument(
      const std::string& name, T def = T(),
      const std::function<T(const std::string&)> converter = read<T>);

  // A mandatory positional argument
  template <typename T = std::string>
  Argument<C, T>& add_argument(
      const std::string& name,
      const std::function<T(const std::string&)> converter = read<T>);

  // Call this after adding all of the options
  void parse(int argc, char** argv);

  // Objects that overload <<
  // i.e. to print help `cout << parser.help();`
  UsageFormatter usage() const;
  HelpFormatter help() const;
};

// Flag (no arguments)
// Visible api is basically the same to every type
template <char C, typename T>
class Flag : Option<C> {
  friend class Parser<C>;
  T value;
  const T constant;

  Flag(const std::string& name, char short_name, const T& constant,
       const T& def);
  std::ostream& format_args(std::ostream& os) override;
  void parse(ArgReader<C>& reader) override;

  ~Flag() override{};

 public:
  // Get the value pre or post parsing
  const T& get() const;
  // Set the help text of this option
  Flag& help(const std::string& new_help);
};

// Argument (one argument)
template <char C, typename T>
class Argument : Option<C> {
  friend class Parser<C>;
  T value;
  const std::function<T(const std::string&)> converter;

  Argument(const std::string& name, char short_name, const T& def,
           const std::function<T(const std::string&)>& converter);
  std::ostream& format_args(std::ostream& os) override;
  void parse(ArgReader<C>& reader) override;

  ~Argument() override{};

 public:
  // See Flag
  const T& get() const;
  // See Flag
  Argument& help(const std::string& new_help);
};
}

#include "cpparse.cxx"

#endif
