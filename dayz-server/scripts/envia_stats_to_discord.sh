#!/bin/bash
CURRENT_DATE=`date "+%d/%m/%Y %H:%M:%S"`
export TZ=America/Sao_Paulo
# Canal Stats-jogadores
WEBHOOK_URL="https://discord.com/api/webhooks/abc123"

discord_bot_file="/home/fastadmin/servers/dayz-server/scripts/extrator_playersdb/discord_bot_update.sh"
players_database="/home/fastadmin/servers/dayz-server/scripts/databases/players_database.csv"
PLAYERS_DB_SQLITE="/home/fastadmin/servers/dayz-server/scripts/databases/players_beco_c1.db"
ranking_infected_killed="/home/fastadmin/servers/dayz-server/scripts/databases/ranking_infected_killed.csv"
ranking_infected_killed_novo="/home/fastadmin/servers/dayz-server/scripts/databases/ranking_infected_killed_novo.csv"
echo "PlayerID;Longest_survivor_hit;Players_killed;Infected_killed;Playtime" > $ranking_infected_killed_novo

compare_infected_killed_script="/home/fastadmin/servers/dayz-server/scripts/extrator_playersdb/compare_infected_killed.sh"

PLAYERSTATS="/home/fastadmin/servers/dayz-server/scripts/databases/players_stats.csv"

declare -A MAP_NAME MAP_STEAMID MAP_STEAMNAME

# Pré-carrega: PlayerID|PlayerName|SteamID|SteamName
while IFS="|" read -r pid pname sid sname; do
  MAP_NAME["$pid"]="$pname"
  MAP_STEAMID["$pid"]="$sid"
  MAP_STEAMNAME["$pid"]="$sname"
done < <(sqlite3 -separator "|" "$PLAYERS_DB_SQLITE" \
          "SELECT PlayerID, PlayerName, SteamID, SteamName FROM players_database;")


# Ordena pelo Longest_survivor_hit
#sort -t';' -k2,2nr $PLAYERSTATS | head -n 10

# Ordena pelo Players_killed
#sort -t';' -k3,3nr $PLAYERSTATS | head -n 10

# Ordena pelo Infected_killed
#sort -t';' -k4,4nr $PLAYERSTATS | head -n 10

# Ordena pelo Playtime
#sort -t';' -k5,5nr $PLAYERSTATS | head -n 10

function FormataEnviaDiscord() {
	temp_file_ordenado=$(mktemp)

	title=""
	id_message=""
	case "$1" in
	"2")
		echo "Ordenando pelo Longest_survivor_hit..."
		sort -t';' -k2,2nr $PLAYERSTATS | head -n 10 > $temp_file_ordenado
		title="🎯 **Ranking dos tiros de maior distância (atualizado em $CURRENT_DATE):**\n\n"
		id_message="1349446312717844490"
	    ;;
	"3")
        	echo "Ordenando pelo Players_killed..."
	        sort -t';' -k3,3nr $PLAYERSTATS | head -n 10 > $temp_file_ordenado
        	title="💀 **Ranking dos maiores assassinos (atualizado em $CURRENT_DATE):**\n\n"
		id_message="1349446329998512138"
	    ;;
	"4")
        	echo "Ordenando pelo Infected_killed..."
	        sort -t';' -k4,4nr $PLAYERSTATS | head -n 10 > $temp_file_ordenado
		cat $temp_file_ordenado >> $ranking_infected_killed_novo
		if [ ! -f $ranking_infected_killed ]; then
			cat $ranking_infected_killed_novo > $ranking_infected_killed
		fi
        	title="🧟 **Ranking de maiores matadores de zumbis (atualizado em $CURRENT_DATE):**\n\n"
		id_message="1349446336793284639"
	    ;;
	"5")
        	echo "Ordenando pelo Playtime..."
	        sort -t';' -k5,5nr $PLAYERSTATS | head -n 10 > $temp_file_ordenado
        	title="⏱️ **Ranking dos players com mais tempo vivo (atualizado em $CURRENT_DATE):**\n\n"
		id_message="1349446347358732420"
	    ;;
	*)
		echo "Opcao desconhecida"
		exit 0
    	;;
	esac

# Criar um arquivo temporário para armazenar os resultados
temp_file=$(mktemp)

