#pragma once
#include "scanner_base.h"
#include <string>
#include <iostream>
using namespace std;

class HandCodedScanner : ScannerBase {
public:
    HandCodedScanner(string src);
    Token get_token();
    void display_tokens();
};