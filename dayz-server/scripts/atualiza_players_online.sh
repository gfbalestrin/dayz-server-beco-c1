#/bin/bash

source ./config.sh

DISCORD_MESSAGE_ID="$DiscordChannelPlayersOnlineMessageId"
DISCORD_BOT_TOKEN="$DiscordChannelPlayersOnlineBotToken"
DISCORD_CHANNEL_ID="$DiscordChannelPlayersOnlineChannelId"
PLAYERS_BECO_C1_DB="$AppFolder/$AppPlayerBecoC1DbFile"
CURRENT_DATE=$(date "+%d/%m/%Y %H:%M:%S")

PLAYER_ID=$1
EVENT=$2

CONTENT=""

function SincronizaDiscord() {
    URL="https://discord.com/api/v10/channels/$DISCORD_CHANNEL_ID/messages/$DISCORD_MESSAGE_ID"
    echo "Atualizando a mensagem com ID $DISCORD_MESSAGE_ID no canal $DISCORD_CHANNEL_ID..."
    echo "Mensagem: $CONTENT"
    sleep 2
    # Enviar requisição PATCH para editar a mensagem
    response=$(curl -s -X PATCH \
        -H "Authorization: Bot $DISCORD_BOT_TOKEN" \
        -H "Content-Type: application/json" \
        -d "{\"content\": \"$CONTENT\"}" \
        "$URL")

    echo ""
    echo $response
    if [[ "$response" == *"Maximum number of edits to messages"* ]]; then
        sleep 5
        SincronizaDiscord
    fi
}

if [[ "$PLAYER_ID" == "RESET" ]]; then
    CONTENT="**(0/60) Usuários online (atualizado em $CURRENT_DATE)**\n\n"
    SincronizaDiscord

    # SQLITE
    for ((i = 1; i <= 5; i++)); do
        echo "Tentativa $i de exclusão..."

        OUTPUT=$(sqlite3 "$PLAYERS_BECO_C1_DB" "DELETE FROM players_online;" 2>&1)

        if [ $? -eq 0 ]; then
            echo "Registros da tabela 'players_online' excluídos com sucesso."
            break;
        else
            if echo "$OUTPUT" | grep -q "database is locked"; then
                echo "Banco de dados está travado. Tentando novamente em 2 segundos..."
                sleep 2
            else
                echo "Erro inesperado: $OUTPUT"
                break;
            fi
        fi
    done

    exit 0
fi
if [[ "$EVENT" == "CONNECT" || "$EVENT" == "DISCONNECT" ]]; then
    echo "O evento recebido é: $EVENT"
else
    echo "Erro: Parâmetro inválido. Os valores permitidos são 'CONNECT' ou 'DISCONNECT'."
    exit 1
fi

if [[ "$PLAYER_ID" == "" ]]; then
    echo "PlayerID não foi identificado"
    exit 1
fi

# SQLITE
DATACONNECT=$(sqlite3 "$PLAYERS_BECO_C1_DB" "SELECT DataConnect FROM players_online WHERE PlayerID = '$PLAYER_ID';")
# Extrair partes da data manualmente
DIA=$(echo "$CURRENT_DATE" | cut -d'/' -f1)
MES=$(echo "$CURRENT_DATE" | cut -d'/' -f2)
ANO_HORA=$(echo "$CURRENT_DATE" | cut -d'/' -f3)
ANO=$(echo "$ANO_HORA" | cut -d' ' -f1)
HORA=$(echo "$ANO_HORA" | cut -d' ' -f2)
DATA_FORMATADA="${ANO}-${MES}-${DIA} ${HORA}"
if [ -n "$DATACONNECT" ]; then
    if [[ "$EVENT" == "DISCONNECT" ]]; then
        for ((i = 1; i <= 5; i++)); do
            echo "Tentativa $i de exclusao..."
            
            OUTPUT=$(sqlite3 "$PLAYERS_BECO_C1_DB" "DELETE FROM players_online WHERE PlayerId = '$PLAYER_ID';" 2>&1)

            if [ $? -eq 0 ]; then
                echo "Registro da tabela 'players_online' excluído com sucesso."
                break;
            else
                if echo "$OUTPUT" | grep -q "database is locked"; then
                    echo "Banco de dados está travado. Tentando novamente em 2 segundos..."
                    sleep 2
                else
                    echo "Erro inesperado: $OUTPUT"
                    break;
                fi
            fi
        done
    elif [[ "$EVENT" == "CONNECT" ]]; then
        for ((i = 1; i <= 5; i++)); do
            echo "Tentativa $i de atualizacao..."
            
            OUTPUT=$(sqlite3 "$PLAYERS_BECO_C1_DB" "UPDATE players_online SET DataConnect = '$DATA_FORMATADA' WHERE PlayerId = '$PLAYER_ID';" 2>&1)

            if [ $? -eq 0 ]; then
                echo "Registro da tabela 'players_online' atualizado com sucesso."
                break;
            else
                if echo "$OUTPUT" | grep -q "database is locked"; then
                    echo "Banco de dados está travado. Tentando novamente em 2 segundos..."
                    sleep 2
                else
                    echo "Erro inesperado: $OUTPUT"
                    break;
                fi
            fi
        done
    fi
else
    if [[ "$EVENT" == "DISCONNECT" ]]; then
        echo "Ignorando pois PlayerID $PLAYER_ID não estava registrado como online"        

    elif [[ "$EVENT" == "CONNECT" ]]; then
        for ((i = 1; i <= 5; i++)); do
            echo "Tentativa $i de insercao..."
            
            OUTPUT=$(sqlite3 "$PLAYERS_BECO_C1_DB" "INSERT INTO players_online (PlayerId, DataConnect) VALUES ('$PLAYER_ID','$DATA_FORMATADA');" 2>&1)

            if [ $? -eq 0 ]; then
                echo "Registro da tabela 'players_online' inserido com sucesso."
                break;
            else
                if echo "$OUTPUT" | grep -q "database is locked"; then
                    echo "Banco de dados está travado. Tentando novamente em 2 segundos..."
                    sleep 2
                else
                    echo "Erro inesperado: $OUTPUT"
                    break;
                fi
            fi
        done        
        
    fi
fi


NUM_REGISTROS=$(sqlite3 "$PLAYERS_BECO_C1_DB" "SELECT COUNT(*) FROM players_online;")
CONTENT="**($NUM_REGISTROS/60) Usuários online (atualizado em $CURRENT_DATE)**\n\n"
if [[ $NUM_REGISTROS -eq 0 ]]; then
    SincronizaDiscord
    exit 0
fi

while IFS="|" read -r PlayerId PlayerName SteamID SteamName DataConnect
do

    link_steam="**NaoIdentificado**"
    if [[ $SteamID != "" && $SteamName != "" ]]; then
        link_steam="[$SteamName](<https://steamcommunity.com/profiles/$SteamID>)"
    fi
    echo "link_steam: $link_steam"

    player_info="🟢 **$PlayerName** ($link_steam) - $HORA"
    echo "player_info: $player_info"
    CONTENT="${CONTENT}${player_info} \n"
    echo "CONTENT: $CONTENT"
done < <(sqlite3 -separator "|" "$PLAYERS_BECO_C1_DB" "
SELECT 
    p.PlayerId,
    p.PlayerName,
    p.SteamID,
    p.SteamName,
    o.DataConnect
FROM 
    players_online o
INNER JOIN 
    players_database p
ON 
    o.PlayerID = p.PlayerID
ORDER BY 
    o.DataConnect ASC;
")

SincronizaDiscord
