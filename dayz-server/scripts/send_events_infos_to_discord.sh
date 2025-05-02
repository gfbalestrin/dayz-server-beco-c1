#!/bin/bash

# Carrega as variáveis
source ./config.sh

ScriptName=$(basename "$0")
LogFileName="$DayzServerFolder/$DayzLogRPTFile"
CurrentLog=""

# Atualiza o symlink para o mais novo arquivo
update_symlink() {
	LatestLog=$(ls -1t "$DayzServerFolder"/profiles/DayZServer_*.RPT 2>/dev/null | head -n1)
	INSERT_CUSTOM_LOG "Último log: $LatestLog" "INFO" "$ScriptName"
	if [[ -n "$LatestLog" && ("$LatestLog" != "$CurrentLog" || ! -L "$LogFileName") ]]; then
		INSERT_CUSTOM_LOG "O arquivo de log RPT mudou ou symlink não existe. Criando novo symlink $LogFileName a partir de $LatestLog" "INFO" "$ScriptName"
		ln -sf "$LatestLog" "$LogFileName"
		CurrentLog="$LatestLog"
	fi
}

function MonitorLog() {
	tail -n +1 -F "$LogFileName" | grep -n --line-buffered -e "is connected" -e "Shutting down in 60 seconds" -e "Invalid number -nan" -e "kicked from server" -e "Termination successfully completed" -e "Mission script has no main function" | while IFS='' read -r Line; do
		CurrentDate=$(date "+%d/%m/%Y %H:%M:%S")
		Content=$(echo $Line | cut -c 15-)
		INSERT_RPT_LOG "$Line" "INFO"
		INSERT_CUSTOM_LOG "Evento capturado: '$Content'" "INFO" "$ScriptName"

		if [[ "$Content" == *"Mission script has no main function"* ]]; then
			SEND_DISCORD_WEBHOOK "Administrador fez alguma merda no script init.c e o servidor está inoperante" "$DiscordWebhookLogs" "$CurrentDate" "$ScriptName"
			continue
		elif [[ "$Content" == *"kicked from server"* ]]; then
			if [[ "$Content" == *"Connection with host has been lost"* || "$Content" == *"Server is shutting down"* ]]; then
				INSERT_CUSTOM_LOG "Ignorando evento" "INFO" "$ScriptName"
				continue
			fi
			SEND_DISCORD_WEBHOOK "$Content" "$DiscordWebhookLogs" "$CurrentDate" "$ScriptName"
			continue
		elif [[ "$Content" == *"Invalid number"* ]]; then
			PlayerId=""
			LineNumber=$(echo $Line | cut -d ":" -f 1)
			while ((LineNumber > 0)); do
				((LineNumber--))
				LinePlayerId=$(sed -n "${LineNumber}p" "$LogFileName")

				if [[ "$LinePlayerId" =~ ^[[:space:]]+uid[[:space:]]+([A-Za-z0-9_=-]{44})$ ]]; then
					PlayerId="${BASH_REMATCH[1]}"
					break
				fi
			done

			if [[ -n "$PlayerId" ]]; then
				SEND_DISCORD_WEBHOOK "Player bugado detectado: $PlayerId" "$DiscordWebhookLogsAdmin" "$CurrentDate" "$ScriptName"
			else
				SEND_DISCORD_WEBHOOK "Player bugado detectado mas PlayerId não identificado" "$DiscordWebhookLogsAdmin" "$CurrentDate" "$ScriptName"
			fi
			
			continue
		elif [[ "$Content" == *"Shutting down in 60 seconds"* ]]; then
			"$AppFolder/$AppScriptExtractPlayersStatsFile" &
			"$AppFolder/$AppScriptUpdateGeneralKillfeed" &
			"$AppFolder/$AppScriptUpdatePlayersOnlineFile" "RESET" &
			SEND_DISCORD_WEBHOOK "Server restarting..." "$DiscordWebhookLogs" "$CurrentDate" "$ScriptName"
			continue
		elif [[ "$Content" == *"is connected"* ]]; then
			LineNumberSteam=$(echo $Line | cut -d ":" -f 1)
			INSERT_CUSTOM_LOG "LineNumberSteam: $LineNumberSteam" "INFO" "$ScriptName"
			LineNumberId=$((LineNumberSteam - 1))
			INSERT_CUSTOM_LOG "LineNumberId: $LineNumberId" "INFO" "$ScriptName"
			PlayerSteamId=""
			if [[ "$Content" == *"steamID="* ]]; then
				PlayerSteamId=$(echo $Content | cut -d "=" -f 2 | cut -d ")" -f 1)
				INSERT_CUSTOM_LOG "PlayerSteamId: $PlayerSteamId" "INFO" "$ScriptName"
			fi
			# Captura linha anterior
			LinePrev=$(awk "NR==$LineNumberId" $LogFileName)
			INSERT_CUSTOM_LOG "LinePrev: $LinePrev" "INFO" "$ScriptName"
			# Remove primeiros 9 caracteres que contem a hora
			ContentPrev=$(echo $LinePrev | cut -c 9-)

			PlayerId=$(echo $ContentPrev | grep -oP 'id=\K[A-Za-z0-9_=-]+')
			PlayerName=$(echo $ContentPrev | grep -oP '(?<=Player ).*?(?= \(id=)')
			PlayerName=$(echo $PlayerName | sed "s/;//g")
			PlayerName=$(echo $PlayerName | sed "s/#//g")
			INSERT_CUSTOM_LOG "PlayerId: $PlayerId" "INFO" "$ScriptName"
			INSERT_CUSTOM_LOG "PlayerName: $PlayerName" "INFO" "$ScriptName"
			if [[ "$PlayerId" == "" ]]; then
				INSERT_CUSTOM_LOG "Ignorando pois PlayerId está em branco" "INFO" "$ScriptName"
				continue
			fi
			if grep -q "$PlayerId" "$DayzServerFolder/$DayzAdminIdsFile"; then
				INSERT_CUSTOM_LOG "Administrador detectado! O evento será ignorado" "INFO" "$ScriptName"
				continue
			fi

			for ((i = 0; i < $AppSpoofCount; i++)); do
				eval from=\$AppSpoofFrom_$i
				eval to=\$AppSpoofTo_$i
				if [[ "$from" == "$PlayerSteamId" ]]; then
					PlayerSteamId=$to
					INSERT_CUSTOM_LOG "Fazendo spoof do steam id: from $from to $to" "INFO" "$ScriptName"
				fi
			done

			PlayerSteamName=""
			if [ "$PlayerSteamId" != "" ]; then
				# Para extrair nome da steam
				PlayerSteamName=$(curl -L -s https://steamcommunity.com/profiles/$PlayerSteamId | grep actual_persona_name | grep -v "&nbsp;" | sed 's:</span>:\n:g' | sed -n 's/.*>//p' | sed 's/[^a-zA-Z0-9_ -]//g' | xargs)

				PlayerSteamName=$(echo $PlayerSteamName | sed "s/[^a-zA-Z0-9_-]//g")
				INSERT_CUSTOM_LOG "PlayerSteamName: $PlayerSteamName" "INFO" "$ScriptName"
			fi

			if [ "$PlayerSteamName" == "" ]; then
				PlayerSteamName="Unknown"
			fi

			if [ "$PlayerSteamId" != "" ]; then
				Content="$Content - Steam=$PlayerSteamName - <https://steamcommunity.com/profiles/$PlayerSteamId>"
			else
				Content="$Content - Steam=Unknown"
			fi

			# Buscar player na database. Se não existir criar e enviar para discord. Se existir apenas atualizar.
			PlayerExists=$(sqlite3 -separator "|" "$AppFolder/$AppPlayerBecoC1DbFile" "SELECT PlayerName, SteamID, SteamName FROM players_database WHERE PlayerID = '$PlayerId';")
			if [[ -z "$PlayerExists" ]]; then
				INSERT_CUSTOM_LOG "Player não consta no banco. O player será inserido no banco de dados." "INFO" "$ScriptName"
				INSERT_PLAYER_DATABASE "$PlayerId" "$PlayerName" "$PlayerSteamId" "$PlayerSteamName"
				sleep 1
				Content="Player **$PlayerName** ([$PlayerSteamName](<https://steamcommunity.com/profiles/$PlayerSteamId>)) is connected"
				SEND_DISCORD_WEBHOOK "$Content" "$DiscordWebhookLogs" "$CurrentDate" "$ScriptName"
				"$AppFolder/$AppScriptUpdatePlayersOnlineFile" "$PlayerId" "CONNECT" &
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
		elif [[ "$Line" == *"Termination successfully completed"* ]]; then
			INSERT_CUSTOM_LOG "Fim do arquivo identificado." "INFO" "$ScriptName"
			break
		fi

		# Verifica se houve rotação do arquivo
		# LatestLog=$(ls -1t "$DayzServerFolder"/profiles/DayZServer_*.RPT 2>/dev/null | head -n1)
		# if [[ "$LatestLog" != "$CurrentLog" ]]; then
		# 	break
		# fi
	done
}

# Loop principal
while true; do
	update_symlink
	MonitorLog
	sleep 10
done
