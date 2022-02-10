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
#include "profiling/argparse.h"

#include "power.h"

// If 1, the infinite loop will exit.
// Value is set to 1 when Ctrl+C is pressed.
int shutdown_flag = 0;

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

/*char* output_path = NULL; // Path to the output file.
FILE* output_fp = NULL;
timespec time_s, time_e, ellapsed; // time variables
int status = 0; // variable for error handling
static bool log_values = false; // Print values in console?

// rails for file reading
rail rail0 = { "", 0, 0, 0, NULL, NULL, NULL};
rail rail1 = { "", 0, 0, 0, NULL, NULL, NULL};
rail rail2 = { "", 0, 0, 0, NULL, NULL, NULL};*/

// main function
int main(int argc, char** argv)
{
    /*
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
        output_path = "profiling_output.csv";
        printf(WARNING COLOR_WHITE "Output file is empty. Using default %s.\n" COLOR_NONE, output_path);
    }
    else
        output_path = (char*)value;

    // print some info
    puts("\n" INFO);
    printf(COLOR_WHITE "Watching sensor: %s\n" COLOR_NONE, "ALL");
    printf(COLOR_WHITE "Output file: %s\n" COLOR_NONE, output_path);
    

    // file pointer
    status = open_file(output_path, "w", &output_fp);
    if(status != 0)
        goto cleaning_up;

    // read rail names
    log("\n" DEBUG "\n");
    
    // 0- Is input
    status = read_rail(RAIL0_NAME_PATH, rail0.name);
    if(status != 0) // if we couldn't find the name
        strcpy(rail0.name, "rail0");
    fprintf(output_fp, "start_time;in_current_%s;in_voltage_%s;in_power_%s", rail0.name, rail0.name, rail0.name);
    log(COLOR_GREEN "Rail 0: " COLOR_WHITE "%s\n" COLOR_NONE, rail0.name);
    
    // 1- Is GPU
    status = read_rail(RAIL1_NAME_PATH, rail1.name);
    if(status != 0) // if we couldn't find the name
        strcpy(rail1.name, "rail1");
    fprintf(output_fp, "in_current_%s;in_voltage_%s;in_power_%s", rail1.name, rail1.name, rail1.name);
    log(COLOR_GREEN "Rail 1: " COLOR_WHITE "%s\n" COLOR_NONE, rail1.name);
    
    // 2- Is CPU
    status = read_rail(RAIL2_NAME_PATH, rail2.name);
    if(status != 0) // if we couldn't find the name
        strcpy(rail2.name, "rail2");
    fprintf(output_fp, "in_current_%s;in_voltage_%s;in_power_%s;duration\n", rail2.name, rail2.name, rail2.name);
    log(COLOR_GREEN "Rail 2: " COLOR_WHITE "%s\n" COLOR_NONE, rail2.name);

    // opean rail files for reading
    if(
        open_file(RAIL0_CURRENT_PATH, "r+", &rail0.curr_fp) != 0 ||
        open_file(RAIL0_VOLTAGE_PATH, "r+", &rail0.volt_fp) != 0 ||
        open_file(RAIL0_POWER_PATH,   "r+", &rail0.pow_fp)  != 0 ||

        open_file(RAIL1_CURRENT_PATH, "r+", &rail1.curr_fp) != 0 ||
        open_file(RAIL1_VOLTAGE_PATH, "r+", &rail1.volt_fp) != 0 ||
        open_file(RAIL1_POWER_PATH,   "r+", &rail1.pow_fp)  != 0 ||

        open_file(RAIL2_CURRENT_PATH, "r+", &rail2.curr_fp) != 0 ||
        open_file(RAIL2_VOLTAGE_PATH, "r+", &rail2.volt_fp) != 0 ||
        open_file(RAIL2_POWER_PATH,   "r+", &rail2.pow_fp)  != 0
    )
    {
        puts(ERROR "Unable to open a rail file.");
        goto cleaning_up;
    }

    // read rail 0
    if(
        read_rail_data(&rail0) != 0 ||
        read_rail_data(&rail1) != 0 ||
        read_rail_data(&rail2) != 0
    )
    {
        puts(ERROR "Failed reading a rail data.");
        goto cleaning_up;
    }

    // start profiling
    puts("\n" INFO COLOR_WHITE "Startted Recording ...\n" COLOR_NONE);
    while(true)
    {
        // Get starting time
        timestamp(&time_s);
        log(COLOR_WHITE "[%f]\n" COLOR_NONE, time_double(&time_s));

        // read rail 0
        if(
            read_rail_data(&rail0) != 0 ||
            read_rail_data(&rail1) != 0 ||
            read_rail_data(&rail2) != 0
        )
        {
            puts(ERROR "Failed reading a rail data.");
            goto cleaning_up;
        }
        log_rail(&rail0);
        log_rail(&rail1);
        log_rail(&rail2);

        // compute duration
        timestamp(&time_e);
        time_diff(&time_s, &time_e, &ellapsed);

        
        log(COLOR_WHITE "[Ellapsed] " COLOR_NONE"%f\n\n", time_double(&ellapsed));

        // write values to file
        fprintf(output_fp, "%f;%d;%d;%d;%d;%d;%d;%d;%d;%d;%f\n", time_double(&time_s), 
        rail0.current, rail0.voltage, rail0.power, 
        rail1.current, rail1.voltage, rail1.power, 
        rail2.current, rail2.voltage, rail2.power, 
        time_double(&ellapsed));
        msleep(4);

        if(shutdown_flag) 
            break;
    }

    cleaning_up:
    puts(INFO COLOR_WHITE "Cleaning up...\n" COLOR_NONE);
    fclose(output_fp);
    
    // free rail
    free_rail(&rail0);
    free_rail(&rail1);
    free_rail(&rail2);

    // free dynamically allocated values
    free_command_line(&cmd);
    */
    // test_rail();

    // char link[BUFF_SIZE];
    // sprintf(link, RAIL_VOLTAGE_PATH_F, 0);

    // int fd = open(buff, O_RDONLY);
    // if(fd < 0)
    // {
    //     printf(ERROR "Couldn't open file %s\n", buff);
    //     exit(1);
    // }
    // char value[6];
    // size_t value_size = 6 * sizeof(char);
    // int status = 0;
    // for(int i = 0; i < 80; i++)
    // {
    //     status = read(fd, value, value_size);
    //     if(status < 0)
    //     {
    //         puts(ERROR "Couldn't read the file.");
    //         close(fd);
    //         exit(1);
    //     }
    //     printf(DEBUG "Got value %s --\n", value);
    // }
    // close(fd);

    test_file();

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

int open_file(char* file_path, char* mode, FILE* *fp)
{
    *fp = fopen(file_path, mode);
    if(*fp == NULL) {
        printf(ERROR "Unable to open file %s\n", file_path);
        return -1;
    }
    return 0;
}

// TODO check fscanf return. maybe eof
// int read_data(char* file_path, int* value)
// {
//     FILE *fp = fopen(file_path, "r"); // drop
//     // rewind
//     if(fp == NULL) {
//         printf(ERROR "Unable to open file %s\n", file_path);
//         return -1;
//     }
//     fscanf(fp, "%d", value); // TODO read str
//     fclose(fp); // drop
//     return 0;
// }
int read_data(FILE* fp, int* value)
{
    if(fseek(fp, 0, SEEK_SET) != 0)  // place cursor at begining
        return -1;
    if(fscanf(fp, "%d", value) < 0)
        return -1;
    return 0;
}

/*void sigint_handler(int sig)
{
    printf("\nCaught SIGINT!\n");
    shutdown_flag = 1;
}*/
