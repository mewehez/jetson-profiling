/**
 * @file power.c
 * @author Mewe-Hezoudah KAHANAM
 * @brief This file containes the main program for power level monitoring on Jetson Nano.
 *
 * It implements functions to read the power, voltage and current in the system files.
 * The program should be run in admin mode in order to have access to system files. 
 *
 * @version 0.1
 * @date 2022-02-01
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include "profiling/argparse.h"

#define BUFF_SIZE 100

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


typedef struct timespec timespec;

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
int read_data(char* file_path, int* value);
/*int read_data(FILE* fp, int* value)
{
    if(fseek(fp, 0, SEEK_SET) != 0)  // place cursor at begining
        return -1;
    if(fscanf(fp, "%d", value) < 0)
        return -1;
    return 0;
}*/

/**
 * @brief Captures the SIGINT signal and sets the shutdown flag to 1.
 * 
 * @param sig The signal id.
 */
void sigint_handler(int sig)
{
    printf("\nCaught SIGINT!\n");
    shutdown_flag = 1;
}

bool log_values = false; // Print values in console?

char* output_file = NULL; // Path to the output file.
FILE* output_fp = NULL; // File descriptor for output file.

// If 1, the infinite loop will exit.
// Value is set to 1 when Ctrl+C is pressed.
int shutdown_flag = 0;

// TODO: error handling
int status = 0;  // variable for error handling

