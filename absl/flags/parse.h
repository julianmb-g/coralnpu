#pragma once
#include <vector>
namespace absl { inline std::vector<char*> ParseCommandLine(int argc, char** argv) { return std::vector<char*>(argv, argv + argc); } }
