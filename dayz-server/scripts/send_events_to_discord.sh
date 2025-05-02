#!/bin/bash

# Carrega as variáveis
source ./config.sh

ScriptName=$(basename "$0")
LogFileName="$DayzServerFolder/$DayzLogAdmFile"

INSERT_CUSTOM_LOG "Monitorando arquivo: $LogFileName" "INFO" "$ScriptName"

tail -n 0 -F $LogFileName | grep --line-buffered -e "is connected" -e "has been disconnected" -e "killed by" -e "is unconscious" -e "AdminLog started on" -e "bled out" -e "died. Stats" -e "Chat(" -e "hit by Player" | while IFS='' read Line; do
	echo "$Line" | grep -q "\[HP: 0\]" && continue

	INSERT_ADM_LOG "$Line" "INFO"
	# Remove primeiros 12 caracteres que contém informações de data e hora
	Content=$(echo $Line | cut -c 12-)
	CurrentDate=$(date "+%d/%m/%Y %H:%M:%S")
	
	INSERT_CUSTOM_LOG "Evento capturado: '$Content'" "INFO" "$ScriptName"
	# Dano em player
	if [[ "$Content" == *"hit by Player"* ]]; then
		DamageParsed=$("$AppFolder/$AppScriptParseDamageFile" "$Content")
		if [ $? -eq 0 ]; then
			INSERT_CUSTOM_LOG "Inserindo informações de dano no banco de dados..." "ERROR" "$ScriptName"
			PlayerIdAttacker=$(echo "$DamageParsed" | cut -d'|' -f1)
			PlayerIdVictim=$(echo "$DamageParsed" | cut -d'|' -f2)
			PosAttacker=$(echo "$DamageParsed" | cut -d'|' -f3)
			PosVictim=$(echo "$DamageParsed" | cut -d'|' -f4)
			LocalDamage=$(echo "$DamageParsed" | cut -d'|' -f5)
			HitType=$(echo "$DamageParsed" | cut -d'|' -f6)
			Damage=$(echo "$DamageParsed" | cut -d'|' -f7)
			Health=$(echo "$DamageParsed" | cut -d'|' -f8)
			Data=$(date "+%Y-%m-%d %H:%M:%S")
			Weapon=$(echo "$DamageParsed" | cut -d'|' -f9)
			DistanceMeter=$(echo "$DamageParsed" | cut -d'|' -f10)
			INSERT_DAMAGE "$PlayerIdAttacker" "$PlayerIdVictim" "$PosAttacker" "$PosVictim" "$LocalDamage" "$HitType" "$Damage" "$Health" "$Data" "$Weapon" "$DistanceMeter"

			SafePlayerAttackerInfo=""
			PlayerAttacker=$(sqlite3 -separator "|" "$AppFolder/$AppPlayerBecoC1DbFile" "SELECT PlayerName, SteamID, SteamName FROM players_database WHERE PlayerID = '$PlayerIdAttacker';")
			if [[ -n "$PlayerAttacker" ]]; then
				PlayerAttackerName=$(echo "$PlayerAttacker" | cut -d'|' -f1)
				AttackerSteamID=$(echo "$PlayerAttacker" | cut -d'|' -f2)
				AttackerSteamName=$(echo "$PlayerAttacker" | cut -d'|' -f3)
				PlayerAttackerInfo="**$PlayerAttackerName** ([$AttackerSteamName](<https://steamcommunity.com/profiles/$AttackerSteamID>))"
				SafePlayerAttackerInfo=$(printf '%s\n' "$PlayerAttackerInfo" | sed 's/[&/]/\\&/g')								
			else
				INSERT_CUSTOM_LOG "PlayerIdAttacker não encontrado no banco de dados. Ignorando..." "INFO" "$ScriptName"
				continue
			fi	

			SafePlayerVictimInfo=""
			PlayerVictim=$(sqlite3 -separator "|" "$AppFolder/$AppPlayerBecoC1DbFile" "SELECT PlayerName, SteamID, SteamName FROM players_database WHERE PlayerID = '$PlayerIdVictim';")
			if [[ -n "$PlayerVictim" ]]; then
				PlayerVictimName=$(echo "$PlayerVictim" | cut -d'|' -f1)
				VictimSteamID=$(echo "$PlayerVictim" | cut -d'|' -f2)
				VictimSteamName=$(echo "$PlayerVictim" | cut -d'|' -f3)
				PlayerVictimInfo="**$PlayerVictimName** ([$VictimSteamName](<https://steamcommunity.com/profiles/$VictimSteamID>))"
				SafePlayerVictimInfo=$(printf '%s\n' "$PlayerVictimInfo" | sed 's/[&/]/\\&/g')								
			else
				INSERT_CUSTOM_LOG "PlayerIdVictim não encontrado no banco de dados. Ignorando..." "INFO" "$ScriptName"
				continue
			fi	
			
			Content="Player $SafePlayerVictimInfo levou um pau do $SafePlayerAttackerInfo. Local do dano: $LocalDamage, dano sofrido: $Damage, arma: $Weapon, tipo de ataque: $HitType, distância: $DistanceMeter metros, HP restante: $Health"
		else
			INSERT_CUSTOM_LOG "Falha ao realizar o parse das informações da dano do player" "ERROR" "$ScriptName"
		fi
	# Chat do jogo com comandos do admin
	elif [[ "$Content" == *"Chat("* ]]; then
		PlayerId=$(echo $Content | awk -F'id=' '{print $2}' | awk -F')' '{print $1}')
		if [[ "$PlayerId" == "" ]]; then			
			INSERT_CUSTOM_LOG "Ignorando pois PlayerId está em branco" "INFO" "$ScriptName"
			continue
		fi
		if ! grep -q "$PlayerId" "$DayzServerFolder/$DayzAdminIdsFile"; then			
			continue
		fi
		INSERT_CUSTOM_LOG "Admin '$PlayerId' digitou no chat!" "INFO" "$ScriptName"
		if [[ "$Content" == *"/admin"* ]]; then
			Command=$(echo "$Content" | sed -n 's|.*\/admin ||p')
			INSERT_CUSTOM_LOG "Admin executou comando: $Command" "INFO" "$ScriptName"			
			echo "$PlayerId $Command" >>"$DayzServerFolder/$DayzAdminCmdsFile"
			if [[ "$Content" == *"giveitem"* ]]; then
				ItemName=$(echo "$Content" | cut -d' ' -f4)
				SEND_DISCORD_WEBHOOK "Administrador executou comando para criar um item: $ItemName" "$DiscordWebhookLogs" "$CurrentDate" "$ScriptName"
			elif [[ "$Content" == *"spawnvehicle"* ]]; then
				ItemName=$(echo "$Content" | cut -d' ' -f4)
				SEND_DISCORD_WEBHOOK "Administrador executou comando para criar um carro: $ItemName" "$DiscordWebhookLogs" "$CurrentDate" "$ScriptName"
			fi
		elif [[ "$Content" == *"/survivor"* ]]; then
			Command=$(echo "$Content" | sed -n 's|.*\/survivor ||p')
			INSERT_CUSTOM_LOG "Admin executou comando para Survivor: $Command" "INFO" "$ScriptName"
			echo "$Command" >>"$DayzServerFolder/$DayzAdminCmdsFile"
			if [[ "$Content" == *"giveitem"* ]]; then
				ItemName=$(echo "$Content" | cut -d' ' -f4)
				ItemName=$(echo "$ItemName" | sed 's/[^a-zA-Z0-9_]/_/g')
				SEND_DISCORD_WEBHOOK "Administrador executou comando para criar um item para um jogador: $ItemName" "$DiscordWebhookLogs" "$CurrentDate" "$ScriptName"
			elif [[ "$Content" == *"spawnvehicle"* ]]; then
				ItemName=$(echo "$Content" | cut -d' ' -f4)
				ItemName=$(echo "$ItemName" | sed 's/[^a-zA-Z0-9_]/_/g')
				SEND_DISCORD_WEBHOOK "Administrador executou comando para criar um carro para um jogador: $ItemName" "$DiscordWebhookLogs" "$CurrentDate" "$ScriptName"
			elif [[ "$Content" == *"teleport"* ]]; then
				SEND_DISCORD_WEBHOOK "Administrador teleportou um jogador" "$DiscordWebhookLogs" "$CurrentDate" "$ScriptName"
			elif [[ "$Content" == *"heal"* ]]; then
				SEND_DISCORD_WEBHOOK "Administrador curou um jogador" "$DiscordWebhookLogs" "$CurrentDate" "$ScriptName"
			elif [[ "$Content" == *"godmode"* ]]; then
				SEND_DISCORD_WEBHOOK "Administrador ativou godmode para um jogador" "$DiscordWebhookLogs" "$CurrentDate" "$ScriptName"
			elif [[ "$Content" == *"ungodmode"* ]]; then
				SEND_DISCORD_WEBHOOK "Administrador desativou godmode para um jogador" "$DiscordWebhookLogs" "$CurrentDate" "$ScriptName"
			fi
		fi
		continue
	# Evento de conexao e desconexao de players
	elif [[ "$Content" == *"is connected"* || "$Content" == *"has been disconnected"* ]]; then
		INSERT_CUSTOM_LOG "Evento de player conectado ou desconectado detectado!" "INFO" "$ScriptName"
		PlayerId=$(echo $Content | awk -F'id=' '{print $2}' | awk -F')' '{print $1}')
		PlayerName=$(echo $Content | awk -F'"' '{print $2}')

		if [[ "$PlayerId" == "Unknown" ]]; then
			INSERT_CUSTOM_LOG "PlayerId Unknown. Ignorando..." "INFO" "$ScriptName"
			continue
		fi

		PlayerExists=$(sqlite3 -separator "|" "$AppFolder/$AppPlayerBecoC1DbFile" "SELECT PlayerName, SteamID, SteamName FROM players_database WHERE PlayerID = '$PlayerId';")
		if [[ -z "$PlayerExists" ]]; then
			INSERT_CUSTOM_LOG "Ignorando pois player não consta no banco" "INFO" "$ScriptName"
			continue
		fi

		PlayerName=$(echo "$PlayerExists" | cut -d'|' -f1)
		SteamID=$(echo "$PlayerExists" | cut -d'|' -f2)
		SteamName=$(echo "$PlayerExists" | cut -d'|' -f3)		

		if [[ -f "$DayzServerFolder/$DayzAdminIdsFile" ]] && grep -q "$PlayerId" "$DayzServerFolder/$DayzAdminIdsFile"; then
			INSERT_CUSTOM_LOG "Ignorando conta do administrador e matando player para renascer com loot admin..." "INFO" "$ScriptName"
			# Mata o administrador para renascer com loot admin
			sqlite3 "$DayzServerFolder/$DayzPlayerDbFile" "UPDATE Players set Alive = 0 where UID = '$PlayerId';"
			continue
		fi

		if [[ "$Content" == *"is connected"* ]]; then
			Content="Player **$PlayerName** ([$SteamName](<https://steamcommunity.com/profiles/$SteamID>)) is connected"
			"$AppFolder/$AppScriptUpdatePlayersOnlineFile" "$PlayerId" "CONNECT" &
		elif [[ "$Content" == *"has been disconnected"* ]]; then
			Content="Player **$PlayerName** ([$SteamName](<https://steamcommunity.com/profiles/$SteamID>)) has been disconnected"
			"$AppFolder/$AppScriptUpdatePlayersOnlineFile" "$PlayerId" "DISCONNECT" &
		fi
	# Evento de morte por player
	elif [[ "$Content" == *"killed by Player"* ]]; then
		INSERT_CUSTOM_LOG "Evento de PVP detectado!" "INFO" "$ScriptName"
		PlayerIdKilled=$(echo "$Content" | grep -oP 'id=\K[^ ]+' | head -n 1)
		PlayerIdKiller=$(echo "$Content" | sed -n 's|.*id=\([^ ]*\) pos.*|\1|p')
		Weapon=$(echo "$Content" | grep -oP 'with \K\w+')
		Distance=$(echo "$Content" | grep -oP 'from \K\d+\.\d+')
		PostKilled=$(echo "$Content" | sed -n 's/.*pos=<\([^>]*\)>.*pos=<[^>]*>.*/\1/p')
		PosKiller=$(echo "$Content" | sed -n 's/.*pos=<[^>]*>.*pos=<\([^>]*\)>.*/\1/p')
		Data=$(date "+%Y-%m-%d %H:%M:%S")
		INSERT_KILLFEED "$PlayerIdKiller" "$PlayerIdKilled" "$Weapon" "$Distance" "$Data" "$PosKiller" "$PostKilled"
	# Eventos de restart do server
	elif [[ "$Line" == *"AdminLog started on"* ]]; then
		INSERT_CUSTOM_LOG "Evento de restart do server detectado! O serviço dayz-infos-logs-discord.service será reiniciado..." "INFO" "$ScriptName"
		Content="Server successfully restarted!"
		sleep 1
		# Configurar visudo
		# <usuario> ALL=NOPASSWD: /bin/systemctl restart dayz-infos-logs-discord.service
		sudo systemctl restart dayz-infos-logs-discord.service
	else	
		PlayerId=$(echo "$Content" | grep -oP 'id=\K[^ ]+' | head -n 1)
		
		if [[ ${#PlayerId} -eq 44 ]]; then
			PlayerExists=$(sqlite3 -separator "|" "$AppFolder/$AppPlayerBecoC1DbFile" "SELECT PlayerName, SteamID, SteamName FROM players_database WHERE PlayerID = '$PlayerId';")
			if [[ -n "$PlayerExists" ]]; then
				PlayerName=$(echo "$PlayerExists" | cut -d'|' -f1)
				SteamID=$(echo "$PlayerExists" | cut -d'|' -f2)
				SteamName=$(echo "$PlayerExists" | cut -d'|' -f3)
				PlayerInfo="**$PlayerName** ([$SteamName](<https://steamcommunity.com/profiles/$SteamID>))"
				INSERT_CUSTOM_LOG "Informações do jogador: $PlayerInfo" "INFO" "$ScriptName"
				SafePlayerInfo=$(printf '%s\n' "$PlayerInfo" | sed 's/[&/]/\\&/g')
                NewContent=$(echo "$Content" | sed -E "s|(Player )\"[^\"]+\"|\1\"$SafePlayerInfo\"|")		
				
				if [[ -n "$NewContent" ]]; then
					Content="$NewContent"
					INSERT_CUSTOM_LOG "Evento formatado com informações do jogador: $Content" "INFO" "$ScriptName"	
				else
					INSERT_CUSTOM_LOG "Erro ao formatar o evento com informações do jogador" "INFO" "$ScriptName"					
				fi				
			else
				INSERT_CUSTOM_LOG "PlayerId não encontrado no banco de dados. Ignorando..." "INFO" "$ScriptName"
			fi			
		else
			INSERT_CUSTOM_LOG "Não foi possível capturar o PlayerId do evento" "INFO" "$ScriptName"		
		fi
	fi

	# Remove aspas
	Content=$(echo $Content | sed -e 's|["'\'']||g')
	# Remove parenteses e seu conteudo
	Content=$(echo $Content | sed "s/[(][id=.+][^)]*[)]//g")
	# Remove multiplos espacos
	Content=$(echo $Content | sed "s/   */ /g")
	
	# Envia $Content para discord
	SEND_DISCORD_WEBHOOK "$Content" "$DiscordWebhookLogs" "$CurrentDate" "$ScriptName"

done