# Percorrer as linhas do arquivo players_kills.csv
while IFS=";" read -r playerID longest_survivor_hit players_killed infected_killed playtime; do
    # Ignorar o cabeçalho
    if [[ "$playerID" == "PlayerID" ]]; then
        continue
    fi

    PlayerName="${MAP_NAME[$playerID]}"
    echo $PlayerName
    PlayerSteamId="${MAP_STEAMID[$playerID]}"
    echo $PlayerSteamId
    PlayerSteamName="${MAP_STEAMNAME[$playerID]}"
    echo $PlayerSteamName

    [[ -z "$PlayerName" ]] && PlayerName="NaoIdentificado"

    link_steam="**NaoIdentificado**"
    if [[ -n "$PlayerSteamId" && -n "$PlayerSteamName" ]]; then
        link_steam="[$PlayerSteamName](<https://steamcommunity.com/profiles/$PlayerSteamId>)"
    fi
    player_info="**$PlayerName** ($link_steam)"

    # Se encontrar o PlayerID correspondente, armazenar o resultado no arquivo temporário
    if [[ -n "$player_info" ]]; then
	case "$1" in
	"2")
		metros=$(echo $longest_survivor_hit | cut -d '.' -f 1)
		echo "$player_info, **$metros metros**" >> "$temp_file"
	    ;;
	"3")
		echo "$player_info, **$players_killed vítimas**" >> "$temp_file"
	    ;;
	"4")
		difference=$("$compare_infected_killed_script" "$ranking_infected_killed" "$ranking_infected_killed_novo" "$playerID")
		if [[ "$difference" != "" ]]; then
			echo "$player_info, **$infected_killed $difference zumbis**" >> "$temp_file"
		else
                	echo "$player_info, **$infected_killed zumbis**" >> "$temp_file"
		fi
            ;;
	"5")
		if [[ "$playtime" =~ \. ]]; then
                        playtime=$(echo "$playtime" | cut -d'.' -f1)
                fi
                DAYS=$(($playtime / 86400))        # 1 dia = 86400 segundos
                HOURS=$((($playtime % 86400) / 3600))  # 1 hora = 3600 segundos
                MINUTES=$((($playtime % 3600) / 60))  # 1 minuto = 60 segundos
                SECONDS=$(($playtime % 60))         # Restante dos segundos
                echo "$player_info, **$DAYS dias, $HOURS horas e $MINUTES minutos**" >> "$temp_file"
            ;;

	*)
	        echo "Opcao desconhecida"
        	exit 0
	    ;;
	esac
    fi

done < "$temp_file_ordenado"

CONTENT=$title
i=1
while IFS= read -r LINE; do
	if [ $i -eq 1 ]; then
		CONTENT+="🥇 $LINE\n"
	elif [ $i -eq 2 ]; then
		CONTENT+="🥈 $LINE\n"
	elif [ $i -eq 3 ]; then
                CONTENT+="🥉 $LINE\n"
	else
		CONTENT+="🏅 $LINE\n"
	fi
	i=$((i+1))
done < <(cat "$temp_file" | nl -s '. ' | head -n 10)

CONTENT+="...\n"

CONTENT=$(echo $CONTENT | sed "s/   */ /g")

# Remover o arquivo temporário
rm "$temp_file"
rm "$temp_file_ordenado"

# $1 id do canal, $2 id da mensagem, $3 mensagem
#$discord_bot_file 1349414065574776883 id_mensagem $CONTENT

TOKEN="abc123"
CHANNEL_ID="1349414065574776883"
URL="https://discord.com/api/v10/channels/$CHANNEL_ID/messages/$id_message"

  # Enviar requisição PATCH para editar a mensagem
  response=$(curl -s -X PATCH \
    -H "Authorization: Bot $TOKEN" \
    -H "Content-Type: application/json" \
    -d "{\"content\": \"$CONTENT\"}" \
    "$URL")

}

##########################################
# Ordena pelo Longest_survivor_hit - coluna 2 do csv
FormataEnviaDiscord 2
sleep 5
##########################################
# Ordena pelo Players_killed - coluna 3 do csv
FormataEnviaDiscord 3
sleep 5
##########################################
# Ordena pelo Infected_killed - coluna 4 do csv
FormataEnviaDiscord 4
sleep 5
##########################################
# Ordena pelo Playtime - coluna 5 do csv
FormataEnviaDiscord 5
sleep 5

# Remover o arquivo temporário
rm "$ranking_infected_killed"
mv "$ranking_infected_killed_novo" "$ranking_infected_killed"

chown fastadmin.fastadmin $ranking_infected_killed