// main function
int main(int argc, char** argv)
{
    arg_option options[] = {
        OPT_BOOLEAN('l', "log", NULL, NULL),
        OPT_BOOLEAN('h', "help", NULL, NULL),
        OPT_STRING('o', "output", NULL, NULL)
    };
    const int cmd_size = 3;
    
    command_line cmd = { options, cmd_size };
    parse_command_line(&cmd, argc, argv);
    
    void* value = get_option_value(&cmd, "help");
    if(value)
    {
        usage();
        exit(0);
    }

    // bind signal listener
    if (signal(SIGINT, sigint_handler) == SIG_ERR)
    {
        printf(ERROR" Signal SIGINT error\n");
        exit(1);
    }

    value = get_option_value(&cmd, "log");
    if(value)
        log_values = true;
    
    value = get_option_value(&cmd, "output");
    if(!value)
    {
        output_file = "profiling_output.csv";
        printf(WARNING COLOR_WHITE "Output file is empty. Using default %s.\n" COLOR_NONE, output_file);
    }
    else
        output_file = (char*)value;

    // print some info
    puts("\n" INFO);
    printf(COLOR_WHITE "Watching sensor: %s\n" COLOR_NONE, "ALL");
    printf(COLOR_WHITE "Output file: %s\n" COLOR_NONE, output_file);

    // sysfs i2c INA base path
    char* base_path="/sys/bus/i2c/drivers/ina3221x/6-0040/iio:device0/";

    // define paths
    char sysfs_path[BUFF_SIZE] = ""; // path to system files

    // file pointer
    FILE *output_fp = fopen(output_file, "w");
    if(!output_fp)
        goto cleaning_up;

    char rail_name[15] = "";
    // read rail names
    log("\n" DEBUG);
    
    // 0- Is input
    sprintf(sysfs_path, "%srail_name_0", base_path);
    status = read_rail(sysfs_path, rail_name);
    if(status != 0) // if we couldn't find the name
        strcpy(rail_name, "Rail 0");
    fprintf(output_fp, "start_time;in_current_%s;in_voltage_%s;in_power_%s", rail_name, rail_name, rail_name);
    log(COLOR_GREEN "Rail 0: " COLOR_WHITE "%s\n" COLOR_NONE, rail_name);
    
    // 1- Is GPU
    sprintf(sysfs_path, "%srail_name_1", base_path);
    status = read_rail(sysfs_path, rail_name);
    if(status != 0) // if we couldn't find the name
        strcpy(rail_name, "Rail 0");
    fprintf(output_fp, "in_current_%s;in_voltage_%s;in_power_%s", rail_name, rail_name, rail_name);
    log(COLOR_GREEN "Rail 1: " COLOR_WHITE "%s\n" COLOR_NONE, rail_name);
    
    // 2- Is CPU
    sprintf(sysfs_path, "%srail_name_2", base_path);
    status = read_rail(sysfs_path, rail_name);
    if(status != 0) // if we couldn't find the name
        strcpy(rail_name, "Rail 0");
    fprintf(output_fp, "in_current_%s;in_voltage_%s;in_power_%s;duration\n", rail_name, rail_name, rail_name);
    log(COLOR_GREEN "Rail 2: " COLOR_WHITE "%s\n" COLOR_NONE, rail_name);

    // define paths
    char curr0_path[BUFF_SIZE],volt0_path[BUFF_SIZE], pow0_path[BUFF_SIZE]; 
    char curr1_path[BUFF_SIZE],volt1_path[BUFF_SIZE], pow1_path[BUFF_SIZE]; 
    char curr2_path[BUFF_SIZE],volt2_path[BUFF_SIZE], pow2_path[BUFF_SIZE];
    
    // read path and try to open files
    FILE *curr0_fp = NULL, *volt0_fp = NULL, *pow0_fp = NULL;
    sprintf(curr0_path, "%sin_current0_input", base_path);
    sprintf(volt0_path, "%sin_voltage0_input", base_path);
    sprintf(pow0_path, "%sin_power0_input", base_path);

    log("\n" DEBUG COLOR_WHITE "Rail 0 files" COLOR_NONE);
    log("%s\n", curr0_path);
    log("%s\n", volt0_path);
    log("%s\n", pow0_path);

    curr0_fp = fopen(curr0_path, "r");
    volt0_fp = fopen(volt0_path, "r");
    pow0_fp = fopen(pow0_path, "r");
    if(!curr0_fp || !volt0_fp || !pow0_fp)
    {
        puts(ERROR "Files for rail 0 not foud");
        goto cleaning_up0;
    }

    // read path and try to open files
    FILE *curr1_fp = NULL, *volt1_fp = NULL, *pow1_fp = NULL;
    sprintf(curr1_path, "%sin_current1_input", base_path);
    sprintf(volt1_path, "%sin_voltage1_input", base_path);
    sprintf(pow1_path, "%sin_power1_input", base_path);
    
    log("\n" DEBUG COLOR_WHITE "Rail 1 files" COLOR_NONE);
    log("%s\n", curr1_path);
    log("%s\n", volt1_path);
    log("%s\n", pow1_path);

    curr1_fp = fopen(curr1_path, "r");
    volt1_fp = fopen(volt1_path, "r");
    pow1_fp = fopen(pow1_path, "r");
    if(!curr1_fp || !volt1_fp || !pow1_fp)
    {
        puts(ERROR "Files for rail 1 not foud");
        goto cleaning_up1;
    }

    // read path and try to open files
    FILE *curr2_fp = NULL, *volt2_fp = NULL, *pow2_fp = NULL;
    sprintf(curr2_path, "%sin_current2_input", base_path);
    sprintf(volt2_path, "%sin_voltage2_input", base_path);
    sprintf(pow2_path, "%sin_power2_input", base_path);
    
    log("\n" DEBUG COLOR_WHITE "Rail 2 files" COLOR_NONE);
    log("%s\n", curr2_path);
    log("%s\n", volt2_path);
    log("%s\n", pow2_path);

    curr2_fp = fopen(curr2_path, "r");
    volt2_fp = fopen(volt2_path, "r");
    pow2_fp = fopen(pow2_path, "r");
    if(!curr2_fp || !volt2_fp || !pow2_fp)
    {
        puts(ERROR "Files for rail 2 not foud");
        goto cleaning_up2;
    }

    timespec time_s, time_e, ellapsed;
    // char read[10];
    int curr_val0, volt_val0, pow_val0;
    int curr_val1, volt_val1, pow_val1;
    int curr_val2, volt_val2, pow_val2;
    puts("\n" INFO COLOR_WHITE "Startted Recording ...\n" COLOR_NONE);
    while(true)
    {
        // Get starting time
        timestamp(&time_s);
        log(COLOR_WHITE "[%f]\n" COLOR_NONE, time_double(&time_s));

        // read and write values
        if(
            read_data(curr0_fp, &curr_val0) < 0 ||
            read_data(volt0_fp, &volt_val0) < 0 ||
            read_data(pow0_fp, &pow_val0) < 0
        )
            goto cleaning_up2;
        log(COLOR_WHITE "[Rail 0] " COLOR_NONE "Current: %4d mA -- Voltage: %4d mV -- Power: %4d mW\n",  curr_val0, volt_val0, pow_val0);

        // read and write values
        if(
            read_data(curr1_fp, &curr_val1) < 0 ||
            read_data(volt1_fp, &volt_val1) < 0 ||
            read_data(pow1_fp, &pow_val1) < 0
        )
            goto cleaning_up2;
        log(COLOR_WHITE "[Rail 1] " COLOR_NONE "Current: %4d mA -- Voltage: %4d mV -- Power: %4d mW\n",  curr_val1, volt_val1, pow_val1);

        // read and write values
        if(
            read_data(curr2_fp, &curr_val2) < 0 ||
            read_data(volt2_fp, &volt_val2) < 0 ||
            read_data(pow2_fp, &pow_val2) < 0
        )
            goto cleaning_up2;
        log(COLOR_WHITE "[Rail 2] " COLOR_NONE "Current: %4d mA -- Voltage: %4d mV -- Power: %4d mW\n",  curr_val2, volt_val2, pow_val2);

        // compute duration
        timestamp(&time_e);
        time_diff(&time_s, &time_e, &ellapsed);

        
        log(COLOR_WHITE "[Ellapsed] " COLOR_NONE"%f\n\n", time_double(&ellapsed));

        // write values to file
        fprintf(output_fp, "%f;%d;%d;%d;%d;%d;%d;%d;%d;%d;%f\n", time_double(&time_s), curr_val0, volt_val0, pow_val0, 
        curr_val1, volt_val1, pow_val1, curr_val2, volt_val2, pow_val2, time_double(&ellapsed));

        msleep(1);  // delay to sample less frequently

        if(shutdown_flag) 
            break;
    }

    puts(INFO COLOR_WHITE "Cleaning up...\n" COLOR_NONE);
    // close all files
    cleaning_up2:
    fclose(pow2_fp);
    fclose(volt2_fp);
    fclose(curr2_fp);

    cleaning_up1:
    fclose(pow1_fp);
    fclose(volt1_fp);
    fclose(curr1_fp);
    
    cleaning_up0:
    fclose(pow0_fp);
    fclose(volt0_fp);
    fclose(curr0_fp);

    cleaning_up:
    fclose(output_fp);

    // free dynamically allocated values
    free_command_line(&cmd);
    return 0;
}


