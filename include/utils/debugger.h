#pragma once

#include <iostream>
#include <fstream>
#include <string>

#include "robin/compiler.h"
#include "utils/file_reader.h"

using namespace std;

class Debugger
{
public:
  static int run();

private:
  static string DEBUGGING_FOLDER;
  static string PROGRAM_FILE;
};
