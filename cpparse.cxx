#include <memory>
#include <map>
#include <vector>
#include <string>

#include <stdexcept>
#include <typeinfo>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <cstdlib>

namespace cpparse {
// Option types for parsing
// The ArgReader will return these to indicate what type of option was parsed
enum class ot { end, argument, short_opt, long_opt, marker };

// ---------------
// Argument Reader
// ---------------
// This is a class to make argument parsing a lot easier by abstracting away
// difference between long and short options as well as handling the -- marker
// for the end of arguments. This also provides convenience methods for
// throwing parsing errors.
template <char C>
struct ArgReader {
  char** itr;
  char* const* end;
  bool process_options;
  std::string current;
  std::string::iterator location;  // Current location in `current`

  // To help with error throwing
  typename Parser<C>::UsageFormatter usage;

  ArgReader(typename Parser<C>::UsageFormatter usage_, int argc, char** argv)
      : itr(argv + 1),
        end(argv + argc),
        process_options(true),
        current(),
        location(current.begin()),
        usage(usage_) {}

  // Grabs the next `flag` from argv. If flag is an empty argument, then
  // location is reset so that next_argument does the right thing. Returns an
  // enum indicating the type of flag it grabbed.
  ot next_flag(std::string& buffer) {
    if (location != current.begin() && location != current.end()) {
      // Parse another short argument
      buffer.clear();
      buffer.push_back(*location++);
      return ot::short_opt;

    } else if (location == current.end() && itr == end) {
      // Nothing else to parse
      return ot::end;

    } else if (location == current.end()) {
      // Grab next argument
      current.assign(*itr++);
      location = current.begin();
    }
    if (process_options && current.size() == 2 && current[0] == C &&
        current[1] == C) {
      // No mare args arg
      process_options = false;
      location = current.end();
      return ot::marker;

    } else if (process_options && current.size() >= 2 && current[0] == C &&
               current[1] != C) {
      // Short arg
      buffer.clear();
      location++;
      buffer.push_back(*location++);
      return ot::short_opt;
    }
    if (process_options && current.size() > 2 && current[0] == C &&
        current[1] == C) {
      // Long option
      buffer.assign(location + 2, current.end());  // ignore dashes
      location = current.end();
      return ot::long_opt;
    } else {
      // Argument
      // keep location at beginning for argument parsing
      buffer.assign(current);
      return ot::argument;
    }
  }

  // Get the next argument. Fails if next argument can't be found (due to
  // having a flag next or being at the end)
  bool next_argument(std::string& buffer) {
    if (location != current.begin() && location != current.end()) {
      buffer.assign(location, current.end());
      location = current.end();
      return true;

    } else if (location == current.end() && itr == end) {
      // Nothing else to parse
      return false;

    } else if (location == current.end()) {
      // Grab next argument
      current.assign(*itr++);
      location = current.begin();
    }
    if (process_options && current.size() >= 1 && current[0] == C) {
      // Got an option, failed
      return false;

    } else {
      buffer.assign(current);
      location = current.end();
      return true;
    }
  }

  // Helpful error messages for parsing. These are here so that arguments can
  // print used e.g. get access to the parser.
  void print_usage_exit() {
    std::cerr << usage << std::flush;
    exit(1);
  }

  void option_not_found(const char* type, const std::string& option) {
    std::cerr << type << " option \"" << option << "\" is not a valid option\n";
    print_usage_exit();
  }

  void too_many_args(const std::string& argument) {
    std::cerr << "Argument \"" << argument
              << "\" specified, but program demands no more arguments\n";
    print_usage_exit();
  }

  template <typename T>
  void parse_error(const std::string& name, const std::string& argument) {
    std::cerr << "Parse error trying to interpret '" << name << "' argument \""
              << argument << "\" as an '" << typeid(T).name() << "'\n";
    print_usage_exit();
  }

  void required_argument(const std::string& name) {
    std::cerr << '\'' << name
              << "' requires an argument, but none was specified\n";
    print_usage_exit();
  }
};

// -----------
// Help Option
// -----------

template <char C>
class HelpFlag : Option<C> {
  friend class Parser<C>;

  const Parser<C>& parser;

  HelpFlag(const Parser<C>& parser_) : Option<C>("help", 'h'), parser(parser_) {
    this->help_text = "Show this help message and exit";
  }

  std::ostream& format_args(std::ostream& os) override { return os; }

  void parse(ArgReader<C>& reader) override {
    (void)reader;
    std::cout << parser.help() << std::flush;
    exit(0);
  }

