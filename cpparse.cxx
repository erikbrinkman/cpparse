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
#include <cctype>

#include "indent_header.hxx"

namespace cpparse {

// ----------
// Formatting
// ----------

unsigned get_max_width() {
  try {
    return std::stoul(std::getenv("COLUMNS"));
  } catch (const std::exception& e) {
    return 80;  // default
  }
}

Parser::UsageFormatter::UsageFormatter(const Parser& parser_)
    : parser(parser_) {}

std::ostream& operator<<(std::ostream& os,
                         const Parser::UsageFormatter& usage) {
  return usage.format(os);
}

// This is defined separately so that is has private access to Parser
std::ostream& Parser::UsageFormatter::format(std::ostream& os) const {
  unsigned padding = parser.program_name.size() + 8;
  unsigned max_width = get_max_width();

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
  for (const auto& opt : parser.options) {
    out << opt->short_usage();
  }
  for (const auto& arg : parser.arguments) {
    (void)arg;
  }
  return os << '\n';
}

std::ostream& operator<<(std::ostream& os, const Parser::HelpFormatter& help) {
  return help.format(os);
}

std::ostream& Parser::HelpFormatter::format(std::ostream& os) const {
  unsigned max_width = get_max_width();
  unsigned padding = 24;
  std::istringstream words;
  std::string word;

  // Usage
  os << parser.usage();

  if (!parser.description_text.empty()) {
    // Description
    words.str(parser.description_text);
    words.clear();
    indent::Indenter desc(os, 0, max_width, 0);
    os << '\n';
    while (words >> word) {
      desc << word;
    }
    os << '\n';
  }

  // Positional Arguments
  if (!parser.arguments.empty()) {
    os << "\nPositional Arguments:\n";

    for (const auto& arg : parser.arguments) {
      (void)arg;
      /*
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
      */
    }
  }

  // Optional Arguments
  if (!parser.options.empty()) {
    os << "\nOptional Arguments:\n";

    for (const auto& opt : parser.options) {
      std::string long_usage = opt->long_usage();
      std::string help = opt->help();

      os << "  " << long_usage;
      if (help.empty()) {  // Don't add spaces if no help
        os << '\n';
        continue;
      }

      // Align help text
      unsigned length = 2 + long_usage.length();
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
      words.str(help);
      words.clear();
      while (words >> word) {
        pos << word;
      }
      os << '\n';
    }
  }
  return os;
}

Parser::HelpFormatter::HelpFormatter(const Parser& parser_) : parser(parser_) {}

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
class ArgReader {
  // FIXME Parse long options with = syntax. Make sure to throw error on next
  // flag if equals bit is set.
 public:
  ArgReader(typename Parser::UsageFormatter usage_, int argc, char** argv)
      : itr(argv + 1),
        end(argv + argc),
        process_options(true),
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
      last_flag = "-";
      last_flag.append(buffer);
      return ot::short_opt;

    } else if (location == current.end() && itr == end) {
      // Nothing else to parse
      return ot::end;

    } else if (location == current.end()) {
      // Grab next argument
      current.assign(*itr++);
      location = current.begin();
    }
    if (process_options && current.size() == 2 && current[0] == '-' &&
        current[1] == '-') {
      // No mare args arg
      process_options = false;
      location = current.end();
      return ot::marker;

    } else if (process_options && current.size() >= 2 && current[0] == '-' &&
               current[1] != '-') {
      // Short arg
      buffer.clear();
      location++;
      buffer.push_back(*location++);
      last_flag = "-";
      last_flag.append(buffer);
      return ot::short_opt;
    }
    if (process_options && current.size() > 2 && current[0] == '-' &&
        current[1] == '-') {
      // Long option
      buffer.assign(location + 2, current.end());  // ignore dashes
      last_flag = "--";
      last_flag.append(buffer);
      location = current.end();
      return ot::long_opt;
    } else {
      // Argument
      // keep location at beginning for argument parsing
      buffer.assign(current);
      last_flag.clear();
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
    if (process_options && current.size() >= 1 && current[0] == '-') {
      // Got an option, failed
      return false;

    } else {
      buffer.assign(current);
      location = current.end();
      return true;
    }
  }

