/**
 * @file argparse.c
 * @brief Prototype functions implementations and helpers.
 *
 * @author Mewe-Hezoudah KAHANAM
 * @bug No known bugs.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "argparse.h"

/**
 * Parses integer value into command line option. 
 *
 * @param option the option to parse.
 * @param value the value to parse into option.
 * @return A status code. 0 if everything is good, else -1.
 */
static int parse_integer(arg_option* option, int value);

/**
 * Parses floating point value into command line option. 
 *
 * @param option the option to parse.
 * @param value the value to parse into option.
 * @return A status code. 0 if everything is good, else -1.
 */
static int parse_float(arg_option* option, float value);

/**
 * Parses string value into command line option. 
 *
 * @param option the option to parse.
 * @param value the value to parse into option.
 * @return A status code. 0 if everything is good, else -1.
 */
static int parse_string(arg_option* option, char* value);

/**
 * Parses boolean value into command line option. 
 *
 * @param option the option to parse.
 * @param value the value to parse into option.
 * @return A status code. 0 if everything is good, else -1.
 */
static int parse_boolean(arg_option* option, bool value);

/**
 * Checks that all the options in command line have correct types.
 *
 * @param options The list of options to check.
 * @return A status code. 0 if everything is good, else -1.
 */
static int check_command_line(const command_line* cmd)
{
  arg_option option;
  for(int i = 0; i < cmd->size; i++)
  {
    option = cmd->options[i];
    switch(option.type)
    {
      case ARG_OPT_BOOLEAN:
        break;
      case ARG_OPT_INTEGER:
        break;
      case ARG_OPT_FLOAT:
        break;
      case ARG_OPT_STRING:
        break;
      default: // incorrect type
        printf("[Error] Invalid data type found in options.\n");
        return -1;
    }
  }
  return 0;
}

/**
 * Splits a given argument into it's name and value if possible.
 * Example --port=8080 will give port and 8080. The name and value are 
 * set into the pointers.
 * 
 * @param arg       The argument we want to split.
 * @param arg_name  The resulting name after the split.
 * @param arg_value The resulting value after the split.
 * @return A status code. 0 if everything is good, else -1.
 */
static int split_arg(const char* arg, char** arg_name, char** arg_value)
{
  if(strcmp(arg, "--") == 0 || strcmp(arg, "-") == 0)
    return -1;
  
  // if positional argument
  if(arg[0] != '-')
  {
    // free arg name
    free(*arg_name); 
    *arg_name = NULL;
    // assign arg value
    *arg_value = (char *) calloc(strlen(arg), sizeof(char)); 
    strncpy(*arg_value, arg, strlen(arg));
    return 0;
  }

  char* buff = NULL;
  int buff_size = 0;
  // if true short name, else long name
  int index = (arg[0] == '-' && arg[1] != '-') ? 1 : 2;

  // copy substring
  buff_size = strlen(arg)-(index - 1);
  buff = (char *) calloc(buff_size, sizeof(char));
  strncpy(buff, arg+index, buff_size);

  // split name from value
  char* sep = strchr(buff, '=');
  if(sep)
  {
    index = (int)(sep - buff)+1;
    buff_size = strlen(buff) - index;
    // if we have characters to copy
    if(buff_size > 0)
    {
      *arg_value = (char *) calloc(buff_size, sizeof(char));  
      strncpy(*arg_value, buff+(index), buff_size);
    }
    else
    {
      // free memory before
      free(*arg_value); 
      *arg_value = NULL;  
    }
    // get name
    *arg_name = (char *) calloc(index, sizeof(char));
    strncpy(*arg_name, buff, index-1);
  }
  else
  {  // only name is suplied
    *arg_name = (char *) calloc(buff_size, sizeof(char));
    strncpy(*arg_name, buff, buff_size);
    // free memory before
    free(*arg_value); 
    *arg_value = NULL;
  }
  free(buff);  // free manually allocated memory
  return 0;
}

/**
 * Parses value into command line option. 
 *
 * @param option the option to parse.
 * @param arg_value the value to parse into option.
 * @return A status code. 0 if everything is good, else -1.
 */
static int parse_value(arg_option* option, char* arg_value)
{
  if(!option)
    return -1;
  
  if(option->type == ARG_OPT_BOOLEAN)
  {
    return parse_boolean(option, true);
  }
  else if(!arg_value)
    return 0;
  else 
  {
    switch(option->type)
    {
      case ARG_OPT_INTEGER:
        return parse_integer(option, atoi(arg_value));
      case ARG_OPT_FLOAT:
        return parse_float(option, atof(arg_value));
      case ARG_OPT_STRING:
        return parse_string(option, arg_value);
      default:
        printf("[Error] Unknown data type\n");
        return -1;
    }
  }
}

void* get_option_value(const command_line* cmd, const char* arg_name)
{
  arg_option* option = get_option_by_name(cmd, arg_name);
  if(!option)
    return NULL;
  return option->value;
}

int free_command_line(const command_line* cmd)
{
  arg_option option;
  for(int i = 0; i < cmd->size; i++)
  {
    option = cmd->options[i];
    free(option.value);
  }
  return 0;
}

arg_option* get_option_by_name(const command_line* cmd, const char* arg_name)
{
  arg_option* option;
  for(int i = 0; i < cmd->size; i++)
  {
    option = &cmd->options[i];
    if(strcmp(option->name, arg_name) == 0 || strcmp(&option->short_name, arg_name) == 0)
      return option;
  }
  return NULL;
}

int parse_command_line(const command_line* cmd, int argc, char** argv)
{
  if(check_command_line(cmd) < 0)
    return -1;
  
  int status = 0; // status returned by helper functions
  // previous parsed argument has value and or name?
  // used to track optional args
  bool prev_has_name  = false;

  char* arg_name  = NULL; // argument name after split
  char* arg_value = NULL; // argument value after split

  // arg_option* options = cmd->options;
  arg_option* option;
  for(int i = FIRST_ARG_POS; i < argc; i++)
  {
    if(split_arg(argv[i], &arg_name, &arg_value) < 0)
    {
      printf("[Error] Unable to split argument %s\n", argv[i]);
      return -1;
    }

    // if we started optional args, we don't want to encounter 
    // positionl args
    if(prev_has_name && !arg_name)
    {
      printf("[Error] Could not parse argument %s\n", arg_value);
      return -1;
    }
    // parse the splited values
    option = get_option_by_name(cmd, arg_name);
    parse_value(option, arg_value);

    // check if we started optional args
    prev_has_name  = (arg_name)  ? true : false;

    // free manually allocated memory
    free(arg_name);
    free(arg_value); 
    arg_name = NULL;
    arg_value = NULL;
  }
  return 0;
}

static int parse_integer(arg_option* option, int value)
{
  option->value = (int*) calloc(1, sizeof(int));
  *(int*)option->value = value;
  return 0;
}

static int parse_boolean(arg_option* option, bool value)
{
  option->value = (bool*) calloc(1, sizeof(bool));
  *(bool*)option->value = value;
  return 0;
}

static int parse_float(arg_option* option, float value)
{
  option->value = (float*) calloc(1, sizeof(float));
  *(float*)option->value = value;
  return 0;
}

static int parse_string(arg_option* option, char* value)
{
  int size = strlen(value);
  option->value = (char *) calloc(size, sizeof(char));
  strncpy((char*)option->value, value, size);
  return 0;
}
