#pragma once

#include "types.hpp"

namespace dppbot {

CliOptions ParseCli(int argc, char** argv);
void PrintUsage();

}  // namespace dppbot