  template <class T>
  void parse_error(const std::string& argument) {
    std::cerr << "Parse error trying to interpret argument of \"" << last_flag
              << "\" \"" << argument << "\" as an \"" << typeid(T).name()
              << "\"\n" << usage << std::flush;
    exit(1);
  }

  void required_argument_error() {
    std::cerr << '"' << last_flag
              << "\" requires an argument, but none was specified\n" << usage
              << std::flush;
    exit(1);
  }

 private:
  char** itr;
  char* const* end;
  bool process_options;
  std::string last_flag;
  std::string current;
  std::string::iterator location;  // location in current
  Parser::UsageFormatter usage;
};

// -----------
// Help Option
// -----------

/*
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
*/

// ----------------
// Parser Functions
// ----------------

Parser& Parser::description(const std::string& new_description) {
  description_text = new_description;
  return *this;
}

// Parsing function works in tandem with ArgReader
void Parser::parse(int argc, char** argv) {
  if (argc > 0 && program_name.empty()) {  // Assign program name
    program_name = *argv;
  }

  ArgReader reader(usage(), argc, argv);
  auto args = arguments.begin();
  std::string flag;
  ot type;
  while ((type = reader.next_flag(flag)) != ot::end) {
    switch (type) {
      case ot::short_opt: {  // Short option
        auto option = short_options.find(flag[0]);
        if (option == short_options.end()) {
          std::cerr << "Short option \"" << flag[0]
                    << "\" is not a valid option\n" << usage() << std::flush;
          exit(1);
        } else {
          option->second->parse(reader);
        }
        break;
      }
      case ot::long_opt: {  // Long option
        auto option = long_options.find(flag);
        if (option == long_options.end()) {
          std::cerr << "Long option \"" << flag[0]
                    << "\" is not a valid option\n" << usage() << std::flush;
          exit(1);
        } else {
          option->second->parse(reader);
        }
        break;
      }
      case ot::argument: {  // Positional argument
        if (args == arguments.end()) {
          std::cerr << "Argument \"" << flag
                    << "\" specified, but program demands no more arguments\n"
                    << usage() << std::flush;
          exit(1);
        }
        (*args++)->parse(reader);
        break;
      }
      case ot::marker: {  // Market - stop parsing options
        break;
      }
      case ot::end: {
        throw std::logic_error("Should never reach here");
      }
    }
  }
  while (args != arguments.end()) {
    (*args++)->parse(reader);
  }
}

/*
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
*/

// Get usage formatter
typename Parser::UsageFormatter Parser::usage() const {
  return Parser::UsageFormatter(*this);
}

// Get help formatter
typename Parser::HelpFormatter Parser::help() const {
  return Parser::HelpFormatter(*this);
}

// =======
// Options
// =======
bool is_short_option(const std::string& name) {
  return name.size() == 2 && name[0] == '-' && std::isalnum(name[1]);
}

bool is_long_option(const std::string& name) {
  return name.size() > 2 && name[0] == '-' && name[1] == '-' &&
         std::isalnum(name[2]) &&
         std::all_of(name.begin() + 3, name.end(), [](char c) -> bool {
           return std::isalnum(c) || c == '-';
         });
}

void parse_names(const std::initializer_list<std::string>& names,
                 std::vector<char>& short_names,
                 std::vector<std::string>& long_names) {
  if (!names.size()) {
    throw std::invalid_argument("Option must have at least one name");
  }
  for (const std::string& name : names) {
    if (is_short_option(name)) {
      short_names.push_back(name[1]);
    } else if (is_long_option(name)) {
      long_names.push_back(std::move(name.substr(2)));
    } else {
      std::ostringstream oss;
      oss << "Option names must be short \"-s\" or long \"--long\". \"" << name
          << "\" is neither.\n";
      throw std::invalid_argument(oss.str());
    }
  }
}

