#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include "profiling/argparse.h"

#define POWER_USAGE_STRING  "Usage of power profiler: ./power [--output=OUTPUT] [--log] [--help]\n"\
                            "--output=OUTPUT        The path of the file in which to write the power consumption.\n"\
                            "--log                  Display values in the consol.\n"\
                            "--help                 Show the help message.\n\n"

#define COLOR_BLUE      "\033[0;34m"
#define COLOR_GREEN     "\033[0;32m"
#define COLOR_WHITE     "\033[1;37m"
#define COLOR_NONE      "\033[0m"

typedef struct timespec timespec;

void timestamp( timespec* timestampOut )
{ 
    if(!timestampOut) 
        return; 
    timestampOut->tv_sec=0; 
    timestampOut->tv_nsec=0; 
    clock_gettime(CLOCK_REALTIME, timestampOut); 
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

double time_double( const timespec* a )	
{ 
    return a->tv_sec * 1000.0 + a->tv_nsec * 0.000001; 
}



char* read_rail(char* file_path, char* value);

void usage(void)
{
    printf(POWER_USAGE_STRING);
}

bool log_values = false;
char* output_file = NULL;
FILE* output_fp = NULL;
int shutdown_flag = 0;

void sigint_handler(int sig)
{
    printf("\nCaught SIGINT!\n");
    shutdown_flag = 1;
    // fclose(output_fp);
}

void read_data(FILE* fp, int* value)
{
    fseek(fp, 0, SEEK_SET);
    fscanf(fp, "%d", value);
    // fgets(value, 6, fp);
    // value[6] = '\0';
    // fseek(fp, 0, SEEK_SET);
}

int main(int argc, char** argv)
{
    arg_option options[] = {
        OPT_BOOLEAN('l', "log", NULL),
        OPT_BOOLEAN('h', "help", NULL),
        OPT_STRING('o', "output", NULL)
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
        printf("[Error] signal SIGINT error\n");

    value = get_option_value(&cmd, "log");
    if(value)
        log_values = true;
    
    value = get_option_value(&cmd, "output");
    if(!value)
    {
        output_file = "profiling_output.csv";
        printf(COLOR_WHITE "Output file is empty. Using default %s.\n" COLOR_NONE, output_file);
    }
    else
        output_file = (char*)value;

    // print some info
    printf(COLOR_GREEN "\nWatching sensor: " COLOR_WHITE "%s\n" COLOR_NONE, "ALL");
    printf(COLOR_GREEN "Output file: " COLOR_WHITE "%s\n" COLOR_NONE, output_file);

    // sysfs i2c INA base path
    char* base_path="/sys/bus/i2c/drivers/ina3221x/6-0040/iio:device0/";

    // define paths
    char sysfs_path[100] = ""; // path to system files

    // file pointer
    FILE *output_fp = fopen(output_file, "w");
    if(!output_fp)
        goto cleaning_up;

    char rail_name[15] = "";
    // read rail names
    printf("\n");
    // 0- Is input
    sprintf(sysfs_path, "%srail_name_0", base_path);
    read_rail(sysfs_path, rail_name);
    fprintf(output_fp, "start_time;in_current_%s;in_voltage_%s;in_power_%s", rail_name, rail_name, rail_name);
    printf(COLOR_GREEN "Rail 0: " COLOR_WHITE "%s\n" COLOR_NONE, rail_name);
    // 1- Is GPU
    sprintf(sysfs_path, "%srail_name_1", base_path);
    read_rail(sysfs_path, rail_name);
    fprintf(output_fp, "in_current_%s;in_voltage_%s;in_power_%s", rail_name, rail_name, rail_name);
    printf(COLOR_GREEN "Rail 1: " COLOR_WHITE "%s\n" COLOR_NONE, rail_name);
    // 2- Is CPU
    sprintf(sysfs_path, "%srail_name_2", base_path);
    read_rail(sysfs_path, rail_name);
    fprintf(output_fp, "in_current_%s;in_voltage_%s;in_power_%s;duration\n", rail_name, rail_name, rail_name);
    printf(COLOR_GREEN "Rail 2: " COLOR_WHITE "%s\n" COLOR_NONE, rail_name);

    
    // define paths
    char curr0_path[100],volt0_path[100], pow0_path[100]; 
    char curr1_path[100],volt1_path[100], pow1_path[100]; 
    char curr2_path[100],volt2_path[100], pow2_path[100];
    
    // read path and try to open files
    FILE *curr0_fp = NULL, *volt0_fp = NULL, *pow0_fp = NULL;
    sprintf(curr0_path, "%sin_current0_input", base_path);
    sprintf(volt0_path, "%sin_voltage0_input", base_path);
    sprintf(pow0_path, "%sin_power0_input", base_path);

    printf("\nRail 0\n");
    printf("Path: %s\n", curr0_path);
    printf("Path: %s\n", volt0_path);
    printf("Path: %s\n", pow0_path);

    curr0_fp = fopen(curr0_path, "r");
    volt0_fp = fopen(volt0_path, "r");
    pow0_fp = fopen(pow0_path, "r");
    if(!curr0_fp || !volt0_fp || !pow0_fp)
    {
        printf("[Error] files for rail 0 not foud\n");
        goto cleaning_up0;
    }

    // read path and try to open files
    FILE *curr1_fp = NULL, *volt1_fp = NULL, *pow1_fp = NULL;
    sprintf(curr1_path, "%sin_current1_input", base_path);
    sprintf(volt1_path, "%sin_voltage1_input", base_path);
    sprintf(pow1_path, "%sin_power1_input", base_path);
    
    printf("\nRail 1\n");
    printf("Path: %s\n", curr1_path);
    printf("Path: %s\n", volt1_path);
    printf("Path: %s\n", pow1_path);

    curr1_fp = fopen(curr1_path, "r");
    volt1_fp = fopen(volt1_path, "r");
    pow1_fp = fopen(pow1_path, "r");
    if(!curr1_fp || !volt1_fp || !pow1_fp)
    {
        printf("[Error] files for rail 1 not foud\n");
        goto cleaning_up1;
    }

    // read path and try to open files
    FILE *curr2_fp = NULL, *volt2_fp = NULL, *pow2_fp = NULL;
    sprintf(curr2_path, "%sin_current2_input", base_path);
    sprintf(volt2_path, "%sin_voltage2_input", base_path);
    sprintf(pow2_path, "%sin_power2_input", base_path);
    
    printf("\nRail 2\n");
    printf("Path: %s\n", curr2_path);
    printf("Path: %s\n", volt2_path);
    printf("Path: %s\n", pow2_path);

    curr2_fp = fopen(curr2_path, "r");
    volt2_fp = fopen(volt2_path, "r");
    pow2_fp = fopen(pow2_path, "r");
    if(!curr2_fp || !volt2_fp || !pow2_fp)
    {
        printf("[Error] files for rail 2 not foud\n");
        goto cleaning_up2;
    }

    /*/
    FILE *curr1_fp, *volt1_fp, *pow1_fp;
    sprintf(sysfs_path, "%sin_current1_input", base_path);
    curr1_fp = fopen(sysfs_path, "r");
    sprintf(sysfs_path, "%sin_voltage1_input", base_path);
    volt1_fp = fopen(sysfs_path, "r");
    sprintf(sysfs_path, "%sin_power1_input", base_path);
    pow1_fp = fopen(sysfs_path, "r");
    if(!curr1_fp || !volt1_fp || pow1_fp)
    {
        printf("[Error] files for rail 1 not foud\n");
        goto cleaning_up1;
    }
    

    //
    FILE *curr2_fp, *volt2_fp, *pow2_fp;
    sprintf(sysfs_path, "%sin_current2_input", base_path);
    curr2_fp = fopen(sysfs_path, "r");
    sprintf(sysfs_path, "%sin_voltage2_input", base_path);
    volt2_fp = fopen(sysfs_path, "r");
    sprintf(sysfs_path, "%sin_power2_input", base_path);
    pow2_fp = fopen(sysfs_path, "r");
    if(!curr2_fp || !volt2_fp || pow2_fp)
    {
        printf("[Error] files for rail 2 not foud\n");
        goto cleaning_up2;
    } */

    

    timespec time_s, time_e, ellapsed;
    // char read[10];
    int curr_val0, volt_val0, pow_val0;
    int curr_val1, volt_val1, pow_val1;
    int curr_val2, volt_val2, pow_val2;
    printf(COLOR_BLUE "\nStartted Recording ...\n" COLOR_NONE);
    while(true)
    {
        // print values
	    printf("\n");
        printf(COLOR_BLUE "[%f]\n" COLOR_NONE, time_double(&time_s));
        // # echo "$(printf "${WHITE}%-10s | ${NO_COLOR}Current: %4u mA -- Voltage: %4u mV -- Power: %4u mW" ${RAIL_NAME_1} ${in_current_1} ${in_voltage_1} ${in_power_1})"
        // # echo "$(printf "${WHITE}%-10s | ${NO_COLOR}Current: %4u mA -- Voltage: %4u mV -- Power: %4u mW" ${RAIL_NAME_2} ${in_current_2} ${in_voltage_2} ${in_power_2})"
        timestamp(&time_s);

        // read and write values
        read_data(curr0_fp, &curr_val0);
        read_data(volt0_fp, &volt_val0);
        read_data(pow0_fp, &pow_val0);
        printf("Current: %4d mA -- Voltage: %4d mV -- Power: %4d mW\n",  curr_val0, volt_val0, pow_val0);

        // read and write values
        read_data(curr1_fp, &curr_val1);
        read_data(volt1_fp, &volt_val1);
        read_data(pow1_fp, &pow_val1);
        printf("Current: %4d mA -- Voltage: %4d mV -- Power: %4d mW\n",  curr_val1, volt_val1, pow_val1);

        // read and write values
        read_data(curr2_fp, &curr_val2);
        read_data(volt2_fp, &volt_val2);
        read_data(pow2_fp, &pow_val2);
        printf("Current: %4d mA -- Voltage: %4d mV -- Power: %4d mW\n",  curr_val2, volt_val2, pow_val2);

        // pause before
        sleep(1);  // the process is to fast for the file to write it
        // compute duration
        timestamp(&time_e);
        time_diff(&time_s, &time_e, &ellapsed);

        
        printf("%f\n", time_double(&ellapsed));

        // write values to file
        fprintf(output_fp, "%f;%d;%d;%d;%d;%d;%d;%d;%d;%d;%f\n", time_double(&time_s), curr_val0, volt_val0, pow_val0, 
        curr_val1, volt_val1, pow_val1, curr_val2, volt_val2, pow_val2, time_double(&ellapsed));

        if(shutdown_flag) 
            break;
    }

    printf("Cleaning up...\n");
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

char* read_rail(char* file_path, char* value)
{ 
    FILE *fp = fopen(file_path, "r");
    if(fp == NULL) {
        printf("[Error] Unable to open file %s\n", file_path);
        return NULL;
    }
    fscanf(fp, "%s", value);
    fclose(fp);
    return value;
}
