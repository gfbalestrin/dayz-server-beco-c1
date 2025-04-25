#!/bin/bash

# apt install xxd bc

PLAYER_UID=$1
DB_PATH="./dayz-server/mpmissions/dayzOffline.chernarusplus/storage_1/"
DB_FILENAME=players.db

FILE_PLAYER=/tmp/player.txt

cd $DB_PATH
sqlite3 $DB_FILENAME "SELECT hex(Data) FROM Players where UID = '$PLAYER_UID';" > $FILE_PLAYER

player=$(</tmp/player.txt)

length=${#player}

bytes_dbversion=${player:0:4}
echo "DB Version - $bytes_dbversion"

hex_position_x=${player:4:8}
float=$(echo $hex_position_x | xxd -r -p | od -An -t fF | tr -d ' ')
float_position_x=$float
echo "Position X - $hex_position_x => $float"

hex_position_z=${player:12:8}
float=$(echo $hex_position_z | xxd -r -p | od -An -t fF | tr -d ' ')
echo "Position Z - $hex_position_z => $float"

hex_position_y=${player:20:8}
float=$(echo $hex_position_y | xxd -r -p | od -An -t fF | tr -d ' ')
float_position_y=$float
echo "Position Y - $hex_position_y => $float"

echo "https://dayz.xam.nu/#location=""$float_position_x"";""$float_position_y"";5"

