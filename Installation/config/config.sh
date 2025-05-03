#!/bin/bash

export TZ=America/Sao_Paulo

[ -z "${CONFIG_FILE:-}" ] && { echo "Erro: CONFIG_FILE não foi definido."; return 1; }

# Leitura dos dados usando jq
LinuxUserName=$(jq -r '.Linux.User.Name // empty' "$CONFIG_FILE")
if [[ -z "$LinuxUserName" ]]; then
    echo "Erro: Nome de usuário não encontrado no arquivo JSON."
    return 1
fi
export LinuxUserName

LinuxUserPassword=$(jq -r '.Linux.User.Password // empty' "$CONFIG_FILE")
if [[ -z "$LinuxUserPassword" ]]; then
    echo "Erro: Senha de usuário não encontrada no arquivo JSON."
    return 1
fi
export LinuxUserPassword

SteamAccount=$(jq -r '.Steam.Account // empty' "$CONFIG_FILE")
if [[ -z "$SteamAccount" ]]; then
    echo "Erro: Conta da steam não encontrada no arquivo JSON."
    return 1
fi
export SteamAccount

DayzServerName=$(jq -r '.DayZ.ServerName // empty' "$CONFIG_FILE")
if [[ -z "$DayzServerName" ]]; then
    echo "Erro: Nome do servidor não encontrado no arquivo JSON."
    return 1
fi
export DayzServerName

DayzPasswordAdmin=$(jq -r '.DayZ.PasswordAdmin // empty' "$CONFIG_FILE")
if [[ -z "$DayzPasswordAdmin" ]]; then
    echo "Erro: Senha de admin do servidor não encontrada no arquivo JSON."
    return 1
fi
export DayzPasswordAdmin

DayzMaxPlayers=$(jq -r '.DayZ.MaxPlayers // empty' "$CONFIG_FILE")
if [[ -z "$DayzMaxPlayers" ]]; then
    echo "Erro: Número máximo de jogadores não encontrado no arquivo JSON."
    return 1
fi
export DayzMaxPlayers

DayzMotdMessage=$(jq -r '.DayZ.MotdMessage // empty' "$CONFIG_FILE")
if [[ -z "$DayzMotdMessage" ]]; then
    echo "Erro: Mensagem do servidor não encontrada no arquivo JSON."
    return 1
fi
export DayzMotdMessage

DayzMotdIntervalSeconds=$(jq -r '.DayZ.MotdIntervalSeconds // empty' "$CONFIG_FILE")
if [[ -z "$DayzMotdIntervalSeconds" ]]; then
    echo "Erro: Intervalo da mensagem do servidor não encontrado no arquivo JSON."
    return 1
fi
export DayzMotdIntervalSeconds

DayzPcCpuMaxCores=$(jq -r '.DayZ.PcCpuMaxCores // empty' "$CONFIG_FILE")
if [[ -z "$DayzPcCpuMaxCores" ]]; then
    echo "Erro: Máximo de núcleos da CPU não encontrado no arquivo JSON."
    return 1
fi
export DayzPcCpuMaxCores

DayzPcCpuReservedcores=$(jq -r '.DayZ.PcCpuReservedcores // empty' "$CONFIG_FILE")
if [[ -z "$DayzPcCpuReservedcores" ]]; then
    echo "Erro: Núcleos reservados da CPU não encontrado no arquivo JSON."
    return 1
fi
export DayzPcCpuReservedcores

DayzRConPassword=$(jq -r '.DayZ.RConPassword // empty' "$CONFIG_FILE")
if [[ -z "$DayzRConPassword" ]]; then
    echo "Erro: Senha RCon não encontrada no arquivo JSON."
    return 1
fi
export DayzRConPassword

DayzMaxPing=$(jq -r '.DayZ.MaxPing // empty' "$CONFIG_FILE")
if [[ -z "$DayzMaxPing" ]]; then
    echo "Erro: Máximo de ping não encontrado no arquivo JSON."
    return 1
fi
export DayzMaxPing

DayzRestrictRCon=$(jq -r '.DayZ.RestrictRCon // empty' "$CONFIG_FILE")
if [[ -z "$DayzRestrictRCon" ]]; then
    echo "Erro: Restrição RCon não encontrada no arquivo JSON."
    return 1
fi
export DayzRestrictRCon

DayzRConPort=$(jq -r '.DayZ.RConPort // empty' "$CONFIG_FILE")
if [[ -z "$DayzRConPort" ]]; then
    echo "Erro: Porta RCon não encontrada no arquivo JSON."
    return 1
fi
export DayzRConPort

DayzRConIP=$(jq -r '.DayZ.RConIP // empty' "$CONFIG_FILE")
if [[ -z "$DayzRConIP" ]]; then
    echo "Erro: IP RCon não encontrado no arquivo JSON."
    return 1
fi
export DayzRConIP

DayzMpmission=$(jq -r '.DayZ.MpMission // empty' "$CONFIG_FILE")
if [[ -z "$DayzMpmission" ]]; then
    echo "Erro: Mpmission não encontrado no arquivo JSON."
    return 1
fi
export DayzMpmission

DayzLimitFPS=$(jq -r '.DayZ.LimitFPS // empty' "$CONFIG_FILE")
if [[ -z "$DayzLimitFPS" ]]; then
    echo "Erro: Limite de FPS não encontrado no arquivo JSON."
    return 1
fi
export DayzLimitFPS