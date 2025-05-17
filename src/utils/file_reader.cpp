#include "utils/file_reader.h"

string read_file(string path)
{
  ifstream file(path);

  // if (!file.is_open())
  // {
  //   cout << "Program file " << path << " was not found." << endl;
  //   system("pause");
  //   exit(1);
  // }

  string program;
  string line;
  while (getline(file, line))
  {
    program += line + '\n';
  }

  return program;
}