  ~HelpFlag() override{};
};

// ----------------
// Parser Functions
// ----------------

template <char C>
Parser<C>::Parser(const std::string& description_, bool enable_help)
    : description(description_) {
  if (enable_help) {
    enroll_option(new HelpFlag<C>(*this));
  }
}

// Parsing function works in tandem with ArgReader
template <char C>
void Parser<C>::parse(int argc, char** argv) {
  if (argc > 0 && !program_name.size()) {  // Assign program name
    program_name = *argv;
  }

  ArgReader<C> reader(usage(), argc, argv);
  auto args = arguments.begin();
  std::string flag;
  ot type;
  while ((type = reader.next_flag(flag)) != ot::end) {
    switch (type) {
      case ot::short_opt: {
        auto option = short_options.find(flag[0]);
        if (option == short_options.end()) {
          reader.option_not_found("Short", flag);
        } else {
          option->second->parse(reader);
        }
        break;
      }
      case ot::long_opt: {
        auto option = options.find(flag);
        if (option == options.end()) {
          reader.option_not_found("Long", flag);
        } else {
          option->second->parse(reader);
        }
        break;
      }
      case ot::argument: {
        if (args == arguments.end()) {
          reader.too_many_args(flag);
        }
        (*args)->parse(reader);
        args++;
        break;
      }
      case ot::marker: {
        break;
      }
      case ot::end: {
        throw std::logic_error("Should never reach here");
      }
    }
  }
  while (args != arguments.end()) {
    (*args)->parse(reader);
    args++;
  }
}

// This is the method to add an option agnostic to everything else
template <char C>
void Parser<C>::enroll_option(Option<C>* option) {
  if (options.find(option->name) != options.end()) {
    throw std::invalid_argument(
        std::string("Can't add two options with the same name: \"") +
        option->name + '"');
  }
  if (option->short_name &&
      short_options.find(option->short_name) != short_options.end()) {
    throw std::invalid_argument(
        std::string("Can't add two options with the same short name: '") +
        option->short_name + '\'');
  }
  options[option->name] = std::move(std::unique_ptr<Option<C>>(option));
  if (option->short_name) {
    short_options[option->short_name] = option;
  }
}

// This is the method to add an argument agnostic to everything else
template <char C>
void Parser<C>::enroll_argument(Option<C>* argument) {
  arguments.push_back(std::move(std::unique_ptr<Option<C>>(argument)));
}

// Get usage formatter
template <char C>
typename Parser<C>::UsageFormatter Parser<C>::usage() const {
  return Parser<C>::UsageFormatter(*this);
}

// Get help formatter
template <char C>
typename Parser<C>::HelpFormatter Parser<C>::help() const {
  return Parser<C>::HelpFormatter(*this);
}

// ------
// Option
// ------
// ABC for all actual options and arguments. Doesn't do anything other than
// allowing virtual calls and having an interface.
template <char C>
class Option {
 public:
  const std::string name;
  const char short_name;  // nonexistent if 0
  std::string help_text;

