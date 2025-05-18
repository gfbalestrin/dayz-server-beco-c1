#!/bin/bash

# Carrega as variáveis

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PARENT_DIR="$(dirname "$SCRIPT_DIR")"
cd "$PARENT_DIR"
source ./config.sh

ScriptName=$(basename "$0")

COMMAND_FILE="$DayzServerFolder/$DayzActionsToExecuteFile"

echo "Monitorando comandos do DayZ em $COMMAND_FILE..."
INSERT_CUSTOM_LOG "Monitorando arquivo: $COMMAND_FILE" "INFO" "$ScriptName"

echo > "$COMMAND_FILE"

tail -F "$COMMAND_FILE" | while read -r line; do
    # Valida se é um JSON válido
    if ! echo "$line" | jq empty 2>/dev/null; then
        echo ">> Linha inválida (não é JSON): $line"
        continue
    fi

    # Extrai o campo "action"
    action=$(echo "$line" | jq -r '.action')

    case "$action" in
        reset_password)
            player_id=$(echo "$line" | jq -r '.player_id')

            echo ">> Resetando senha de $player_id"
            INSERT_CUSTOM_LOG "Resetando senha de $player_id"

            # Caminho absoluto para garantir que funcione via systemd
            result_json=$("$AppFolder/$AppScriptPlayerLoadoutManagerFile" --player-id "$player_id" --reset-password 2>&1)

            # Verifica se a saída é JSON válido
            if ! echo "$result_json" | jq -e . >/dev/null 2>&1; then
                echo "Erro: saída inválida do script de reset:"
                echo "$result_json"
                echo "$player_id;[ERROR] Erro interno ao resetar senha (formato inválido)" >> "$DayzServerFolder/$DayzMessagesPrivateToSendoFile"
                continue
            fi

            # Verifica erro no JSON retornado
            if echo "$result_json" | jq -e 'has("error")' >/dev/null; then
                erro=$(echo "$result_json" | jq -r '.error')
                echo "Erro do script: $erro"
                echo "$player_id;[ERROR] Erro ao resetar a senha: $erro" >> "$DayzServerFolder/$DayzMessagesPrivateToSendoFile"
                continue
            fi

            login=$(echo "$result_json" | jq -r '.login // empty')
            senha=$(echo "$result_json" | jq -r '.senha // empty')
            url=$(echo "$result_json" | jq -r '.url // empty')

            echo ">> Senha redefinida com sucesso para o jogador $player_id"
            echo "Login: $login"
            echo "Senha: $senha"
            echo "URL: $url"

            echo "$player_id;Nova senha gerada com sucesso. Acesse $url" >> "$DayzServerFolder/$DayzMessagesPrivateToSendoFile"
            echo "$player_id;Nova senha: $senha" >> "$DayzServerFolder/$DayzMessagesPrivateToSendoFile"
            ;;

        active_loadout)
            player_id=$(echo "$line" | jq -r '.player_id')
            loadout_name=$(echo "$line" | jq -r '.loadout_name')
            echo ">> Ativando loadout de $player_id para $loadout_name"
            INSERT_CUSTOM_LOG "Ativando loadout de $player_id para $loadout_name" "INFO" "$ScriptName"

            result_json=$("$AppFolder/$AppScriptPlayerLoadoutManagerFile" --player-id "$player_id" --loadout-name "$loadout_name" --active)

            if echo "$result_json" | jq -e 'has("error")' >/dev/null; then
                erro=$(echo "$result_json" | jq -r '.error')
                echo "Erro ao ativar loadout: $erro"
                echo "$player_id;[ERROR] Erro ao ativar o loadout '$loadout_name': $erro" >> "$DayzServerFolder/$DayzMessagesPrivateToSendoFile"
                continue
            fi

            msg=$(echo "$result_json" | jq -r '.message')
            echo ">> Loadout ativado com sucesso: $msg"
            echo "$player_id;$msg" >> "$DayzServerFolder/$DayzMessagesPrivateToSendoFile"
            ;;

        restart_server)
            minutes=$(echo "$line" | jq -r '.minutes')
            message=$(echo "$line" | jq -r '.message')
            echo ">> Reinício do servidor em $minutes minuto(s): $message"
            INSERT_CUSTOM_LOG "Servidor será reiniciado por votação" "INFO" "$ScriptName"

            # Validação mínima
            if ! [[ "$minutes" =~ ^[0-9]+$ ]] || [[ "$minutes" -le 0 ]]; then
                echo ">> Valor inválido para minutos: $minutes"
                continue
            fi

            echo "[ERROR] Atenção: o servidor será reiniciado em $minutes minuto(s)!" >> "$DayzServerFolder/$DayzMessagesToSendoFile"
            sleep 60
            sudo systemctl restart dayz-server
            ;;
        
        update_player)
            PlayerId=$(echo "$line" | jq -r '.player_id')
            PlayerName=$(echo "$line" | jq -r '.player_name')
            PlayerSteamId=$(echo "$line" | jq -r '.steam_id')
            
            echo ">> Atualizando jogador na player_database: $PlayerId"

            PlayerSteamName=$(curl -L -s https://steamcommunity.com/profiles/$PlayerSteamId | grep actual_persona_name | grep -v "&nbsp;" | sed 's:</span>:\n:g' | sed -n 's/.*>//p' | sed 's/[^a-zA-Z0-9_ -]//g' | xargs)
            PlayerSteamName=$(echo $PlayerSteamName | sed "s/[^a-zA-Z0-9_-]//g")
            if [ "$PlayerSteamName" == "" ]; then
				PlayerSteamName="Unknown"
			fi

            PlayerExists=$(sqlite3 -separator "|" "$AppFolder/$AppPlayerBecoC1DbFile" "SELECT PlayerName, SteamID, SteamName FROM players_database WHERE PlayerID = '$PlayerId';")
			if [[ -z "$PlayerExists" ]]; then
				INSERT_CUSTOM_LOG "Player não consta no banco. O player será inserido no banco de dados." "INFO" "$ScriptName"
				INSERT_PLAYER_DATABASE "$PlayerId" "$PlayerName" "$PlayerSteamId" "$PlayerSteamName"
                Content="Jogador **$PlayerName** ([$PlayerSteamName](<https://steamcommunity.com/profiles/$PlayerSteamId>)) conectou"
				SEND_DISCORD_WEBHOOK "$Content" "$DiscordWebhookLogs" "$CurrentDate" "$ScriptName"
				"$AppFolder/$AppScriptUpdatePlayersOnlineFile" "$PlayerId" "CONNECT" 		
				continue
			fi

            PlayerNameCurrent=$(echo "$PlayerExists" | cut -d'|' -f1)
			PlayerSteamIdCurrent=$(echo "$PlayerExists" | cut -d'|' -f2)
			PlayerSteamNameCurrent=$(echo "$PlayerExists" | cut -d'|' -f3)
			INSERT_CUSTOM_LOG "Player já consta no banco. O player será atualizado no banco de dados." "INFO" "$ScriptName"
			UPDATE_PLAYER_DATABASE "$PlayerId" "$PlayerName" "$PlayerSteamId" "$PlayerSteamName"
			if [[ "$PlayerNameCurrent" != "$PlayerName" ]] || [[ "$PlayerSteamIdCurrent" != "$PlayerSteamId" ]] || [[ "$PlayerSteamNameCurrent" != "$PlayerSteamName" ]]; then
				INSERT_CUSTOM_LOG "Player alterou seus dados desde a última conexão." "INFO" "$ScriptName"
				INSERT_PLAYER_NAME_HISTORY "$PlayerId" "$PlayerName" "$PlayerSteamId" "$PlayerSteamName"
			fi
            
            ;;
        player_connected)     
            CurrentDate=$(date "+%d/%m/%Y %H:%M:%S")      
            PlayerId=$(echo "$line" | jq -r '.player_id')
            echo "Evento de player conectado detectado!"
            INSERT_CUSTOM_LOG "Evento de player conectado detectado!" "INFO" "$ScriptName"
            PlayerExists=$(sqlite3 -separator "|" "$AppFolder/$AppPlayerBecoC1DbFile" "SELECT PlayerName, SteamID, SteamName FROM players_database WHERE PlayerID = '$PlayerId';")
            if [[ -z "$PlayerExists" ]]; then
                echo "Ignorando pois player não consta no banco"
                INSERT_CUSTOM_LOG "Ignorando pois player não consta no banco" "INFO" "$ScriptName"
                continue
            fi
            PlayerName=$(echo "$PlayerExists" | cut -d'|' -f1 | tr -d '|' | sed 's/[^a-zA-Z0-9_ -]//g' | xargs)
            SteamID=$(echo "$PlayerExists" | cut -d'|' -f2)
            SteamName=$(echo "$PlayerExists" | cut -d'|' -f3 | tr -d '|' | sed 's/[^a-zA-Z0-9_ -]//g' | xargs)

            if [[ -f "$DayzServerFolder/$DayzAdminIdsFile" ]] && grep -q "$PlayerId" "$DayzServerFolder/$DayzAdminIdsFile"; then
                echo "Ignorando conta do administrador e matando player para renascer com loot admin..."
                INSERT_CUSTOM_LOG "Ignorando conta do administrador e matando player para renascer com loot admin..." "INFO" "$ScriptName"
                continue
            fi

            Content="Jogador **$PlayerName** ([$SteamName](<https://steamcommunity.com/profiles/$SteamID>)) conectou"			
            SEND_DISCORD_WEBHOOK "$Content" "$DiscordWebhookLogs" "$CurrentDate" "$ScriptName"
            "$AppFolder/$AppScriptUpdatePlayersOnlineFile" "$PlayerId" "CONNECT"

            ;;
        player_disconnected)     
            CurrentDate=$(date "+%d/%m/%Y %H:%M:%S")      
            PlayerId=$(echo "$line" | jq -r '.player_id')
            echo "Evento de player desconectado detectado!" 
            INSERT_CUSTOM_LOG "Evento de player desconectado detectado!" "INFO" "$ScriptName"
            PlayerExists=$(sqlite3 -separator "|" "$AppFolder/$AppPlayerBecoC1DbFile" "SELECT PlayerName, SteamID, SteamName FROM players_database WHERE PlayerID = '$PlayerId';")
            if [[ -z "$PlayerExists" ]]; then
                echo "Ignorando pois player não consta no banco" 
                INSERT_CUSTOM_LOG "Ignorando pois player não consta no banco" "INFO" "$ScriptName"
                continue
            fi
            PlayerName=$(echo "$PlayerExists" | cut -d'|' -f1 | tr -d '|' | sed 's/[^a-zA-Z0-9_ -]//g' | xargs)
            SteamID=$(echo "$PlayerExists" | cut -d'|' -f2)
            SteamName=$(echo "$PlayerExists" | cut -d'|' -f3 | tr -d '|' | sed 's/[^a-zA-Z0-9_ -]//g' | xargs)

            if [[ -f "$DayzServerFolder/$DayzAdminIdsFile" ]] && grep -q "$PlayerId" "$DayzServerFolder/$DayzAdminIdsFile"; then
                echo "Ignorando conta do administrador e matando player para renascer com loot admin..."
                INSERT_CUSTOM_LOG "Ignorando conta do administrador e matando player para renascer com loot admin..." "INFO" "$ScriptName"
                continue
            fi

            Content="Jogador **$PlayerName** ([$SteamName](<https://steamcommunity.com/profiles/$SteamID>)) desconectou"			
            SEND_DISCORD_WEBHOOK "$Content" "$DiscordWebhookLogs" "$CurrentDate" "$ScriptName"
            "$AppFolder/$AppScriptUpdatePlayersOnlineFile" "$PlayerId" "DISCONNECT"
            ;;

        *)
            echo ">> Ação desconhecida: $action"
            ;;
    esac
done
