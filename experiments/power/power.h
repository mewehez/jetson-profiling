#ifndef __POWER_H__
#define __POWER_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define BUFF_SIZE 100
// sysfs i2c INA base path
#define I2C_BASE_PATH "/sys/bus/i2c/drivers/ina3221x/6-0040/iio:device0/"

#define RAIL_NAME_PATH_F     I2C_BASE_PATH "rail_name_%d"
#define RAIL_CURRENT_PATH_F  I2C_BASE_PATH "in_current%d_input"
#define RAIL_VOLTAGE_PATH_F  I2C_BASE_PATH "in_voltage%d_input"
#define RAIL_POWER_PATH_F    I2C_BASE_PATH "in_power%d_input"

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
#define rail_new() {-1, NULL, NULL, NULL, NULL, NULL, NULL, NULL}


static bool_t log_values;

typedef struct timespec timespec;

typedef struct 
{
    int fd;
    struct stat file_info;
} file_t;

typedef struct 
{
    // rail values
    int id;
    char* name;
    char* c_value;
    char* v_value;
    char* p_value;

    // rail files pointers
    file_t* c_file;
    file_t* v_file;
    file_t* p_file;
} rail_t;

void rail_set_name(rail_t* rail, char* path)
{
    char tmp[15] = "";
    FILE *fp = fopen(path, "r");
    if(fp == NULL) {
        printf(ERROR "Unable to open file %s\n", path);
        sprintf(tmp, "rail%d", rail->id);
        rail->name = (char*) calloc(7, sizeof(char));
        strncpy(rail->name, tmp, 7);
        return;
    }
    fscanf(fp, "%s", tmp);
    int size = strlen(tmp);
    rail->name = (char*) calloc(size, sizeof(char));
    strcpy(rail->name, tmp);
    fclose(fp);
}

int rail_set_value(file_t* *file, char* *value, char* path)
{
    /* Open the bash ELF executable file on Linux */
    *file = (file_t*) malloc(sizeof(file_t*));
    (*file)->fd = open(path, O_RDONLY);
    if ((*file)->fd == -1)
        return -1;

    /* Get information about the file, including size */
    struct stat file_info;
    if(fstat((*file)->fd, &(*file)->file_info) == -1)
        return -1;

    /* Create a private, read-only memory mapping */
    *value = (char*) mmap(NULL, (*file)->file_info.st_size, PROT_READ, MAP_PRIVATE, (*file)->fd, 0);
    if(*value == MAP_FAILED)
        return -1;
    return 0;
}

int rail_init(rail_t* rail, int id) 
{
    if( id < 0 || id > 2)
        printf(ERROR "Invalid rail id %d. Should be 0, 1 or 2.\n", id);
    rail->id = id; //  set the id

    // get the name
    char buff[BUFF_SIZE] = "";
    sprintf(buff, RAIL_NAME_PATH_F, id);
    rail_set_name(rail, buff);

    // open Memory mapped files
    sprintf(buff, RAIL_CURRENT_PATH_F, id);
    if(rail_set_value(&rail->c_file, &rail->c_value, buff) != 0)
        return -1;
    
    sprintf(buff, RAIL_VOLTAGE_PATH_F, id);
    if(rail_set_value(&rail->v_file, &rail->v_value, buff) != 0)
        return -1;
    
    sprintf(buff, RAIL_POWER_PATH_F,   id);
    if(rail_set_value(&rail->p_file, &rail->p_value, buff) != 0)
        return -1;

    return 0;
}

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

/*int read_rail_data(rail* p_rail)
{
    rewind(p_rail->curr_fp);
    rewind(p_rail->volt_fp);
    rewind(p_rail->pow_fp);
    p_rail->current = 0;
    p_rail->voltage = 0;
    p_rail->power = 0;
    if(
        fscanf(p_rail->curr_fp, "%d", &p_rail->current) == 0 ||
        fscanf(p_rail->volt_fp, "%d", &p_rail->voltage) == 0 ||
        fscanf(p_rail->pow_fp,  "%d", &p_rail->power)   == 0
    )
    {
        puts(ERROR "Inside read_rail_data function.");
        return -1;
    }
    return 0;
}*/

void rail_log(rail_t* rail)
{
    puts("OK3-");
    log(COLOR_WHITE "[%-11s] " COLOR_NONE "Current: %4s mA -- Voltage: %4s mV -- Power: %4s mW\n", 
    rail->name, rail->c_value, rail->v_value, rail->p_value);
}

void rail_unmap_close(char* mmap_addr, file_t* file)
{
    if(!file)
        return;
    munmap(mmap_addr, file->file_info.st_size);
    close(file->fd);
    free(file);  // we used calloc for this one
}

void rail_free(rail_t* rail)
{
    // free memory
    free(rail->name);
    // unmap and close files
    rail_unmap_close(rail->c_value, rail->c_file);
    rail_unmap_close(rail->v_value, rail->v_file);
    rail_unmap_close(rail->p_value, rail->p_file);
}

void test_mman(void)
{
    /* Code Listing 3.6:
    Read the first bytes of the bash executable to confirm it is ELF format
    */

    /* Open the bash ELF executable file on Linux */
    int fd = open("/bin/bash", O_RDONLY);
    assert (fd != -1);

    /* Get information about the file, including size */
    struct stat file_info;
    assert (fstat (fd, &file_info) != -1);

    /* Create a private, read-only memory mapping */
    char *mmap_addr = (char*) mmap(NULL, file_info.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    assert (mmap_addr != MAP_FAILED);

    /* ELF specification:
    Bytes 1 - 3 of the file must be 'E', 'L', 'F' */
    assert (mmap_addr[1] == 'E');
    assert (mmap_addr[2] == 'L');
    assert (mmap_addr[3] == 'F');

    /* Unmap the file and close it */
    munmap (mmap_addr, file_info.st_size);
    close (fd);
}

void test_file(void)
{
    char path[] = "/sys/bus/i2c/drivers/ina3221x/6-0040/iio:device0/in_voltage0_input";
    /* Open the bash ELF executable file on Linux */
    int fd = open(path, O_RDONLY);
    assert (fd != -1);

    /* Get information about the file, including size */
    struct stat file_info;
    assert (fstat (fd, &file_info) != -1);

    /* Create a private, read-only memory mapping */
    char *mmap_addr = (char*) mmap(NULL, file_info.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    printf("Mapped value %s\n", mmap_addr);
    assert (mmap_addr != MAP_FAILED);

    printf("Mapped value %s\n", mmap_addr);

    /* Unmap the file and close it */
    munmap (mmap_addr, file_info.st_size);
    close (fd);
}

void test_rail(void)
{
    rail_t rail = rail_new();
    rail_init(&rail, 0);
    rail_log(&rail);

    rail_free(&rail);
}

#ifdef __cplusplus
}
#endif

#endif
