#include "main.h"

#include <iostream>
using std::cout;
using std::endl;
#include <fstream>
#include <string>
#include <boost/filesystem.hpp>
using boost::filesystem::absolute;

#ifdef UNIT_TESTS
#define MAIN not_main
#else
#define MAIN main
#endif

int MAIN(int argc, char** argv) {

  return EXIT_SUCCESS;
}

