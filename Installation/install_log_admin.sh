#!/bin/bash

DELAY=10

# Função de ajuda
usage() {
  echo "Uso: $0 [--skip-discord]"
  exit 1
}

SKIP_DISCORD=0

# Processa os argumentos
while [[ "$#" -gt 0 ]]; do
  case "$1" in
    --skip-discord)
      SKIP_DISCORD=1
      shift
      ;;
    *)
      echo "Parâmetro desconhecido: $1"
      usage
      ;;
  esac
done

[[ "$SKIP_DISCORD" -eq 1 ]] && echo "Flag --skip-discord foi ativada. O sistema integrado ao Discord não será instalado." 
[[ "$SKIP_DISCORD" -eq 0 ]] && echo "Flag --skip-discord não foi ativada. A integração com o discord está desativada." && exit 1

echo "Iniciando em $DELAY segundos..."
sleep $DELAY

set -eEuo pipefail  # u para erro em variáveis não definidas, o pipefail para detectar falhas em pipes
#set -x              # debug: mostra cada comando antes de executar

# Função de erro personalizada
erro_tratado() {
    local exit_code=$?
    local cmd="${BASH_COMMAND}"
    echo "❌ Erro ao executar: '$cmd'" >&2
    echo "Código de saída: $exit_code" >&2
    echo "O script falhou. Verifique os detalhes acima." >&2
}
trap erro_tratado ERR

# Verifica se está sendo executado como root
if [[ "$EUID" -ne 0 ]]; then
    echo "Erro: este script deve ser executado como root." >&2
    exit 1
fi

# Atualiza pacotes e instala jq
apt -y update
apt -y install sqlite3

# Determina o diretório do script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CONFIG_SCRIPT="$SCRIPT_DIR/config/config.sh"
CONFIG_FILE="$SCRIPT_DIR/config/config.json"

# Valida existência dos arquivos
[[ -f "$CONFIG_SCRIPT" ]] || { echo "Erro: config.sh não encontrado em $CONFIG_SCRIPT"; exit 1; }
[[ -f "$CONFIG_FILE" ]] || { echo "Erro: config.json não encontrado em $CONFIG_FILE"; exit 1; }

# Exporta caminho do JSON para ser usado no config.sh
export CONFIG_FILE

# Executa config.sh
source "$CONFIG_SCRIPT"

DayzFolder="/home/$LinuxUserName/servers/dayz-server"
echo "Diretório do servidor: $DayzFolder"
cd "$DayzFolder"
echo "Criando diretório scripts em $DayzFolder"
mkdir -p scripts 
cd scripts

curl -o config.json https://raw.githubusercontent.com/gfbalestrin/dayz-server-beco-c1/refs/heads/main/dayz-server/scripts/config.json
curl -o config.sh https://github.com/gfbalestrin/dayz-server-beco-c1/blob/main/dayz-server/scripts/config.sh
chmod +x config.sh

curl -o send_events_to_discord.sh https://raw.githubusercontent.com/gfbalestrin/dayz-server-beco-c1/refs/heads/main/dayz-server/scripts/send_events_to_discord.sh

curl -o send_events_to_discord.sh https://raw.githubusercontent.com/gfbalestrin/dayz-server-beco-c1/refs/heads/main/dayz-server/scripts/send_events_to_discord.sh
chmod +x send_events_to_discord.sh

curl -o send_events_infos_to_discord.sh https://raw.githubusercontent.com/gfbalestrin/dayz-server-beco-c1/refs/heads/main/dayz-server/scripts/send_events_infos_to_discord.sh
chmod +x send_events_infos_to_discord.sh

mkdir -p "$DayzFolder/scripts/databases"
cd "$DayzFolder/scripts/databases"

curl -o players_beco_c1.sql https://raw.githubusercontent.com/gfbalestrin/dayz-server-beco-c1/refs/heads/main/dayz-server/scripts/databases/players_beco_c1.sql
curl -o server_beco_c1_logs.sql https://raw.githubusercontent.com/gfbalestrin/dayz-server-beco-c1/refs/heads/main/dayz-server/scripts/databases/server_beco_c1_logs.sql

# Cria as bases de dados e executa os comandos SQL
sqlite3 "players_beco_c1.db" < "players_beco_c1.sql"
echo "✔ Banco players_beco_c1.db criado com sucesso."

sqlite3 "server_beco_c1_logs.db" < "server_beco_c1_logs.sql"
echo "✔ Banco server_beco_c1_logs.db criado com sucesso."


chown -R "$LinuxUserName:$LinuxUserName" "$DayzFolder"