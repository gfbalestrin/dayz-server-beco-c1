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
		DamageParsed=$("$AppFolder/$AppScriptGetPlayerDamageFile" "$Content")
		echo $DamageParsed
		INSERT_CUSTOM_LOG "$DamageParsed" "INFO" "$ScriptName"
		if [ $? -eq 0 ]; then
			INSERT_CUSTOM_LOG "Inserindo informações de dano no banco de dados..." "INFO" "$ScriptName"
			PlayerIdAttacker=$(echo "$DamageParsed" | cut -d'|' -f1)
			PlayerIdVictim=$(echo "$DamageParsed" | cut -d'|' -f2)
			PosAttacker=$(echo "$DamageParsed" | cut -d'|' -f3)
			PosAttacker=$(echo "$PosAttacker" | sed 's/, */,/g')
			PosVictim=$(echo "$DamageParsed" | cut -d'|' -f4)
			PosVictim=$(echo "$PosVictim" | sed 's/, */,/g')
			LocalDamage=$(echo "$DamageParsed" | cut -d'|' -f5)
			HitType=$(echo "$DamageParsed" | cut -d'|' -f6)
			Damage=$(echo "$DamageParsed" | cut -d'|' -f7)
			Health=$(echo "$DamageParsed" | cut -d'|' -f8)
			Data=$(date "+%Y-%m-%d %H:%M:%S")
			Weapon=$(echo "$DamageParsed" | cut -d'|' -f9)
			DistanceMeter=$(echo "$DamageParsed" | cut -d'|' -f10)
			INSERT_PLAYER_DAMAGE "$PlayerIdAttacker" "$PlayerIdVictim" "$PosAttacker" "$PosVictim" "$LocalDamage" "$HitType" "$Damage" "$Health" "$Data" "$Weapon" "$DistanceMeter"

			if [[ "$DayzDeathmatch" -eq "1" ]]; then
				continue
			fi

			SafePlayerAttackerInfo=""
			PlayerAttacker=$(sqlite3 -separator "|" "$AppFolder/$AppPlayerBecoC1DbFile" "SELECT PlayerName, SteamID, SteamName FROM players_database WHERE PlayerID = '$PlayerIdAttacker';")
			if [[ -n "$PlayerAttacker" ]]; then
				PlayerAttackerName=$(echo "$PlayerAttacker" | cut -d'|' -f1)
				AttackerSteamID=$(echo "$PlayerAttacker" | cut -d'|' -f2)
				AttackerSteamName=$(echo "$PlayerAttacker" | cut -d'|' -f3)
				PlayerAttackerInfo="**$PlayerAttackerName** ([$AttackerSteamName](<https://steamcommunity.com/profiles/$AttackerSteamID>))"
				SafePlayerAttackerInfo=$(printf '%s\n' "$PlayerAttackerInfo" | sed 's/[&/]/\\&/g')								
			else
				INSERT_CUSTOM_LOG "PlayerIdAttacker não encontrado no banco de dados. Ignorando log para o discord..." "ERROR" "$ScriptName"
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
				INSERT_CUSTOM_LOG "PlayerIdVictim não encontrado no banco de dados. Ignorando log para o discord..." "ERROR" "$ScriptName"
				continue
			fi	
			metros=$(echo $DistanceMeter | cut -d '.' -f 1)
			Content="Jogador $SafePlayerVictimInfo foi atingido por $SafePlayerAttackerInfo. Local do dano: $LocalDamage, dano sofrido: $Damage, arma: $Weapon, tipo de ataque: $HitType, distância: $metros metros, HP restante: $Health"
		else
			INSERT_CUSTOM_LOG "Falha ao realizar o parse das informações de dano do player" "ERROR" "$ScriptName"
		fi
	# Chat do jogo com comandos do admin
	elif [[ "$Content" == *"Chat("* ]]; then
		PlayerId=$(echo $Content | awk -F'id=' '{print $2}' | awk -F')' '{print $1}')
		if [[ "$PlayerId" == "" ]]; then			
			INSERT_CUSTOM_LOG "Ignorando pois PlayerId está em branco" "INFO" "$ScriptName"
			continue
		fi
		if [[ "$DayzDeathmatch" -eq "1" ]] && ! grep -q "$PlayerId" "$DayzServerFolder/$DayzAdminIdsFile"; then
			if [[ "$Content" == *"teleport"* ]]; then
				continue
			elif [[ "$Content" == *"godmode"* ]]; then
				continue
			elif [[ "$Content" == *"heal"* ]]; then
				continue			
			fi
			Command=$(echo "$Content" | sed -n 's|.*\/admin ||p')
			echo "$PlayerId $Command" >>"$DayzServerFolder/$DayzAdminCmdsFile"
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

		# Mensagem ingame
		if [[ "$Content" == *"is connected"* ]]; then
			echo "Jogador $PlayerName conectou" >> "$DayzServerFolder/$DayzMessagesToSendoFile"
		elif [[ "$Content" == *"has been disconnected"* ]]; then
			echo "Jogador $PlayerName desconectou" >> "$DayzServerFolder/$DayzMessagesToSendoFile"
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
			Content="Jogador **$PlayerName** ([$SteamName](<https://steamcommunity.com/profiles/$SteamID>)) conectou"
			"$AppFolder/$AppScriptUpdatePlayersOnlineFile" "$PlayerId" "CONNECT" &
		elif [[ "$Content" == *"has been disconnected"* ]]; then
			Content="Jogador **$PlayerName** ([$SteamName](<https://steamcommunity.com/profiles/$SteamID>)) desconectou"
			"$AppFolder/$AppScriptUpdatePlayersOnlineFile" "$PlayerId" "DISCONNECT" &
		fi
	# Evento de morte por player
	elif [[ "$Content" == *"killed by Player"* ]]; then
		INSERT_CUSTOM_LOG "Evento de PVP detectado!" "INFO" "$ScriptName"
		PlayerIdKilled=$(echo "$Content" | grep -oP 'id=\K[^ ]+' | head -n 1)
		PlayerIdKiller=$(echo "$Content" | sed -n 's|.*id=\([^ ]*\) pos.*|\1|p')
		Weapon=$(echo "$Content" | grep -oP 'with \K\w+')
		Distance=$(echo "$Content" | grep -oP 'from \K\d+\.\d+')
		Distance=$(echo $Distance | cut -d '.' -f 1 | cut -d ':' -f 1)
		PostKilled=$(echo "$Content" | sed -n 's/.*pos=<\([^>]*\)>.*pos=<[^>]*>.*/\1/p')
		PostKilled=$(echo "$PostKilled" | sed 's/, */,/g')
		PosKiller=$(echo "$Content" | sed -n 's/.*pos=<[^>]*>.*pos=<\([^>]*\)>.*/\1/p')
		PosKiller=$(echo "$PosKiller" | sed 's/, */,/g')
		Data=$(date "+%Y-%m-%d %H:%M:%S")
		INSERT_KILLFEED "$PlayerIdKiller" "$PlayerIdKilled" "$Weapon" "$Distance" "$Data" "$PosKiller" "$PostKilled"

		SafePlayerKillerInfo=""
		PlayerKiller=$(sqlite3 -separator "|" "$AppFolder/$AppPlayerBecoC1DbFile" "SELECT PlayerName, SteamID, SteamName FROM players_database WHERE PlayerID = '$PlayerIdKiller';")
		if [[ -n "$PlayerKiller" ]]; then
			PlayerKillerName=$(echo "$PlayerKiller" | cut -d'|' -f1)
			KillerSteamID=$(echo "$PlayerKiller" | cut -d'|' -f2)
			KillerSteamName=$(echo "$PlayerKiller" | cut -d'|' -f3)
			PlayerKillerInfo="**$PlayerKillerName** ([$KillerSteamName](<https://steamcommunity.com/profiles/$KillerSteamID>))"
			SafePlayerKillerInfo=$(printf '%s\n' "$PlayerKillerInfo" | sed 's/[&/]/\\&/g')								
		else
			INSERT_CUSTOM_LOG "PlayerIdKiller não encontrado no banco de dados" "ERROR" "$ScriptName"
		fi	

		SafePlayerVictimInfo=""
		PlayerVictim=$(sqlite3 -separator "|" "$AppFolder/$AppPlayerBecoC1DbFile" "SELECT PlayerName, SteamID, SteamName FROM players_database WHERE PlayerID = '$PlayerIdKilled';")
		if [[ -n "$PlayerVictim" ]]; then
			PlayerVictimName=$(echo "$PlayerVictim" | cut -d'|' -f1)
			VictimSteamID=$(echo "$PlayerVictim" | cut -d'|' -f2)
			VictimSteamName=$(echo "$PlayerVictim" | cut -d'|' -f3)
			PlayerVictimInfo="**$PlayerVictimName** ([$VictimSteamName](<https://steamcommunity.com/profiles/$VictimSteamID>))"
			SafePlayerVictimInfo=$(printf '%s\n' "$PlayerVictimInfo" | sed 's/[&/]/\\&/g')								
		else
			INSERT_CUSTOM_LOG "PlayerIdVictim não encontrado no banco de dados. Ignorando log para o discord..." "ERROR" "$ScriptName"
		fi	

		if [[ -n "$SafePlayerKillerInfo" && -n "$SafePlayerVictimInfo" ]]; then
			metros=$(echo $Distance | cut -d '.' -f 1)
			Content="💀 Jogador $SafePlayerVictimInfo foi executado por $SafePlayerKillerInfo. Arma: $Weapon, distância: $metros metros"

			# Mensagem ingame
			echo "Jogador $PlayerKillerName eliminou $PlayerVictimName" >> "$DayzServerFolder/$DayzMessagesToSendoFile"
		else
			INSERT_CUSTOM_LOG "PlayerIdKilled ou PlayerIdVictim não encontrado no banco de dados. Usando o conteúdo original para o discord..." "ERROR" "$ScriptName"
		fi
	# Eventos de restart do server
	elif [[ "$Line" == *"AdminLog started on"* ]]; then
		INSERT_CUSTOM_LOG "Evento de restart do server detectado! O serviço dayz-infos-logs-discord.service será reiniciado..." "INFO" "$ScriptName"		
		if [[ "$DayzWipeOnRestart" -eq "1" ]]; then
			Content="Wipe realizado e servidor reiniciado. Aguardando liberação de conexão..."
		else
			Content="Servidor reiniciado. Aguardando liberação de conexão..."
		fi

		# if [[ "$DayzDeathmatch" -eq "1" ]]; then
		# 	DELETE_KILLFEED 
		# 	DELETE_PLAYER_DAMAGE
		# fi
		sleep 1
		# Configurar visudo
		# <usuario> ALL=NOPASSWD: /bin/systemctl restart dayz-infos-logs-discord.service
		sudo systemctl restart dayz-infos-logs-discord.service
	else
		
		Content="${Content//is unconscious/está inconsciente}"
		Content="${Content//bled out/morreu por sangramento}"
		Content="${Content//killed by/morto por}"
		Content="${Content/(DEAD)/}"
		Content=$(echo $Content | sed 's/died\..*/morreu para o ambiente/')

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

		Content="${Content//Player/Jogador}"
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