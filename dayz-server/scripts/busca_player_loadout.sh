#/bin/bash

# apt install python3 python3-pip -y

URL_LOADOUT="http://beco.servegame.com:54321/"

# Função de ajuda
usage() {
  echo "Uso: $0 [--player-id \"ID_DO_JOGADOR\"] [--reset-password]"
  exit 1
}

PLAYER_ID=""
RESET_PASSWORD="0"
# Processa os argumentos
while [[ "$#" -gt 0 ]]; do
  case "$1" in
    --player-id)
      PLAYER_ID="$2"
      shift 2
      ;;
    --reset-password)
      RESET_PASSWORD=1
      shift
      ;;
    *)
      echo "Parâmetro desconhecido: $1"
      usage
      ;;
  esac
done

if [[ "$PLAYER_ID" == "" ]]; then
    echo "PlayerID não foi identificado"
    exit 1
fi

# Caminho do ambiente virtual
VENV_DIR="./venv"

# Cria o venv se não existir
if [ ! -d "$VENV_DIR" ]; then
    echo "Criando ambiente virtual..."
    python3 -m venv "$VENV_DIR"
    "$VENV_DIR/bin/pip" install --upgrade pip
    "$VENV_DIR/bin/pip" install werkzeug
fi

# Ativa o ambiente virtual
source "$VENV_DIR/bin/activate"

source ./config.sh

DAYZ_ITEMS_DB="$AppFolder/$AppDayzItemsDbFile"
PLAYERS_BECO_C1_DB="$AppFolder/$AppPlayerBecoC1DbFile"
CURRENT_DATE=$(date "+%d/%m/%Y %H:%M:%S")



PlayerLoadoutExists=$(sqlite3 -separator "|" "$DAYZ_ITEMS_DB" "SELECT login, password, token, active, admin FROM player_logins WHERE player_id = '$PLAYER_ID';")
PlayerExists=$(sqlite3 -separator "|" "$PLAYERS_BECO_C1_DB" "SELECT PlayerName, SteamID, SteamName FROM players_database WHERE PlayerID = '$PLAYER_ID';")

if [[ -z "$PlayerExists" ]]; then
    echo "PlayerID não consta na database"
    exit 1
fi

if [[ -z "$PlayerLoadoutExists" ]]; then
    echo "Inserindo player na database de loadout..."    
else
    echo "Player já está na database de loadout"
fi

if [[ "$RESET_PASSWORD" == "1" ]]; then
    PlayerLoadoutExists=$(sqlite3 -separator "|" "$DAYZ_ITEMS_DB" "SELECT login, password, token, active, admin FROM player_logins WHERE player_id = '$PLAYER_ID';")
    if [[ -z "$PlayerLoadoutExists" ]]; then
        echo "PlayerID não consta na database de loadout"
        exit 1
    fi
    login=$(echo "$PlayerLoadoutExists" | cut -d'|' -f1 | tr -d '|' | sed 's/[^a-zA-Z0-9_ -]//g' | xargs)
    senha=$(LC_ALL=C tr -dc 'a-z0-9' </dev/urandom | head -c 8)    
    hash=$(python3 -c "from werkzeug.security import generate_password_hash; print(generate_password_hash('$senha'))")
    sqlite3 "$DAYZ_ITEMS_DB" "UPDATE player_logins SET password = '$hash' WHERE player_id = '$PLAYER_ID';"
    echo "Senha gerada com sucesso!"
    echo "Acesse: $URL_LOADOUT"
    echo "Login: $login"
    echo "Senha: $senha"
fi
