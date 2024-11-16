#pragma once

#include <iostream>
#include <fstream>
#include <string>

#include "wren_compiler.h"
#include "json.h"

using namespace std;

class Debugger
{
public:
  static int run();

private:
  static string DEBUGGING_FOLDER;
  static string PROGRAM_FILE;
};
