#!/bin/bash
export TZ=America/Sao_Paulo
LOGFILE="./dayz-server/scripts/send_events_to_discord.log"
WEBHOOK_URL="https://discord.com/api/webhooks/********************/************************************"
LOGFILE_NAME="./dayz-server/profiles/DayZServer.ADM"
CURRENT_DATE_BKP=`date "+%Y-%m-%d_%H-%M-%S"`
PLAYERSFILE="./dayz-server/scripts/databases/players_database.csv"
PLAYERSKILLFEEDFILE="./dayz-server/scripts/databases/players_killfeed.csv"
ADMIN_DAYZ_UID="********************"

CURRENT_DATE=`date "+%d/%m/%Y %H:%M:%S"`
echo "$CURRENT_DATE - Monitorando arquivo: $LOGFILE_NAME"  >> $LOGFILE
echo "" >> $LOGFILE

if [ ! -f $PLAYERSKILLFEEDFILE ]; then
        echo "PlayerIDKiller;PlayerIDKilled;Weapon;DistanceMeter;Data;PosKiller;PosKilled" > $PLAYERSKILLFEEDFILE
fi

#./dayz-server/scripts/extrator_playersdb/atualiza_players_online.sh "RESET"

cp $LOGFILE_NAME ./dayz-server/scripts/logs/DayZServer.ADM_$CURRENT_DATE_BKP
echo > $LOGFILE_NAME
tail -f $LOGFILE_NAME | grep --line-buffered -e "is connected" -e "has been disconnected" -e "killed by" -e "is unconscious" -e "AdminLog started on" -e "bled out" -e "died. Stats" -e "Chat(" | while IFS='' read LINE; 
do
    # Remove primeiros 12 caracteres que contem a hora
    CONTENT=$(echo $LINE | cut -c 12-)
    CURRENT_DATE=`date "+%d/%m/%Y %H:%M:%S"`
    echo "-- Inicio $CURRENT_DATE --"  >> $LOGFILE
    echo "CONTENT => $CONTENT"  >> $LOGFILE
    # Chat
    if [[ "$CONTENT" == *"Chat("* ]]; then
	PLAYER_ID=$(echo $CONTENT | awk -F'id=' '{print $2}' | awk -F')' '{print $1}')
	if [ "$PLAYER_ID" == $ADMIN_DAYZ_UID ];then
		echo "Admin digitou no chat!"  >> $LOGFILE
		if [[ "$CONTENT" == *"/admin"* ]]; then
			comando=$(echo "$CONTENT" | sed -n 's|.*\/admin ||p')
			echo "Comando digitado para o admin: $comando" >> $LOGFILE
			echo "$PLAYER_ID $comando" >> ./dayz-server/mpmissions/dayzOffline.chernarusplus/admin_cmds.txt
		elif [[ "$CONTENT" == *"/survivor"* ]]; then
			comando=$(echo "$CONTENT" | sed -n 's|.*\/survivor ||p')
                        echo "Comando digitado para o survivor: $comando" >> $LOGFILE
                        echo "$comando" >> ./dayz-server/mpmissions/dayzOffline.chernarusplus/admin_cmds.txt
		fi
	fi
	continue;
    fi

    # Players online
    if [[ "$CONTENT" == *"is connected"* || "$CONTENT" == *"has been disconnected"* ]]; then
        PLAYER_ID=$(echo $CONTENT | awk -F'id=' '{print $2}' | awk -F')' '{print $1}')
        PLAYER_NAME=$(echo $CONTENT | awk -F'"' '{print $2}')

        if [[ "$PLAYER_ID" == "Unknown" ]]; then
		echo "PlayerId Unknown. Ignorando..."  >> $LOGFILE
                continue;
        fi
	isInFile=$(cat $PLAYERSFILE | grep -c -- "$PLAYER_ID")
	if [ $isInFile -eq 0 ]; then
		echo "Ignorando detalhes do players pois nao consta no banco"
		continue;
	else
		CSV_LINE=$(cat $PLAYERSFILE | grep -- "$PLAYER_ID")
                STEAM_ID=$(echo $CSV_LINE | cut -d ";" -f 3)
		if [ "$PLAYER_ID" == $ADMIN_DAYZ_UID ];then
			echo "Ignorando conta do administrador e matando player para renascer com loot admin..."
			sqlite3 ./dayz-server/mpmissions/dayzOffline.chernarusplus/storage_1/players.db "UPDATE Players set Alive = 0 where UID = '$ADMIN_DAYZ_UID';"
                	continue;
        	fi

                STEAM_NAME=$(echo $CSV_LINE | cut -d ";" -f 4)
		if [[ "$CONTENT" == *"is connected"* ]]; then
			CONTENT="Player **$PLAYER_NAME** ([$STEAM_NAME](<https://steamcommunity.com/profiles/$STEAM_ID>)) is connected"
			./dayz-server/scripts/extrator_playersdb/atualiza_players_online.sh "$PLAYER_ID" "CONNECT" &
		elif [[ "$CONTENT" == *"has been disconnected"* ]]; then
			CONTENT="Player **$PLAYER_NAME** ([$STEAM_NAME](<https://steamcommunity.com/profiles/$STEAM_ID>)) has been disconnected"
			./dayz-server/scripts/extrator_playersdb/atualiza_players_online.sh "$PLAYER_ID" "DISCONNECT" & 
		fi
	fi

    fi
    if [[ "$CONTENT" == *"killed by Player"* ]]; then
		PLAYER_ID_MORREU=$(echo "$CONTENT" | grep -oP 'id=\K[^ ]+' | head -n 1)
		PLAYER_ID_MATOU=$(echo "$CONTENT" | sed -n 's|.*id=\([^ ]*\) pos.*|\1|p')
		WEAPON=$(echo "$CONTENT" | grep -oP 'with \K\w+')
		DISTANCE=$(echo "$CONTENT" | grep -oP 'from \K\d+\.\d+')
		POSITIONS=($(echo "$CONTENT" | grep -oE 'pos=<[^>]+>'))
		POS_KILLER="${POSITIONS[0]}"
		POS_KILLED="${POSITIONS[1]}"
		echo "$PLAYER_ID_MATOU;$PLAYER_ID_MORREU;$WEAPON;$DISTANCE;$CURRENT_DATE;$POS_KILLER;$POS_KILLED" >> $PLAYERSKILLFEEDFILE
    fi

    CONTENT=$(echo $CONTENT | sed -e 's/arted on/Server restarted/g')

    if [[ "$CONTENT" == *"Server restarted"* ]]; then
	CONTENT="Server successfully restarted!"
    fi


    # Remove aspas
    CONTENT=$(echo $CONTENT | sed -e 's|["'\'']||g')
    # Remove parenteses e seu conteudo
    CONTENT=$(echo $CONTENT | sed "s/[(][id=.+][^)]*[)]//g")
    # Remove multiplos espacos
    CONTENT=$(echo $CONTENT | sed "s/   */ /g")

    echo $CONTENT

    payload=$(cat <<EOF 
{ "content": "$CURRENT_DATE - $CONTENT" } 
EOF
)


    curl -s -H "Content-Type: application/json" -X POST -d "$payload" $WEBHOOK_URL

    CURRENT_DATE=`date "+%d/%m/%Y %H:%M:%S"`
    echo "-- Fim $CURRENT_DATE --"  >> $LOGFILE
done