void timestamp( timespec* timestamp_out )
{ 
    if(!timestamp_out) 
        return; 
    timestamp_out->tv_sec=0; 
    timestamp_out->tv_nsec=0; 
    clock_gettime(CLOCK_REALTIME, timestamp_out); 
}

void time_diff( const timespec* start, const timespec* end, timespec* result )
{
	if ((end->tv_nsec-start->tv_nsec)<0) {
		result->tv_sec = end->tv_sec-start->tv_sec-1;
		result->tv_nsec = 1000000000+end->tv_nsec-start->tv_nsec;
	} else {
		result->tv_sec = end->tv_sec-start->tv_sec;
		result->tv_nsec = end->tv_nsec-start->tv_nsec;
	}
}

int read_rail(char* file_path, char* value)
{ 
    FILE *fp = fopen(file_path, "r");
    if(fp == NULL) {
        printf(ERROR "Unable to open file %s\n", file_path);
        return -1;
    }
    fscanf(fp, "%s", value);
    fclose(fp);
    return 0;
}

int read_data(char* file_path, int* value)
{
    FILE *fp = fopen(file_path, "r");
    if(fp == NULL) {
        printf(ERROR "Unable to open file %s\n", file_path);
        return -1;
    }
    fscanf(fp, "%d", value);
    fclose(fp);
    return 0;
}
// int read_data(char* file_path, int* value)
// {
//     if(fseek(fp, 0, SEEK_SET) != 0)  // place cursor at begining
//         return -1;
//     if(fscanf(fp, "%d", value) < 0)
//         return -1;
//     return 0;
// }

/*void sigint_handler(int sig)
{
    printf("\nCaught SIGINT!\n");
    shutdown_flag = 1;
}*/
