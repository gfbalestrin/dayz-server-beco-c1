#!/bin/bash

export TZ=America/Sao_Paulo
LOGFILE="./dayz-server/scripts/send_events_infos_to_discord.log"
WEBHOOK_URL="https://discord.com/api/webhooks/*********************/******************************"
WEBHOOK_ADMIN_URL="https://discord.com/api/webhooks/***********************/*************************************"
LOGFILE_NAME=`ls -lh ./dayz-server/profiles/*.RPT | tail -1 | rev | cut -d " " -f 1 | rev`
CURRENT_DATE_BKP=`date "+%Y-%m-%d_%H-%M-%S"`
PLAYERSFILE="./dayz-server/scripts/databases/players_database.csv"

CURRENT_DATE=`date "+%d/%m/%Y %H:%M:%S"`
echo "$CURRENT_DATE - Monitorando arquivo: $LOGFILE_NAME"  >> $LOGFILE
echo "" >> $LOGFILE

if [ ! -f $PLAYERSFILE ]; then
	echo "PlayerID;PlayerName;SteamID;SteamName" > $PLAYERSFILE
fi

cp -Rap $LOGFILE_NAME ./dayz-server/scripts/logs/DayZServer.RPT_$CURRENT_DATE_BKP
echo > $LOGFILE_NAME

