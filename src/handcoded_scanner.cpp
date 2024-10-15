#include "handcoded_scanner.h"

HandCodedScanner::HandCodedScanner(string src) : ScannerBase(src)
{
}

Token HandCodedScanner::get_token()
{
    char ch;
    string s = "";
    return {"$", TokenType::END_OF_FILE};
}