void register_option(Option* option, const std::vector<char> short_names,
                     const std::vector<std::string> long_names,
                     std::vector<std::unique_ptr<Option>>& options,
                     std::map<char, Option*>& short_options,
                     std::map<std::string, Option*>& long_options) {
  options.emplace_back(option);
  for (const char name : short_names) {
    if (short_options.find(name) != short_options.end()) {
      std::ostringstream oss;
      oss << "Can't add two options with the same short name: \"-" << name
          << '"';
      throw std::invalid_argument(oss.str());
    } else {
      short_options[name] = option;
    }
  }
  for (const std::string& name : long_names) {
    if (long_options.find(name) != long_options.end()) {
      std::ostringstream oss;
      oss << "Can't add two options with the same long name: \"--" << name
          << '"';
      throw std::invalid_argument(oss.str());
    } else {
      long_options[name] = option;
    }
  }
}

// ----------------------------------
// Flag : An option with no arguments
// ----------------------------------

template <class T>
Flag<T>& Parser::add_flag(
    const std::initializer_list<std::string>& names, const T& constant,
    const T& def, const std::function<T(const T&, const T&)>& aggregator) {
  std::vector<char> short_names;
  std::vector<std::string> long_names;
  parse_names(names, short_names, long_names);

  Flag<T>* flag = new Flag<T>(short_names, long_names, constant, def);
  register_option(reinterpret_cast<Option*>(flag), short_names, long_names,
                  options, short_options, long_options);
  return *flag;
}

template <class T>
Flag<T>::Flag(const std::vector<char>& short_names,
              const std::vector<std::string>& long_names, const T& constant_,
              const T& def)
    : value(def), constant(constant_) {
  short_usage_text.append("[-");
  if (!short_names.empty()) {
    short_usage_text.push_back(short_names[0]);
  } else {
    short_usage_text.push_back('-');
    short_usage_text.append(long_names[0]);
  }
  short_usage_text.push_back(']');

  bool first = true;
  for (const char name : short_names) {
    if (!first) {
      long_usage_text.append(", ");
    }
    long_usage_text.push_back('-');
    long_usage_text.push_back(name);
    first = false;
  }
  for (const std::string& name : long_names) {
    if (!first) {
      long_usage_text.append(", ");
    }
    long_usage_text.append("--").append(name);
    first = false;
  }
}

template <class T>
void Flag<T>::parse(ArgReader& reader) {
  (void)reader;  // unused
  value = constant;
}

template <class T>
std::string Flag<T>::short_usage() {
  return short_usage_text;
}

template <class T>
std::string Flag<T>::long_usage() {
  return long_usage_text;
}

template <class T>
std::string Flag<T>::help() {
  return help_text;
}

template <class T>
Flag<T>& Flag<T>::help(const std::string& new_string) {
  help_text = new_string;
  return *this;
}

template <class T>
const T& Flag<T>::get() const {
  return value;
}

// ------------------------------------------------------------------
// AggFlag : An option with no arguments, multiple calls have meaning
// ------------------------------------------------------------------

template <class T>
AggFlag<T>& Parser::add_agg_flag(
    const std::initializer_list<std::string>& names, const T& constant,
  std::vector<char> short_names;
  std::vector<std::string> long_names;
  parse_names(names, short_names, long_names);

  AggFlag<T>* flag =
      new AggFlag<T>(short_names, long_names, constant, def, aggregator);
  register_option(reinterpret_cast<Option*>(flag), short_names, long_names,
                  options, short_options, long_options);
  return *flag;
}

