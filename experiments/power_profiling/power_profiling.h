#include <fstream>
#include <profiling/logger.h>

#define BUFF_SIZE 100

#define I2C_BASE_PATH "/sys/bus/i2c/drivers/ina3221x/6-0040/iio:device0/"

#define RAIL_NAME_PATH_F     I2C_BASE_PATH "rail_name_%d"
#define RAIL_CURRENT_PATH_F  I2C_BASE_PATH "in_current%d_input"
#define RAIL_VOLTAGE_PATH_F  I2C_BASE_PATH "in_voltage%d_input"
#define RAIL_POWER_PATH_F    I2C_BASE_PATH "in_power%d_input"

static bool logValues;

enum RailTypes
{
  BOARD_RAIL = 0,
  GPU_RAIL,
  CPU_RAIL
};

enum ValueTypes
{
  CURRENT_VALUE = 0,
  VOLTAGE_VALUE,
  POWER_VALUE,
  ALL_VALUE
};

struct RailData
{
  uint8_t       mId;
  std::string   mName;
  std::string   mCurrValue;
  std::string   mVoltValue;
  std::string   mPoweValue;

  // Constructor
  RailData(uint8_t id) : RailData(id, ALL_VALUE)
  {}

  RailData(uint8_t id, int valueId) : mId(id)
  {
    initRail(valueId);
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
    mCurrFile.seekg(0, std::ios::beg);

    // Read voltage value
    getline(mVoltFile, mVoltValue);
    mVoltFile.seekg(0, std::ios::beg);

    // Read power value
    getline(mPoweFile, mPoweValue);
    mPoweFile.seekg(0, std::ios::beg);
  }

  void logRail()
  {
    log(COLOR_WHITE "[%-11s] " COLOR_NONE "Current: %4s mA -- Voltage: %4s mV -- Power: %4s mW\n", 
    mName.c_str(), mCurrValue.c_str(), mVoltValue.c_str(), mPoweValue.c_str());
  }

private:
  std::ifstream mCurrFile;
  std::ifstream mVoltFile;
  std::ifstream mPoweFile;
  void initRail(int valueId);
};

void RailData::initRail(int valueId)
{
  if(mId < 0 || mId > 2)
    throw std::invalid_argument("Rail id should be 0, 1 or 2.");
  
  char path[BUFF_SIZE];
  sprintf(path, RAIL_NAME_PATH_F, mId);
  
  // open file and read name
  std::ifstream file(path);

  if( !(file.is_open() && getline(file, mName)) )
    mName = "Rail" + std::to_string(mId);
  file.close();

  if(valueId == ALL_VALUE || valueId == CURRENT_VALUE)
  {  
    // open current file
    sprintf(path, RAIL_CURRENT_PATH_F, mId);
    mCurrFile.open(path);
    if(!mCurrFile.is_open())
      throw std::runtime_error(std::string("Unable to open file: ") + path);
  }
  
  if(valueId == ALL_VALUE || valueId == VOLTAGE_VALUE)
  { 
    // open voltage file
    sprintf(path, RAIL_VOLTAGE_PATH_F, mId);
    mVoltFile.open(path);
    if(!mVoltFile.is_open())
      throw std::runtime_error(std::string("Unable to open file: ") + path);
  }
  
  if(valueId == ALL_VALUE || valueId == POWER_VALUE)
  { 
    // open power file
    sprintf(path, RAIL_POWER_PATH_F, mId);
    mPoweFile.open(path);
    if(!mPoweFile.is_open())
      throw std::runtime_error(std::string("Unable to open file: ") + path);
  }
}



const char* valueTypeToString(int valueId)
{
  switch(valueId)
  {
    case POWER_VALUE:
      return "POWER";
    case CURRENT_VALUE:
      return "CURRENT";
    case VOLTAGE_VALUE:
      return "VOLTAGE";
    case ALL_VALUE:
      return "ALL";
    default:
      return "UNKNOWN";
  }
}
