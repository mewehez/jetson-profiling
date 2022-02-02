#ifndef __POWER_H__
#define __POWER_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BUFF_SIZE 100
// sysfs i2c INA base path
#define I2C_BASE_PATH "/sys/bus/i2c/drivers/ina3221x/6-0040/iio:device0/"

#define RAIL0_NAME_PATH     I2C_BASE_PATH "rail_name_0"
#define RAIL0_CURRENT_PATH  I2C_BASE_PATH "in_current0_input"
#define RAIL0_VOLTAGE_PATH  I2C_BASE_PATH "in_voltage0_input"
#define RAIL0_POWER_PATH    I2C_BASE_PATH "in_power0_input"

#define RAIL1_NAME_PATH     I2C_BASE_PATH "rail_name_1"
#define RAIL1_CURRENT_PATH  I2C_BASE_PATH "in_current1_input"
#define RAIL1_VOLTAGE_PATH  I2C_BASE_PATH "in_voltage1_input"
#define RAIL1_POWER_PATH    I2C_BASE_PATH "in_power1_input"

#define RAIL2_NAME_PATH     I2C_BASE_PATH "rail_name_2"
#define RAIL2_CURRENT_PATH  I2C_BASE_PATH "in_current2_input"
#define RAIL2_VOLTAGE_PATH  I2C_BASE_PATH "in_voltage2_input"
#define RAIL2_POWER_PATH    I2C_BASE_PATH "in_power2_input"

// colors used to style th console outputs

#define COLOR_BLUE      "\033[0;34m"
#define COLOR_GREEN     "\033[0;32m"
#define COLOR_WHITE     "\033[1;37m"
#define COLOR_RED       "\033[0;31m"
#define COLOR_YELLOW    "\033[0;33m"
#define COLOR_NONE      "\033[0m"

#define WARNING     COLOR_YELLOW"[Warn] "COLOR_NONE
#define ERROR       COLOR_RED"[Error] "COLOR_NONE
#define INFO        COLOR_BLUE"[Info] "COLOR_NONE
#define DEBUG       COLOR_WHITE"[Debug] "COLOR_NONE

#define usage() printf(POWER_USAGE_STRING)
#define log(format, args...) if(log_values == 1) printf(format, ##args)

#define POWER_USAGE_STRING  "Usage of power profiler: \n"\
                            "./power [--output=OUTPUT] [--delay=DELAY] [--log] [--help]\n"\
                            "Arguments: \n"\
                            "--output           The path of the file in which to write the power consumption.\n"\
                            "--delay            Delay in milliseconds before next power reading.\n"\
                            "--log              Display values in the consol.\n"\
                            "--help             Show the help message.\n\n"


static bool log_values;

typedef struct timespec timespec;

typedef struct 
{
    // rail values
    char name[15];
    char current[15];
    char voltage[15];
    char power[15];

    // rail files pointers
    FILE* curr_fp;
    FILE* volt_fp;
    FILE* pow_fp;
} rail;

/**
 * @brief Gets the system current time in seceonds and nano seconds.
 * 
 * Inspired by the git of jetson utils
 * https://github.com/dusty-nv/jetson-utils
 * 
 * @param timestamp_out A pointer to the timespec to store the result.
 */
void timestamp( timespec* timestamp_out );

/**
 * @brief Computes de difference between two times.
 * 
 * Inspired by the git of jetson utils
 * https://github.com/dusty-nv/jetson-utils
 * 
 * @param start     A pointer to the first time.
 * @param end       A pointer to the second time (should be greater than start).
 * @param result    A pointer to the timespec to store result.
 */
void time_diff( const timespec* start, const timespec* end, timespec* result );

/**
 * @brief Converts a timespec into double.
 * 
 * Inspired by the git of jetson utils
 * https://github.com/dusty-nv/jetson-utils
 * 
 * @param a         Apointer to the timespec to convert.
 * @return double   The value after conversion.
 */
double time_double( const timespec* a )	
{ 
    return a->tv_sec * 1000.0 + a->tv_nsec * 0.000001; 
}

/**
 * @brief Halts the process for milliseconds.
 * 
 * Inspired by this blog:
 * https://qnaplus.com/c-program-to-sleep-in-milliseconds/
 * 
 * @param tms The time in milliseconds.
 * @return int 0 on success and -1 on error.
 */
int msleep(unsigned int tms) {
  return usleep(tms * 1000);
}

/**
 * @brief Reads rail name from system files.
 * 
 * @param file_path The path of the system file containing rail name.
 * @param value     A pointer to the variable in which the result is stored.
 * @return int      0 if success, -1 if error. 
 */
int read_rail(char* file_path, char* value);

/**
 * @brief Read the first line in system file.
 * 
 * @param file_path The path to the syetem file.
 * @param value     The pointer of the variable to store the result.
 * @return int      0 if success, -1 if error.
 */
int read_data(FILE* fp, int* value);
// int read_data(char* file_path, int* value);
/*int read_data(FILE* fp, int* value)
{
    if(fseek(fp, 0, SEEK_SET) != 0)  // place cursor at begining
        return -1;
    if(fscanf(fp, "%d", value) < 0)
        return -1;
    return 0;
}*/

int open_file(char* file_path, char* mode, FILE* *fp);

void drop_eol(char* string)
{
    char *c = strchr(string, '\n');
    if (c)
        *c = 0;
}

int read_rail_data(rail* p_rail)
{
    rewind(p_rail->curr_fp);
    rewind(p_rail->volt_fp);
    rewind(p_rail->pow_fp);
    // if(
    //     fseek(p_rail->curr_fp, 0, SEEK_SET) != 0 ||
    //     fseek(p_rail->volt_fp, 0, SEEK_SET) != 0 ||
    //     fseek(p_rail->pow_fp,  0, SEEK_SET) != 0
    // ) 
    // {
    //     puts(ERROR "Failed cursor reset.");
    //     return -1;
    // }
    if(
        !fgets(p_rail->current, sizeof(p_rail->current), p_rail->curr_fp) ||
        !fgets(p_rail->voltage, sizeof(p_rail->voltage), p_rail->volt_fp) ||
        !fgets(p_rail->power, sizeof(p_rail->power), p_rail->pow_fp)
    )
    {
        puts(ERROR "Inside read_rail_data function.");
        return -1;
    }
    drop_eol(p_rail->current);
    drop_eol(p_rail->voltage);
    drop_eol(p_rail->power);
    return 0;
}

void log_rail(rail* p_rail)
{
    log(COLOR_WHITE "[%-11s] " COLOR_NONE "Current: %4s mA -- Voltage: %4s mV -- Power: %4s mW\n", 
    p_rail->name, p_rail->current, p_rail->voltage, p_rail->power);
}

void free_rail(rail* p_rail)
{
    // close files
    fclose(p_rail->curr_fp);
    fclose(p_rail->volt_fp);
    fclose(p_rail->pow_fp);
}

#ifdef __cplusplus
}
#endif

#endif
