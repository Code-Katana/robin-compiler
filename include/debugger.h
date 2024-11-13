#pragma once

#include <iostream>

#include "wren_compiler.h"
#include "json.h"

using namespace std;

class Debugger
{
public:
  static int run();
  static string DEBUGGING_FOLDER;
};
