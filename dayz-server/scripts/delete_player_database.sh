#!/bin/bash

# Caminho do ambiente virtual
VENV_DIR="./venv"

# Ativa config
source ./config.sh 2>/dev/null || { echo '{"error": "Falha ao carregar config.sh"}'; exit 1; }

DAYZ_ITEMS_DB="$AppFolder/$AppDayzItemsDbFile"
PLAYERS_BECO_C1_DB="$AppFolder/$AppPlayerBecoC1DbFile"

# Verificação do parâmetro
if [[ -z "$1" ]]; then
    echo "Uso: $0 PLAYER_ID"
    exit 1
fi

PLAYER_ID="$1"

# Verificação da variável do caminho do banco
if [[ -z "$PLAYERS_BECO_C1_DB" ]]; then
    echo "Erro: variável PLAYERS_BECO_C1_DB não definida."
    exit 1
fi

echo "Deletando jogador com PLAYER_ID: $PLAYER_ID"

# Verifica se o jogador existe
PLAYER_EXISTS=$(sqlite3 "$PLAYERS_BECO_C1_DB" "SELECT COUNT(*) FROM players_database WHERE PlayerID = '$PLAYER_ID';")

if [[ "$PLAYER_EXISTS" -eq 0 ]]; then
    echo "Jogador com PLAYER_ID '$PLAYER_ID' não encontrado no banco."
    exit 0
fi

# Executa a exclusão
sqlite3 "$PLAYERS_BECO_C1_DB" <<EOF
PRAGMA foreign_keys = ON;

DELETE FROM players_database WHERE PlayerID = '$PLAYER_ID';
EOF

if [[ $? -eq 0 ]]; then
    echo "Jogador removido com sucesso de todas as tabelas relacionadas."
else
    echo "Erro ao remover jogador."
    exit 1
fi
