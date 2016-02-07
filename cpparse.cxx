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

#include "indent_header.hxx"

namespace cpparse {
// ----------------
// Usage Formatting
// ----------------

class Parser::UsageFormatter {
  friend std::ostream& operator<<(std::ostream& os,
                                  const UsageFormatter& usage) {
    return usage.format(os);
  }

  const Parser& parser;
  std::ostream& format(std::ostream& os) const {
    unsigned padding = parser.program_name.size() + 8;
    unsigned max_width = 80;
    os << "usage: " << parser.program_name;
    if (padding + 4 >= max_width) {
      padding = 24;
      os << '\n';
      for (unsigned i = 0; i < padding; i++) {
        os << ' ';
      }
    } else {
      os << ' ';
    }

    indent::Indenter out(os, padding, max_width, padding);
    std::ostringstream stringify;

    for (const auto& opt : parser.options) {
      stringify.clear();
      stringify.str("");

      stringify << '[';
      if (opt.second->short_name) {
        stringify << option_char << opt.second->short_name;
      } else {
        stringify << option_char << option_char << opt.second->name;
      }
      opt.second->format_args(stringify);
      stringify << ']';

      out << stringify.str();
    }

    for (const auto& arg : parser.arguments) {
      stringify.clear();
      stringify.str("");

      arg->format_args(stringify);

      out << stringify.str().substr(1);  // Must start with a space
    }
    return os << '\n';
  }

 public:
  UsageFormatter(const Parser& parser_) : parser(parser_) {}
};

// ---------------
// Help Formatting
// ---------------
class Parser::HelpFormatter {
  friend std::ostream& operator<<(std::ostream& os, const HelpFormatter& help) {
    return help.format(os);
  }

  const Parser& parser;
  const UsageFormatter usage;
  std::ostream& format(std::ostream& os) const {
    unsigned max_width = 80;
    unsigned padding = 24;
    std::istringstream words;
    std::string word;

    // Usage
    os << usage << '\n';

    // Description
    words.str(parser.description);
    words.clear();
    indent::Indenter desc(os, 0, max_width, 0);
    while (words >> word) {
      desc << word;
    }
    os << '\n';

    // Positional Arguments
    if (!parser.arguments.empty()) {
      os << "\nPositional Arguments:\n";

      for (const auto& arg : parser.arguments) {
        // Print out name
        std::ostringstream buffer;
        buffer << ' ';
        arg->format_args(buffer);
        os << buffer.str();

        if (arg->help_text.empty()) {
          // Don't add spaces if no help to render
          os << '\n';
          continue;
        }

        // Align help text
        unsigned length = buffer.tellp();
        if (length + 1 <= padding) {
          for (unsigned i = 0; i < (padding - length); i++) {
            os << ' ';
          }
        } else {
          os << '\n';
          for (unsigned i = 0; i < padding; i++) {
            os << ' ';
          }
        }

        // Print out help text
        indent::Indenter pos(os, padding, max_width, padding);
        words.str(arg->help_text);
        words.clear();
        while (words >> word) {
          pos << word;
        }
        os << '\n';
      }
    }

    // Optional Arguments
    if (!parser.options.empty()) {
      os << "\nOptional Arguments:\n";

      for (const auto& opt : parser.options) {
        // Print out name
        std::ostringstream buffer;
        buffer << "  ";
        if (opt.second->short_name) {
          buffer << option_char << opt.second->short_name;
          opt.second->format_args(buffer);
          buffer << ", ";
        }
        buffer << option_char << option_char << opt.second->name;
        opt.second->format_args(buffer);
        os << buffer.str();

        if (opt.second->help_text.empty()) {
          // Don't add spaces is no help to render
          os << '\n';
          continue;
        }

        // Align help text
        unsigned length = buffer.tellp();
        if (length + 1 <= padding) {
          for (unsigned i = 0; i < (padding - length); i++) {
            os << ' ';
          }
        } else {
          os << '\n';
          for (unsigned i = 0; i < padding; i++) {
            os << ' ';
          }
        }

        // Print out help text
        indent::Indenter pos(os, padding, max_width, padding);
        words.str(opt.second->help_text);
        words.clear();
        while (words >> word) {
          pos << word;
        }
        os << '\n';
      }
    }
    return os;
  }

 public:
  HelpFormatter(const Parser& parser_)
      : parser(parser_), usage(parser_.usage()) {}
};
// ---------------
// Argument Reader
// ---------------
// This is a class to make argument parsing a lot easier by abstracting away
// difference between long and short options as well as handling the -- marker
// for the end of arguments. This also provides convenience methods for
// throwing parsing errors.

// Option types for parsing
// The ArgReader will return these to indicate what type of option was parsed
enum class ot { end, argument, short_opt, long_opt, marker };

// Actual ArgReader
struct ArgReader {
  char** itr;
  char* const* end;
  bool process_options;
  std::string current;
  std::string::iterator location;  // Current location in `current`

  // To help with error throwing
  typename Parser::UsageFormatter usage;

