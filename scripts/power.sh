#!/bin/bash

# colors
BLUE="\033[0;34m"
GREEN="\033[1;32m"
WHITE="\033[1;37m"
RED="\033[1;31m"
NO_COLOR="\033[0m"

# help function
usage()
{
	echo ""
	echo "Nvidia Jetson Nano power monitoring"
	echo "Usage: $0 [ -o OUTPUT_FILE ] [ -h ] [ -p ]"
	echo "[ -h ] display the help message."
	echo "[ -p ] to print the values to conole."
	echo "[ -o ] the output file. Defaults to profiling_<sensor>_<timestamp>."
}

# function to exit with error
exit_with_error()
{
	usage
	exit 1
}


PRINT_OUT=0     # print values to console
SENSOR_TYPE=""  # the sensor we wna t to watch
OUTPUT_FILE=""  # output file name

VALID_SENSORS=("ALL" "SOURCE" "CPU" "GPU")

tput clear  # clear the terminal

# options parsing
while getopts ":hpo:" option; do
	case $option in
		p) # print outputs
			PRINT_OUT=1
			;;
		h) # display help
			usage
			exit;;
		o) # output file
			OUTPUT_FILE=${OPTARG}
			;;
		:) # if expected argument omitted
			echo "Error: -${OPTARG} requires an argument"
			exit_with_error
			;;
		*) # any other option
			echo "Error: Invalid option -${OPTARG}. Type -h to se usage."
			exit 1;;
	esac
done

# set default values
if [ -z $OUTPUT_FILE ]; then
	OUTPUT_FILE="profiling_output.csv"  # output file name
	echo -e "${GREEN}Output file is empty. Using default $OUTPUT_FILE.${NO_COLOR}"
	echo ""
fi

# remove all options passed by getopts for normal parameters reading
# shift "$(($OPTIND -1))"

# print some info
echo "$(printf "${GREEN}Watching sensor: ${WHITE}%s${NO_COLOR}" "ALL")"
echo "$(printf "${GREEN}Output file: ${WHITE}%s${NO_COLOR}" "${OUTPUT_FILE}")"
echo ""

# sysfs i2c INA base path
BASE_PATH="/sys/bus/i2c/drivers/ina3221x/6-0040/iio:device0/"

# files paths
declare -A RAILS_NAMES

# rails names
RAIL_NAME_0=$(cat "${BASE_PATH}rail_name_0")
RAIL_NAME_1=$(cat "${BASE_PATH}rail_name_1")
RAIL_NAME_2=$(cat "${BASE_PATH}rail_name_2")

# init headers
header="start_time"
header="${header}; in_current_${RAIL_NAME_0} (mA); in_voltage_${RAIL_NAME_0} (mV); in_power_${RAIL_NAME_0} (mW)"
header="${header}; in_current_${RAIL_NAME_1} (mA); in_voltage_${RAIL_NAME_1} (mV); in_power_${RAIL_NAME_1} (mW)"
header="${header}; in_current_${RAIL_NAME_2} (mA); in_voltage_${RAIL_NAME_2} (mV); in_power_${RAIL_NAME_2} (mW)"
header="${header,,}; duration" 


# write header to output file
(
echo "$header"  # > overides the file. Use >> to append at the end of the file

# start infinite loop
while : ; do
	s_time=$(date +%s.%3N)  # start time

	# prepare the line
	line="${s_time}"

	# read values
	in_current_0=$(cat "${BASE_PATH}in_current0_input")
	in_voltage_0=$(cat "${BASE_PATH}in_voltage0_input")
	in_power_0=$(cat "${BASE_PATH}in_power0_input")

	in_current_1=$(cat "${BASE_PATH}in_current1_input")
	in_voltage_1=$(cat "${BASE_PATH}in_voltage1_input")
	in_power_1=$(cat "${BASE_PATH}in_power1_input")

	in_current_2=$(cat "${BASE_PATH}in_current2_input")
	in_voltage_2=$(cat "${BASE_PATH}in_voltage2_input")
	in_power_2=$(cat "${BASE_PATH}in_power2_input")

	# print values
	# echo ""
	# echo "$(printf "${BLUE}[%s]" "$(date +"%D %T")")"
	# echo "$(printf "${WHITE}%-10s | ${NO_COLOR}Current: %4u mA -- Voltage: %4u mV -- Power: %4u mW" ${RAIL_NAME_0} ${in_current_0} ${in_voltage_0} ${in_power_0})"
	# echo "$(printf "${WHITE}%-10s | ${NO_COLOR}Current: %4u mA -- Voltage: %4u mV -- Power: %4u mW" ${RAIL_NAME_1} ${in_current_1} ${in_voltage_1} ${in_power_1})"
	# echo "$(printf "${WHITE}%-10s | ${NO_COLOR}Current: %4u mA -- Voltage: %4u mV -- Power: %4u mW" ${RAIL_NAME_2} ${in_current_2} ${in_voltage_2} ${in_power_2})"

	# append values to line
	line="${line}; ${in_current_0}; ${in_voltage_0}; ${in_power_0}"
	line="${line}; ${in_current_1}; ${in_voltage_1}; ${in_power_1}"
	line="${line}; ${in_current_2}; ${in_voltage_2}; ${in_power_2}"

	# compute duration
	e_time=$(date +%s.%3N)  # end time
	duration=$(echo "$e_time - $s_time" | bc -l)

	# add duration to line
	line="${line}; ${duration}"
	echo "$line"

done
) > $OUTPUT_FILE # write to file