template <class T>
AggFlag<T>::AggFlag(const std::vector<char>& short_names,
                    const std::vector<std::string>& long_names,
                    const T& constant_, const T& def,
                    const std::function<T(const T&, const T&)>& aggregator_)
    : value(def), constant(constant_), aggregator(aggregator_) {
  short_usage_text.append("[-");
  if (!short_names.empty()) {
    short_usage_text.push_back(short_names[0]);
  } else {
    short_usage_text.push_back('-');
    short_usage_text.append(long_names[0]);
  }
  short_usage_text.append("]...");

  bool first = true;
  for (const char name : short_names) {
    if (!first) {
      long_usage_text.append(", ");
    }
    long_usage_text.push_back('-');
    long_usage_text.push_back(name);
    first = false;
  }
  for (const std::string& name : long_names) {
    if (!first) {
      long_usage_text.append(", ");
    }
    long_usage_text.append("--").append(name);
    first = false;
  }
}

template <class T>
void AggFlag<T>::parse(ArgReader& reader) {
  (void)reader;  // unused
  value = aggregator(value, constant);
}

template <class T>
std::string AggFlag<T>::short_usage() {
  return short_usage_text;
}

template <class T>
std::string AggFlag<T>::long_usage() {
  return long_usage_text;
}

template <class T>
std::string AggFlag<T>::help() {
  return help_text;
}

template <class T>
AggFlag<T>& AggFlag<T>::help(const std::string& new_string) {
  help_text = new_string;
  return *this;
}

template <class T>
const T& AggFlag<T>::get() const {
  return value;
}

// ------------------------------------------
// SingleOption : An option with one argument
// ------------------------------------------

template <typename T>
SingleOption<T>& Parser::add_option(
    const std::initializer_list<std::string>& names, const T& def,
    const std::function<T(const std::string&)>& converter) {
  std::vector<char> short_names;
  std::vector<std::string> long_names;
  parse_names(names, short_names, long_names);

  SingleOption<T>* option =
      new SingleOption<T>(short_names, long_names, def, converter);
  register_option(reinterpret_cast<Option*>(option), short_names, long_names,
                  options, short_options, long_options);

  return *option;
}

template <typename T>
const T& SingleOption<T>::get() const {
  return value;
}
template <typename T>
SingleOption<T>& SingleOption<T>::help(const std::string& new_help) {
  help_text = new_help;
  return *this;
}
template <typename T>
SingleOption<T>& SingleOption<T>::meta_var(const std::string& meta_var_) {
  meta_var_text = meta_var_;
  return *this;
}

template <typename T>
SingleOption<T>::SingleOption(
    const std::vector<char>& short_names_,
    const std::vector<std::string>& long_names_, const T& def,
    const std::function<T(const std::string&)>& converter_)
    : value(def),
      converter(converter_),
      short_names(short_names_),
      long_names(long_names_) {
  if (!long_names.empty()) {
    meta_var_text = long_names[0];
  } else {
    meta_var_text = short_names[0];
  }
}

template <typename T>
void SingleOption<T>::parse(ArgReader& reader) {
  std::string buffer;
  if (reader.next_argument(buffer)) {
    try {
      value = converter(buffer);
    } catch (std::invalid_argument) {
      reader.parse_error<T>(buffer);
    }
  } else {
    reader.required_argument_error();
  }
}

template <typename T>
std::string SingleOption<T>::short_usage() {
  std::string usage("[-");
  if (!short_names.empty()) {
    usage.push_back(short_names[0]);
  } else {
    usage.push_back('-');
    usage.append(long_names[0]);
  }
  usage.append(" <").append(meta_var_text).append(">]");
  return usage;
}

template <typename T>
std::string SingleOption<T>::long_usage() {
  std::string usage;
  bool first = true;
  for (const char name : short_names) {
    if (!first) {
      usage.append(", ");
    }
    usage.push_back('-');
    usage.push_back(name);
    usage.append(" <").append(meta_var_text).push_back('>');
    first = false;
  }
  for (const std::string& name : long_names) {
    if (!first) {
      usage.append(", ");
    }
    usage.append("--")
        .append(name)
        .append(" <")
        .append(meta_var_text)
        .push_back('>');
    first = false;
  }
  return usage;
}

template <typename T>
std::string SingleOption<T>::help() {
  return help_text;
}

/*
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
*/

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

// FIXME Add File Specialization / maybe someway to return cin cout?
}
