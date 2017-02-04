#!/bin/bash
BASE_PATH=/opt/mz-bicycle-commuter

RESET_FILE=$BASE_PATH/reset
if [ -f $RESET_FILE ]; then
    echo "Reset file present! Script will exit now."
    exit 0
fi

LEFT_FILE=$BASE_PATH/left.count
RIGHT_FILE=$BASE_PATH/right.count

if [ ! -f $LEFT_FILE ] || [ ! -f $RIGHT_FILE ]; then
    echo "Count file not found!"
    exit 1
fi

declare -i left=$(cat $LEFT_FILE)
declare -i right=$(cat $RIGHT_FILE)

JSON_MSG=\{left:\ $left\,\ right\:\ $right\}

echo "###########################################"
echo "# will transmit \""$JSON_MSG"\" now"
echo "###########################################"

timeout 10 cat < /dev/ttyUSB0 &

MESSAGE=`echo -n $JSON_MSG | od -A n -t x1 | tr -d ' \r\n' `

stty -F /dev/ttyUSB0 57600
echo -ne "mac join abp\r\n" | tee /dev/ttyUSB0
sleep 1;
echo -ne "mac tx uncnf 1 $MESSAGE\r\n" | tee /dev/ttyUSB0

wait

echo "touch reset file"
touch $RESET_FILE

echo "###########################################"
