// reading a text file
#include <iostream>
#include <fstream>
#include <string>
#include <cstdio>
#include <signal.h>
#include <jetson-utils/timespec.h>
using namespace std;


#define BUFF_SIZE 100

#define I2C_BASE_PATH "/sys/bus/i2c/drivers/ina3221x/6-0040/iio:device0/"

#define RAIL_NAME_PATH_F     I2C_BASE_PATH "rail_name_%d"
#define RAIL_CURRENT_PATH_F  I2C_BASE_PATH "in_current%d_input"
#define RAIL_VOLTAGE_PATH_F  I2C_BASE_PATH "in_voltage%d_input"
#define RAIL_POWER_PATH_F    I2C_BASE_PATH "in_power%d_input"

#define COLOR_BLUE      "\033[0;34m"
#define COLOR_GREEN     "\033[0;32m"
#define COLOR_WHITE     "\033[1;37m"
#define COLOR_RED       "\033[0;31m"
#define COLOR_YELLOW    "\033[0;33m"
#define COLOR_NONE      "\033[0m"

#define WARNING     COLOR_YELLOW "[Warn] " COLOR_NONE
#define ERROR       COLOR_RED "[Error] " COLOR_NONE
#define INFO        COLOR_BLUE "[Info] " COLOR_NONE
#define DEBUG       COLOR_WHITE "[Debug] " COLOR_NONE

// #define LOG_VALUES 1
#define log(format, args...) if(logValues == 1) printf(format, ##args)

static bool logValues = true;

struct RailData
{
  uint8_t mId;
  string  mName;
  string  mCurrValue;
  string  mVoltValue;
  string  mPoweValue;

  // Constructor
  RailData(uint8_t id) : mId(id)
  {
    initRail();
  }

  // destructor
  ~RailData()
  {
    if ( mCurrFile.is_open() ) mCurrFile.close();
    if ( mVoltFile.is_open() ) mVoltFile.close();
    if ( mPoweFile.is_open() ) mPoweFile.close();
  }

  void readValues()
  {
    // Read current value
    getline(mCurrFile, mCurrValue);
    mCurrFile.seekg(0, ios::beg);

    // Read voltage value
    getline(mVoltFile, mVoltValue);
    mVoltFile.seekg(0, ios::beg);

    // Read power value
    getline(mPoweFile, mPoweValue);
    mPoweFile.seekg(0, ios::beg);
  }

  void logRail()
  {
    log(COLOR_WHITE "[%-11s] " COLOR_NONE "Current: %4s mA -- Voltage: %4s mV -- Power: %4s mW\n", 
    mName.c_str(), mCurrValue.c_str(), mVoltValue.c_str(), mPoweValue.c_str());
  }

private:
  ifstream mCurrFile;
  ifstream mVoltFile;
  ifstream mPoweFile;
  void initRail();
};

void RailData::initRail()
{
  if(mId < 0 || mId > 2)
    throw std::invalid_argument("Rail id should be 0, 1 or 2.");
  
  char path[BUFF_SIZE];
  sprintf(path, RAIL_NAME_PATH_F, mId);
  
  // open file and read name
  ifstream file(path);

  if( !(file.is_open() && getline(file, mName)) )
    mName = "Rail" + std::to_string(mId);
  file.close();

  // open current file
  sprintf(path, RAIL_CURRENT_PATH_F, mId);
  mCurrFile.open(path);
  if(!mCurrFile.is_open())
    throw std::runtime_error(std::string("Unable to open file: ") + path);
  
  // open voltage file
  sprintf(path, RAIL_VOLTAGE_PATH_F, mId);
  mVoltFile.open(path);
  if(!mVoltFile.is_open())
    throw std::runtime_error(std::string("Unable to open file: ") + path);
  
  // open power file
  sprintf(path, RAIL_POWER_PATH_F, mId);
  mPoweFile.open(path);
  if(!mPoweFile.is_open())
    throw std::runtime_error(std::string("Unable to open file: ") + path);
}

bool shutdownFlag = false;
void sigintHandler(int sig)
{
  printf("\nCaught SIGINT!\n");
  shutdownFlag = true;
}

int main () {
  if (signal(SIGINT, sigintHandler) == SIG_ERR)
  {
    printf(ERROR " Signal SIGINT error\n");
    exit(EXIT_FAILURE);
  }
  RailData rail0(0); //, rail1(1), rail2(2);

  // output file
  ofstream outputFile("power_output.csv");
  if( !outputFile.is_open() )
    throw std::runtime_error("Unable to open file: power_output.csv");

  outputFile << "start_time_sec;" << "start_time_nsec;";
  outputFile << "curr_" << rail0.mName << ';' << "volt_" << rail0.mName << ';' << "powe_" << rail0.mName << '\n';
  // outputFile << "curr_" << rail1.mName << ';' << "volt_" << rail1.mName << ';' << "powe_" << rail1.mName << ';';
  // outputFile << "curr_" << rail2.mName << ';' << "volt_" << rail2.mName << ';' << "powe_" << rail2.mName << '\n';

  timespec time_s;
  #ifdef LOG_VALUES
  timespec time_e, ellapsed; // time variables
  #endif
  while(!shutdownFlag)
  {
    timestamp(&time_s);
    // update values
    rail0.readValues();
    // rail1.readValues();
    // rail2.readValues();

    // log values
    #ifdef LOG_VALUES
    // compute duration
    timestamp(&time_e);
    timeDiff(time_s, time_e, &ellapsed);
    rail0.logRail();
    // rail1.logRail();
    // rail2.logRail();
    cout <<"[Time Diff] " << timeDouble(ellapsed) << '\n';
    #endif

    // save values
    outputFile << time_s.tv_sec << ';' << time_s.tv_nsec << ';';
    outputFile << rail0.mCurrValue << ';' << rail0.mVoltValue << ';' << rail0.mPoweValue << '\n';
    // outputFile << rail1.mCurrValue << ';' << rail1.mVoltValue << ';' << rail1.mPoweValue << ';';
    // outputFile << rail2.mCurrValue << ';' << rail2.mVoltValue << ';' << rail2.mPoweValue << '\n';
  }
  outputFile.close();

  return 0;
}