  Option(const std::string& name_, char short_name_)
      : name(name_), short_name(short_name_) {}
  virtual ~Option(){};
  virtual std::ostream& format_args(std::ostream& os) { return os; }
  virtual void parse(ArgReader<C>& reader) {
    (void)reader;  // ignore unused argument
  }
};

// ----
// Flag
// ----
// An option with no arguments.
template <char C>
template <typename T>
Flag<C, T>& Parser<C>::add_flag(const std::string& name, char short_name,
                                T constant, T def) {
  auto* flag = new Flag<C, T>(name, short_name, constant, def);
  enroll_option(flag);
  return *flag;
}

template <char C>
template <typename T>
Flag<C, T>& Parser<C>::add_flag(const std::string& name, T constant, T def) {
  return Parser<C>::add_flag(name, '\0', constant, def);
}

template <char C, typename T>
Flag<C, T>::Flag(const std::string& name, char short_name, const T& constant_,
                 const T& def)
    : Option<C>(name, short_name), value(def), constant(constant_) {}

template <char C, typename T>
std::ostream& Flag<C, T>::format_args(std::ostream& os) {
  return os;
}

template <char C, typename T>
void Flag<C, T>::parse(ArgReader<C>& reader) {
  (void)reader;  // hide unused warning
  value = constant;
}

template <char C, typename T>
Flag<C, T>& Flag<C, T>::help(const std::string& new_string) {
  this->help_text.assign(new_string);
  return *this;
}

template <char C, typename T>
const T& Flag<C, T>::get() const {
  return value;
}

// --------
// Argument
// --------
// An option or argument with one argument
template <char C>
template <typename T>
Argument<C, T>& Parser<C>::add_optargument(
    const std::string& name, char short_name, T def,
    const std::function<T(const std::string&)> converter) {
  auto* optarg = new Argument<C, T>(name, short_name, def, converter);
  enroll_option(optarg);
  return *optarg;
}

template <char C>
template <typename T>
Argument<C, T>& Parser<C>::add_optargument(
    const std::string& name, T def,
    const std::function<T(const std::string&)> converter) {
  return Parser<C>::add_optargument(name, '\0', def, converter);
}

template <char C>
template <typename T>
Argument<C, T>& Parser<C>::add_argument(
    const std::string& name,
    const std::function<T(const std::string&)> converter) {
  auto* arg = new Argument<C, T>(name, '\0', T(), converter);
  enroll_argument(arg);
  return *arg;
}

template <char C, typename T>
Argument<C, T>::Argument(const std::string& name, char short_name, const T& def,
                         const std::function<T(const std::string&)>& converter_)
    : Option<C>(name, short_name), value(def), converter(converter_) {}

template <char C, typename T>
std::ostream& Argument<C, T>::format_args(std::ostream& os) {
  // One arg is required so surrounded with <>
  return os << " <" << this->name << '>';
}

template <char C, typename T>
void Argument<C, T>::parse(ArgReader<C>& reader) {
  std::string buffer;
  if (reader.next_argument(buffer)) {
    try {
      value = converter(buffer);
    } catch (std::invalid_argument) {
      reader.template parse_error<T>(this->name, buffer);
    }
  } else {
    reader.required_argument(this->name);
  }
}

template <char C, typename T>
Argument<C, T>& Argument<C, T>::help(const std::string& new_string) {
  this->help_text.assign(new_string);
  return *this;
}

template <char C, typename T>
const T& Argument<C, T>::get() const {
  return value;
}

// ----------------
// Usage Formatting
// ----------------

template <char C>
class Parser<C>::UsageFormatter {
  friend std::ostream& operator<<(std::ostream& os,
                                  const UsageFormatter& usage) {
    return usage.format(os);
  }

  const Parser<C>& parser;
  std::ostream& format(std::ostream& os) const {
    os << "usage: " << parser.program_name;
    for (const auto& opt : parser.options) {
      os << " [";
      if (opt.second->short_name) {
        os << C << opt.second->short_name;
      } else {
        os << C << C << opt.second->name;
      }
      opt.second->format_args(os);
      os << ']';
    }
    for (const auto& arg : parser.arguments) {
      arg->format_args(os);
    }
    return os << '\n';
  }

 public:
  UsageFormatter(const Parser<C>& parser_) : parser(parser_) {}
};
// ---------------
// Help Formatting
// ---------------

template <char C>
class Parser<C>::HelpFormatter {
  friend std::ostream& operator<<(std::ostream& os, const HelpFormatter& help) {
    return help.format(os);
  }

  const Parser<C>& parser;
  const UsageFormatter usage;
  std::ostream& format(std::ostream& os) const {
    os << usage << '\n' << parser.description << '\n';
    if (!parser.arguments.empty()) {
      os << "\nPositional Arguments:\n";
      for (const auto& arg : parser.arguments) {
        os << " ";
        arg->format_args(os);
        os << "    " << arg->help_text << '\n';
      }
    }
    if (!parser.options.empty()) {
      os << "\nOptional Arguments:\n";
      for (const auto& opt : parser.options) {
        os << "  ";
        if (opt.second->short_name) {
          os << C << opt.second->short_name;
          opt.second->format_args(os);
          os << ", ";
        }
        os << C << C << opt.second->name;
        opt.second->format_args(os);
        os << "    " << opt.second->help_text << '\n';
      }
    }
    return os;
  }

 public:
  HelpFormatter(const Parser<C>& parser_)
      : parser(parser_), usage(parser_.usage()) {}
};

// ---------------------
// String Interpretation
// ---------------------
// Implementations of default string interpretations
template <typename T>
T read(const std::string& input) {
  std::istringstream iss(input);
  T result;
  iss >> std::boolalpha >> result;  // Read "true" as true
  if (iss.eof() || iss.tellg() == int(input.size())) {
    return result;
  } else {
    throw std::invalid_argument("Couldn't parse string");
  }
}

// Needs to be specialized so that it takes the whole string and not only
// whitespace
template <>
std::string read<std::string>(const std::string& input) {
  return input;
}
}
