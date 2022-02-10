#include <iostream>
#include <fstream>
#include <signal.h>
#include <strings.h>
#include <jetson-utils/timespec.h>

#include <profiling/argparse.h>
#include "power_profiling.h"

#define POWER_USAGE_STRING  "Usage of power profiler: \n"\
                            "./power_profiler [--output=OUTPUT] [--device=DEVICE] [--value=VALUE] [--help]\n"\
                            "Arguments: \n"\
                            "--output | -o            The path of the file in which to write the power consumption. Defaults to power_output.csv.\n"\
                            "--rail   | -r            Chose the rail to monitor. Use CPU, GPU or BOARD.\n"\
                            "--value  | -v            Chose the value to watch. One of POWER, CURRENT, VOLTAGE or ALL. Defaults to ALL.\n"\
                            "--help   | -h            Show the help message.\n\n"

#define usage() printf(POWER_USAGE_STRING)

bool shutdownFlag = false;
void sigintHandler(int sig)
{
  printf("\nCaught SIGINT!\n");
  shutdownFlag = true;
}

int getRailId(char* railType)
{
  if(!railType)
    return -1;
  if(strcasecmp(railType, "cpu") == 0)
    return CPU_RAIL;
  if(strcasecmp(railType, "gpu") == 0)
    return GPU_RAIL;
  if(strcasecmp(railType, "board") == 0)
    return BOARD_RAIL;
  return -1;
}

int getValueId(char* valueType)
{
  if(!valueType)
    return ALL_VALUE;
  if(strcasecmp(valueType, "current") == 0)
    return CURRENT_VALUE; 
  if(strcasecmp(valueType, "voltage") == 0)
    return VOLTAGE_VALUE;
  if(strcasecmp(valueType, "power") == 0)
    return POWER_VALUE;
  
  // by default we watch all the values
  return ALL_VALUE;
}


int main (int argc, char** argv) {
  if (signal(SIGINT, sigintHandler) == SIG_ERR)
  {
    printf(ERROR " Signal SIGINT error\n");
    exit(EXIT_FAILURE);
  }

  arg_option options[] = {
    OPT_BOOLEAN('h', "help",   NULL),
    OPT_STRING ('o', "output", NULL),
    OPT_STRING ('v', "value", NULL),
    OPT_STRING ('r', "rail",   NULL),
  };

  command_line cmd = { options, 4 };
  parse_command_line(&cmd, argc, argv);

  void* value = get_option_value(&cmd, "help");
  if(value)
  {
    // free dynamically allocated values
    free_command_line(&cmd);
    usage();
    exit(EXIT_SUCCESS);
  }
  
  char* railType = (char*) get_option_value(&cmd, "rail");
  int railId = getRailId(railType);

  if(railId == -1)
  {
    printf("Unexpected rail type %s.\n", railType);
    puts("Use one of CPU, GPU or BOARD. Type -h for more help.");
    // free dynamically allocated values
    free_command_line(&cmd);
    exit(EXIT_FAILURE);
  }

  char* outputPath = NULL;
  value = get_option_value(&cmd, "output");
  if(!value)
  {
      outputPath = "power_output.csv";
      printf(WARNING COLOR_WHITE "Output file is empty. Using default %s.\n" COLOR_NONE, outputPath);
  }
  else
      outputPath = (char*)value;

  char* valueType = (char*) get_option_value(&cmd, "value");
  int valueId = getValueId(valueType);

  printf(INFO "Using rail: %s\n", railType);
  printf(INFO "Watching value: %s\n", valueTypeToString(valueId));
  
  RailData rail(railId, valueId);

  // output file
  std::ofstream outputFile(outputPath);
  if( !outputFile.is_open() )
    throw std::runtime_error("Unable to open file: power_output.csv");

  outputFile << "start_time_sec;" << "start_time_nsec;";
  outputFile << "curr_" << rail.mName << ';' << "volt_" << rail.mName << ';' << "powe_" << rail.mName << '\n';

  timespec time_s;
  #ifdef LOG_VALUES
  timespec time_e, ellapsed; // time variables
  #endif

  while(!shutdownFlag)
  {
    timestamp(&time_s);
    // update values
    rail.readValues();

    rail.logRail();

    // log values
    #ifdef LOG_VALUES
    // compute duration
    timestamp(&time_e);
    timeDiff(time_s, time_e, &ellapsed);
    std::cout <<"[Time Diff] " << timeDouble(ellapsed) << '\n';
    #endif

    // save values
    outputFile << time_s.tv_sec << ';' << time_s.tv_nsec << ';';
    outputFile << rail.mCurrValue << ';' << rail.mVoltValue << ';' << rail.mPoweValue << '\n';
  }
  outputFile.close();
  // free dynamically allocated values
  free_command_line(&cmd);

  return 0;
}

// sudo ./power_profiler --rail=gpu --value=power
