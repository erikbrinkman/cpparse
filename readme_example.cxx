#include <iostream>
#include <string>
#include "cpparse.hxx"

using namespace std;
using namespace cpparse;

int main(int argc, char** argv) {
  Parser parser;
  parser.description("An example parser.");
  auto& all = parser.add_flag({"-a", "--all", "--alias"}, true);
  all.help("Sets the all flag to true.");
  auto& verbose = parser.add_agg_flag({"-v", "--verbose"}, 1);
  verbose.help("Sets verbosity. Set multiple times for more verbosity.");
  auto& integer = parser.add_option<int>({"-i", "--integer"}).meta_var("int");
  parser.parse(argc, argv);

  cout << parser.help();
  cout << all.get() << ' ' << verbose.get() << ' ' << integer.get() << '\n';

  cout << flush;
}
