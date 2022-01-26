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
	echo "Usage: $0 [ -s SENSOR_TYPE ] [ -o OUTPUT_FILE ] [ -h ] [ -p ]"
	echo "[ -h ] display the help message."
	echo "[ -p ] to print the values to conole."
	echo "[ -o ] the output file. Defaults to profiling_<sensor>_<timestamp>."
	echo "[ -s ] the sensor we want to watch. Use one of ALL, CPU, GPU, SOURCE (input power sensor);"
	echo "defaults to ALL."
}

# function to exit with error
exit_with_error()
{
	usage
	exit 1
}

# function to print values to output
print_out()
{
	if [[ $# -ne 0 && PRINT_OUT -eq 1 ]]; then
		echo "$1"  # use double quote to preserve printf format
	fi
}


PRINT_OUT=0     # print values to console
SENSOR_TYPE=""  # the sensor we wna t to watch
OUTPUT_FILE=""  # output file name

VALID_SENSORS=("ALL" "SOURCE" "CPU" "GPU")

tput clear  # clear the terminal

# options parsing
while getopts ":hpo:s:" option; do
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
		s) # sensor to watch
			if [[ "${VALID_SENSORS[*]}" != *"$OPTARG"* ]]; then
				echo -e "${RED}Error: Inalid value for SENSOR_TYPE${NO_COLOR}"
				exit_with_error
			fi
			SENSOR_TYPE=${OPTARG}
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
if [[ -z $SENSOR_TYPE || -z $OUTPUT_FILE ]]; then
	echo ""
	if [ -z $SENSOR_TYPE ]; then
		SENSOR_TYPE="ALL"
		echo -e "${GREEN}Sensor is empty. Using default $SENSOR_TYPE.${NO_COLOR}"
	fi

	if [ -z $OUTPUT_FILE ]; then
		OUTPUT_FILE="profiling_${SENSOR_TYPE}_$(date +%s)"  # output file name
		echo -e "${GREEN}Output file is empty. Using default $OUTPUT_FILE.${NO_COLOR}"
	fi
	echo ""
fi

# remove all options passed by getopts for normal parameters reading
shift "$(($OPTIND -1))"

# build file paths
RAILS_IDS=()
case $SENSOR_TYPE in
	ALL) # all sensors
		RAILS_IDS=(0 1 2)
		;;
	SOURCE) # input power
		RAILS_IDS=(0)
		;;
	GPU) # GPU sensor
		RAILS_IDS=(1)
		;;
	CPU) # CPU sensor
		RAILS_IDS=(2)
		;;
esac

# print some info
print_out ""
print_out "$(printf "${GREEN}Watching sensor: ${WHITE}%s${NO_COLOR}" "${SENSOR_TYPE}")"
print_out "$(printf "${GREEN}Output file: ${WHITE}%s${NO_COLOR}" "${OUTPUT_FILE}")"
print_out ""

# sysfs i2c INA base path
BASE_PATH="/sys/bus/i2c/drivers/ina3221x/6-0040/iio:device0/"

# files paths
declare -A RAILS_NAMES

# init files paths
header="start_time"
for id in "${RAILS_IDS[@]}"; do
	RAILS_NAMES[$id]=$(cat "${BASE_PATH}rail_name_${id}")
	header="${header}; in_current_${RAILS_NAMES[$id]} (mA); in_voltage_${RAILS_NAMES[$id]} (mV); in_power_${RAILS_NAMES[$id]} (mW)"
	# echo "Rail name: ${RAILS_NAMES[$id]}"
	# echo "[Current: ${IN_CURRENTS[$id]}mA | Voltage: ${IN_VOLTAGES[$id]}mV | Power: ${IN_POWERS[$id]}mW]"
done
header="${header,,}; duration" 
# write header to output file
echo $header > $OUTPUT_FILE  # > overides the file. Use >> to append at the end of the file

tput sc  # save cursor position
# start infinite loop
while : ; do
	tput rc  # restore cursor position
	s_time=$(date +%s.%N)  # start time

	# prepare the line
	line="${s_time}"

	# loop trhough sensors
	for id in "${RAILS_IDS[@]}"; do
		in_current=$(cat "${BASE_PATH}in_current${id}_input")
		in_voltage=$(cat "${BASE_PATH}in_voltage${id}_input")
		in_power=$(cat "${BASE_PATH}in_power${id}_input")
		line="${line}; ${in_current}; ${in_voltage}; ${in_power}"
		print_out "$(printf "${BLUE}[%s] ${WHITE}%-10s | ${NO_COLOR}Current: %4u mA -- Voltage: %4u mV -- Power: %4u mW" "$(date +"%D %T")" ${RAILS_NAMES[$id]} ${in_current} ${in_voltage} ${in_power})"
		# print_out "$log"
		# print_out "[$(date +"%D %T")] ${RAILS_NAMES[$id]} | Current: ${in_current} mA -- Voltage: ${in_voltage} mV -- Power: ${in_power}mW"
	done
	e_time=$(date +%s.%N)  # end time
	duration=$(echo "$e_time - $s_time" | bc -l)
	line="${line}; ${duration}"

	# write line to file
	echo $line >> $OUTPUT_FILE
done