tail -n +1 -f $LOGFILE_NAME | grep -n --line-buffered -e "is connected" -e "Shutting down in 60 seconds" -e "Invalid number -nan" | while IFS='' read LINE
do
    CURRENT_DATE=`date "+%d/%m/%Y %H:%M:%S"`
    echo "-- Inicio $CURRENT_DATE --"  >> $LOGFILE
    CONTENT=$(echo $LINE | cut -c 13-)
    echo "LINE => $LINE" >> $LOGFILE

    if [[ "$LINE" == *"Invalid number"* ]]; then
        LINE_NUMBER_ERROR=$(echo $LINE | cut -d ":" -f 1)
	UID_FOUND=$(head -n $((LINE_NAN - 1)) "$LOGFILE_NAME" | tac | awk '
  		$0 ~ /uid uid [^ ]{44}/ {
    			match($0, /uid [A-Za-z0-9+/=]{44}=/)
    			print substr($0, RSTART + 4, RLENGTH - 4)
    			exit
  		}
	')
	 CONTENT="Player bugado: $UID_FOUND" 
	 echo $CONTENT >> $LOGFILE
	  payload=$(cat <<EOF
{ "content": "$CURRENT_DATE - $CONTENT" }
EOF
)
                curl -s -H "Content-Type: application/json" -X POST -d "$payload" $WEBHOOK_ADMIN_URL

	 continue
    fi

    if [[ "$CONTENT" == *"Shutting down in 60 seconds"* ]]; then
	./dayz-server/scripts/extrator_playersdb/extrai_players_stats.sh &
        ./dayz-server/scripts/extrator_playersdb/monta_killfeed_geral.sh &
        ./dayz-server/scripts/extrator_playersdb/atualiza_players_online.sh "RESET" &
        	CONTENT="Server restarting..."
        payload=$(cat <<EOF
{ "content": "$CURRENT_DATE - $CONTENT" }
EOF
)
       		curl -s -H "Content-Type: application/json" -X POST -d "$payload" $WEBHOOK_URL
		continue
    fi

    LINE_NUMBER_STEAM=$(echo $LINE | cut -d ":" -f 1)
    if [[ "$CONTENT" == *"is connected"* ]]; then
    	#LINE_NUMBER_STEAM=$(grep -n "steamID=" $LOGFILE_NAME  | tail -1 | cut -d: -f1)
		LINE_NUMBER_ID=$((LINE_NUMBER_STEAM - 1))
		echo "CONTENT => $CONTENT" >> $LOGFILE
		echo "LINE_NUMBER_STEAM => $LINE_NUMBER_STEAM" >> $LOGFILE
		echo "LINE_NUMBER_ID => $LINE_NUMBER_ID" >> $LOGFILE

    	if [[ "$CONTENT" == *"steamID="* ]]; then
    		STEAMID=`echo $CONTENT | cut -d "=" -f 2 | cut -d ")" -f 1`
			echo "STEAMID => $STEAMID" >> $LOGFILE
    	fi
    	
    	if [ "$STEAMID" != "" ];then
    		PLAYER_NAME_STEAM=`curl -L -s https://steamcommunity.com/profiles/$STEAMID | grep actual_persona_name | grep -v "&nbsp;" | sed 's:</span>:\n:g' | sed -n 's/.*>//p' | sed 's/[^a-zA-Z0-9_ -]//g' | xargs`
		
			PLAYER_NAME_STEAM=$(echo $PLAYER_NAME_STEAM | sed "s/;//g")
			PLAYER_NAME_STEAM=$(echo $PLAYER_NAME_STEAM | sed "s/#//g")
			echo "PLAYER_NAME_STEAM => $PLAYER_NAME_STEAM" >> $LOGFILE
    	fi

    	if [ "$PLAYER_NAME_STEAM" == "" ];then
    		PLAYER_NAME_STEAM="Unknown"
    	fi

    	if [ "$STEAMID" != "" ];then
        	CONTENT="$CONTENT - Steam=$PLAYER_NAME_STEAM - <https://steamcommunity.com/profiles/$STEAMID>"
    	else
        	CONTENT="$CONTENT - Steam=Unknown"
    	fi
		# Captura linha anterior
        LINE=$(awk "NR==$LINE_NUMBER_ID" $LOGFILE_NAME)
        # Remove primeiros 9 caracteres que contem a hora
        CONTENT=$(echo $LINE | cut -c 9-)
		echo "CONTENT_LINHA_ANTERIOR => $CONTENT" >> $LOGFILE

        PLAYER_ID=$(echo $CONTENT | grep -oP 'id=\K[A-Za-z0-9_=-]+')
        PLAYER_NAME=$(echo $CONTENT | grep -oP '(?<=Player ).*?(?= \(id=)' )
		PLAYER_NAME=$(echo $PLAYER_NAME | sed "s/;//g")
		PLAYER_NAME=$(echo $PLAYER_NAME | sed "s/#//g")
		echo "PLAYER_ID => $PLAYER_ID" >> $LOGFILE
		echo "PLAYER_NAME => $PLAYER_NAME" >> $LOGFILE

        CSV_LINE="$PLAYER_ID;$PLAYER_NAME;$STEAMID;$PLAYER_NAME_STEAM"
		echo "CSV_LINE => $CSV_LINE" >> $LOGFILE
		isInFile=$(cat $PLAYERSFILE | grep -c -- "$PLAYER_ID")
        if [ $isInFile -eq 0 ]; then
		echo "Inserindo CSV_LINE no arquivo $PLAYERSFILE e enviando para discord" >> $LOGFILE
                echo $CSV_LINE >> $PLAYERSFILE
		CONTENT="Player **$PLAYER_NAME** ([$PLAYER_NAME_STEAM](<https://steamcommunity.com/profiles/$STEAMID>)) is connected"
    payload=$(cat <<EOF
{ "content": "$CURRENT_DATE - $CONTENT" }
EOF
)
       curl -s -H "Content-Type: application/json" -X POST -d "$payload" $WEBHOOK_URL

		./dayz-server/scripts/extrator_playersdb/atualiza_players_online.sh "$PLAYER_ID" "CONNECT" &
        else
			CSV_LINE_OLD=$(cat $PLAYERSFILE | grep -- "$PLAYER_ID")
			sed -i "s#${CSV_LINE_OLD}#${CSV_LINE}#g" $PLAYERSFILE
			./dayz-server/scripts/extrator_playersdb/atualiza_players_online.sh "$PLAYER_ID" "CONNECT" &

			echo "Atualizando CSV_LINE no arquivo $PLAYERSFILE..." >> $LOGFILE
			echo "CSV_LINE_OLD => $CSV_LINE_OLD" >> $LOGFILE
			CSV_LINE_UPDATED=$(cat $PLAYERSFILE | grep -- "$PLAYER_ID")
			echo "CSV_LINE_UPDATED => $CSV_LINE_UPDATED" >> $LOGFILE
        fi
     fi


     CURRENT_DATE=`date "+%d/%m/%Y %H:%M:%S"`
     echo "-- Fim $CURRENT_DATE --"  >> $LOGFILE
done
