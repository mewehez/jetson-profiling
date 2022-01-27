#include <stdio.h>
#include "argparse.h"

#define POWER_USAGE_STRING  "Usage of power profiler: ./power [--output=OUTPUT] [--log] [--help]\n"\
                            "--output=OUTPUT        The path of the file in which to write the power consumption.\n"\
                            "--log                  Display values in the consol.\n"\
                            "--help                 Show the help message.\n\n"

#define COLOR_BLUE      "\033[0;34m"
#define COLOR_GREEN     "\033[0;32m"
#define COLOR_WHITE     "\033[1;37m"
#define COLOR_NONE      "\033[0m"

// log function
#define log(format, args...)   (if display == 1) printf(format, ## args)

void usage(void)
{
    printf(POWER_USAGE_STRING);
}

int display = 0;

int main(int argc, char** argv)
{
    return 0;
}