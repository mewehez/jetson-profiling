/*
 * @file argparse.h
 * @brief Functions prototypes for the command line parser.
 *
 * This file contains the prototypes for the command line parser,
 * the macros, constant and global variables needed.
 *
 * @author Mewe-Hezoudah KAHANAM
 * @bug No known bugs.
 */

#ifndef __ARGPARSE_H__
#define __ARGPARSE_H__

#ifdef __cplusplus // c++ compatibility
extern "C"
{
#endif

/**
 * Type bool to use boolean style in c.
 */
#define bool unsigned int
/**
 * Boolean true value.
 */
#define true  1
/**
 * Boolean false value.
 */
#define false 0
/**
 * First position of the command line arguments to read.
 */
#define FIRST_ARG_POS 1

/**
 * Defines the type of the argument to be parsed.
 * It is used to get the value in specific type 
 * instead of generic pointer void*.
 */
typedef enum
{
  ARG_OPT_BOOLEAN=0, /**< Boolean value */
  ARG_OPT_INTEGER,   /**< Integer value */
  ARG_OPT_FLOAT,     /**< Floating point value */
  ARG_OPT_STRING     /**< String value */
} arg_type;

/**
 * Represents command line's optional values.
 */
typedef struct
{
  arg_type type;      /**< The data type of the argument */
  char short_name;    /**< The short name of the argument. Exemple `h` */
  char* name;         /**< The long name of the argument. Exemple `help` */
  void* value;        /**< Pointer to the value of the argument */
  void* default_val;  /**< Default value if not supplied by the user */
} arg_option;

/**
 * Represents the parsed options from the command line inputs.
 */
typedef struct
{
  arg_option* options; /**< The options to be parsed */
  int size;            /**< The number of options */
} command_line;

/**
 * Macro to create boolean option.
 */
#define OPT_BOOLEAN(...) { ARG_OPT_BOOLEAN, __VA_ARGS__ }
/**
 * Macro to create intger option.
 */
#define OPT_INTEGER(...) { ARG_OPT_INTEGER, __VA_ARGS__ }
/**
 * Macro to create float option.
 */
#define OPT_FLOAT(...)   { ARG_OPT_FLOAT, __VA_ARGS__ }
/**
 * Macro to create string option.
 */
#define OPT_STRING(...)  { ARG_OPT_STRING, __VA_ARGS__ }

/**
 * Allocates memory to store the values in the argument options.
 * 
 * Memory is allocated for all types except string type, 
 * for which the memory is allocated at the assignement.
 * The command line should be freed using `free_command_line`.
 *
 * @param cmd The command line data structure.
 * @return A status code. 0 if everything is good, else -1.
 */
int init_command_line(command_line* cmd);

/**
 * Free dynamically allocated memory in the options.
 * 
 * @param cmd The command line data structure.
 * @return A status code. 0 if everything is good, else -1.
 */
int free_command_line(const command_line* cmd);

/**
 * Parses the options in the command line to the options.
 * 
 * @param cmd The command line data structure.
 * @return A status code. 0 if everything is good, else -1.
 */
int parse_command_line(const command_line* cmd, int argc, char** argv);

/**
 * Get the option by it's name. 
 *
 * @param cmd The command line data structure.
 * @param argc The number of arguments passed to the program.
 * @param argv The list of arguments passed to the command line.
 * @return A pointer to the option foud or NULL if not found.
 */
arg_option* get_option_by_name(const command_line* cmd, const char* arg_name);

/**
 * Gets the value of an option given it's name.
 *
 * @param cmd The command line data structure.
 * @param arg_name The name of the option.
 * @return A generic pointer if found, else NULL.
 */
void* get_option_value(const command_line* cmd, const char* arg_name);

#ifdef __cplusplus // c++ compatibility
}
#endif

#endif