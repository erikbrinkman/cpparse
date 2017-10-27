#ifndef CPPARSE_HXX
#define CPPARSE_HXX

#include <memory>
#include <map>
#include <vector>
#include <string>
#include <functional>
#include <initializer_list>

namespace cpparse {

// Default conversion of strings to types
template <class T>
T read(const std::string& input);

// Option classes and supporting classes
// Option is an ABC that allows easy storage of all types
class Option;
template <class T>
class Flag;
template <class T>
class SingleOption;

class ArgReader;

// Parser object
// This controls all of the parsing, and is the main point of api entry
class Parser {
 public:
  // Classes that overload << to allow easy formatting of help and usage in
  // streams
  class UsageFormatter;
  class HelpFormatter;

  Parser& description(const std::string& new_description);

  void parse(int argc, char** argv);
  UsageFormatter usage() const;
  HelpFormatter help() const;

  // Add a flag (no arguments) with a short name
  template <typename T = bool>
  Flag<T>& add_flag(const std::initializer_list<std::string>& names,
                    const T& constant, const T& def = T(),
                    const std::function<T(const T&, const T&)>& aggregator =
                        [](const T& a, const T& b) -> T { return b });

  template <typename T = std::string>
  SingleOption<T>& add_option(
      const std::initializer_list<std::string>& names, const T& def = T(),
      const std::function<T(const std::string&)>& converter = read<T>);

 private:
  std::vector<std::unique_ptr<Option>> options;
  std::map<std::string, Option*> long_options;
  std::map<char, Option*> short_options;
  std::vector<std::unique_ptr<Option>> arguments;

  std::string program_name;
  std::string description_text;
};

// Option
class Option {
 public:
  virtual void parse(ArgReader&) = 0;
  virtual std::string short_usage() = 0;
  virtual std::string long_usage() = 0;
  virtual std::string help() = 0;
};

// Flag (no arguments)
template <typename T>
class Flag : public Option {
 public:
  const T& get() const;
  Flag& help(const std::string& new_help);

  // Non-copyable
  Flag& operator=(const Flag& copy) = delete;
  Flag(const Flag& copy) = delete;

  // Used by parser
  Flag(const std::vector<char>& short_names,
       const std::vector<std::string>& long_names, const T& constant,
       const T& def, const std::function<T(const T&, const T&)>& aggregator);

  void parse(ArgReader& reader) override;
  std::string short_usage() override;
  std::string long_usage() override;
  std::string help() override;

 private:
  T value;
  T constant;
  std::function<T(const T&, const T&)> aggregator;
  std::string short_usage_text;
  std::string long_usage_text;
  std::string help_text;
};

// SingleOption (one argument)
template <typename T>
class SingleOption : public Option {
 public:
  const T& get() const;
  SingleOption& help(const std::string& new_help);
  SingleOption& meta_var(const std::string& meta_var);

  // Non-copyable
  SingleOption& operator=(const SingleOption&) = delete;
  SingleOption(const SingleOption&) = delete;

  // Used by parser
  SingleOption(const std::vector<char>& short_names,
               const std::vector<std::string>& long_names, const T& def,
               const std::function<T(const std::string&)>& converter);

  void parse(ArgReader& reader) override;
  std::string short_usage() override;
  std::string long_usage() override;
  std::string help() override;

 private:
  T value;
  const std::function<T(const std::string&)> converter;
  std::string meta_var_text;
  std::string help_text;
  const std::vector<char> short_names;
  const std::vector<std::string> long_names;
};

/*
// Argument (one argument)
template <typename T>
class Argument : Option {
  friend class Parser;
  T value;
  const std::function<T(const std::string&)> converter;

  Argument(const std::string& name, char short_name, const T& def,
           const std::function<T(const std::string&)>& converter);
  std::ostream& format_args(std::ostream& os) override;
  void parse(ArgReader& reader) override;

  ~Argument() override{};

 public:
  // See Flag
  const T& get() const;
  // See Flag
  Argument& help(const std::string& new_help);

  // Non-copyable
  Argument& operator=(const Argument& copy) = delete;
  Argument(const Argument& copy) = delete;
};

// Arguments (variable number of arguments)
template <typename T>
class Arguments : Option {
  friend class Parser;
  std::vector<T> values;
  const std::function<T(const std::string&)> converter;

  Arguments(const std::string& name, char short_name, unsigned min, unsigned
max,
           const std::function<T(const std::string&)>& converter);
  std::ostream& format_args(std::ostream& os) override;
  void parse(ArgReader& reader) override;

  ~Arguments() override{};

 public:
  // See Flag
  const T& get() const;
  // See Flag
  Arguments& help(const std::string& new_help);

  // Non-copyable
  Arguments& operator=(const Arguments& copy) = delete;
  Arguments(const Arguments& copy) = delete;
};
*/

class Parser::UsageFormatter {
 public:
  UsageFormatter(const Parser&);

 private:
  const Parser& parser;

  friend std::ostream& operator<<(std::ostream& os,
                                  const UsageFormatter& usage);

  std::ostream& format(std::ostream& os) const;
};

class Parser::HelpFormatter {
 public:
  HelpFormatter(const Parser&);

 private:
  const Parser& parser;

  friend std::ostream& operator<<(std::ostream& os, const HelpFormatter& usage);

  std::ostream& format(std::ostream& os) const;
};
}

#include "cpparse.cxx"

#endif
