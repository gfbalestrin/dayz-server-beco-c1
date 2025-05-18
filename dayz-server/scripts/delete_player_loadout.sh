#!/bin/bash

# Caminho do ambiente virtual
VENV_DIR="./venv"

# Ativa config
source ./config.sh 2>/dev/null || { echo '{"error": "Falha ao carregar config.sh"}'; exit 1; }

DAYZ_ITEMS_DB="$AppFolder/$AppDayzItemsDbFile"
PLAYERS_BECO_C1_DB="$AppFolder/$AppPlayerBecoC1DbFile"

PLAYER_ID="$1"

# Verificação de argumentos
if [[ -z "$PLAYER_ID" ]]; then
    echo "Uso: $0 PLAYER_ID"
    exit 1
fi

if [[ -z "$DAYZ_ITEMS_DB" ]]; then
    echo "Erro: variável DAYZ_ITEMS_DB não está definida."
    exit 1
fi

# Verifica se o jogador existe
PLAYER_EXISTS=$(sqlite3 "$DAYZ_ITEMS_DB" "SELECT COUNT(*) FROM player_logins WHERE player_id = '$PLAYER_ID';")

if [[ "$PLAYER_EXISTS" -eq 0 ]]; then
    echo "Jogador com player_id '$PLAYER_ID' não encontrado."
    exit 0
fi

# Busca os IDs de loadout do jogador
LOADOUT_IDS=$(sqlite3 "$DAYZ_ITEMS_DB" "SELECT id FROM player_loadouts WHERE player_id = '$PLAYER_ID';")

# Ativa FK e exclui dependências
sqlite3 "$DAYZ_ITEMS_DB" <<EOF
PRAGMA foreign_keys = ON;

-- Remove todos os player_loadouts do jogador (gatilhos de ON DELETE CASCADE cuidam das tabelas relacionadas)
DELETE FROM player_loadouts WHERE player_id = '$PLAYER_ID';

-- Agora que os loadouts e dependências foram removidos, remova o login
DELETE FROM player_logins WHERE player_id = '$PLAYER_ID';
EOF

if [[ $? -eq 0 ]]; then
    echo "Jogador '$PLAYER_ID' removido com sucesso com todas as dependências."
else
    echo "Erro ao remover o jogador."
    exit 1
fi