  ArgReader(typename Parser::UsageFormatter usage_, int argc, char** argv)
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
    if (process_options && current.size() == 2 && current[0] == option_char &&
        current[1] == option_char) {
      // No mare args arg
      process_options = false;
      location = current.end();
      return ot::marker;

    } else if (process_options && current.size() >= 2 &&
               current[0] == option_char && current[1] != option_char) {
      // Short arg
      buffer.clear();
      location++;
      buffer.push_back(*location++);
      return ot::short_opt;
    }
    if (process_options && current.size() > 2 && current[0] == option_char &&
        current[1] == option_char) {
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
    if (process_options && current.size() >= 1 && current[0] == option_char) {
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

class HelpFlag : Option {
  friend class Parser;

  const Parser& parser;  // Need a reference to print help

  HelpFlag(const Parser& parser_) : Option("help", 'h'), parser(parser_) {
    this->help_text = "Show this help message and exit";
  }

  std::ostream& format_args(std::ostream& os) override { return os; }

  void parse(ArgReader& reader) override {
    (void)reader;
    std::cout << parser.help() << std::flush;
    exit(0);
  }

  ~HelpFlag() override{};
};

// ----------------
// Parser Functions
// ----------------

Parser::Parser(const std::string& description_, bool enable_help)
    : description(description_) {
  if (enable_help) {
    enroll_option(new HelpFlag(*this));
  }
}

// Parsing function works in tandem with ArgReader
void Parser::parse(int argc, char** argv) {
  if (argc > 0 && !program_name.size()) {  // Assign program name
    program_name = *argv;
  }

  ArgReader reader(usage(), argc, argv);
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
void Parser::enroll_option(Option* option) {
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
  options[option->name] = std::move(std::unique_ptr<Option>(option));
  if (option->short_name) {
    short_options[option->short_name] = option;
  }
}

// This is the method to add an argument agnostic to everything else
void Parser::enroll_argument(Option* argument) {
  arguments.push_back(std::move(std::unique_ptr<Option>(argument)));
}

// Get usage formatter
typename Parser::UsageFormatter Parser::usage() const {
  return Parser::UsageFormatter(*this);
}

// Get help formatter
typename Parser::HelpFormatter Parser::help() const {
  return Parser::HelpFormatter(*this);
}

// ------
// Option
// ------
// ABC for all actual options and arguments. Doesn't do anything other than
// allowing virtual calls and having a minimal interface.

Option::Option(const std::string& name_, char short_name_)
    : name(name_), short_name(short_name_) {}

Option::~Option() {}

std::ostream& Option::format_args(std::ostream& os) { return os; }

void Option::parse(ArgReader& reader) {
  (void)reader;  // ignore unused argument
}

// ----
// Flag
// ----
// An option with no arguments.
template <typename T>
Flag<T>& Parser::add_flag(const std::string& name, char short_name, T constant,
                          T def) {
  auto* flag = new Flag<T>(name, short_name, constant, def);
  enroll_option(flag);
  return *flag;
}

template <typename T>
Flag<T>& Parser::add_flag(const std::string& name, T constant, T def) {
  return Parser::add_flag(name, '\0', constant, def);
}

template <typename T>
Flag<T>::Flag(const std::string& name, char short_name, const T& constant_,
              const T& def)
    : Option(name, short_name), value(def), constant(constant_) {}

template <typename T>
std::ostream& Flag<T>::format_args(std::ostream& os) {
  return os;
}

template <typename T>
void Flag<T>::parse(ArgReader& reader) {
  (void)reader;  // hide unused warning
  value = constant;
}

template <typename T>
Flag<T>& Flag<T>::help(const std::string& new_string) {
  this->help_text.assign(new_string);
  return *this;
}

template <typename T>
const T& Flag<T>::get() const {
  return value;
}

// --------
// Argument
// --------
// An option or argument with one argument
template <typename T>
Argument<T>& Parser::add_optargument(
    const std::string& name, char short_name, T def,
    const std::function<T(const std::string&)> converter) {
  auto* optarg = new Argument<T>(name, short_name, def, converter);
  enroll_option(optarg);
  return *optarg;
}

template <typename T>
Argument<T>& Parser::add_optargument(
    const std::string& name, T def,
    const std::function<T(const std::string&)> converter) {
  return Parser::add_optargument(name, '\0', def, converter);
}

template <typename T>
Argument<T>& Parser::add_argument(
    const std::string& name,
    const std::function<T(const std::string&)> converter) {
  auto* arg = new Argument<T>(name, '\0', T(), converter);
  enroll_argument(arg);
  return *arg;
}

template <typename T>
Argument<T>::Argument(const std::string& name, char short_name, const T& def,
                      const std::function<T(const std::string&)>& converter_)
    : Option(name, short_name), value(def), converter(converter_) {}

template <typename T>
std::ostream& Argument<T>::format_args(std::ostream& os) {
  // One arg is required so surrounded with <>
  return os << " <" << this->name << '>';
}

template <typename T>
void Argument<T>::parse(ArgReader& reader) {
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

template <typename T>
Argument<T>& Argument<T>::help(const std::string& new_string) {
  this->help_text.assign(new_string);
  return *this;
}

template <typename T>
const T& Argument<T>::get() const {
  return value;
}

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
