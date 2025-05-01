#!/bin/bash
source ./config.sh


PLAYERS_BECO_C1_DB="$AppFolder/$AppPlayerBecoC1DbFile"
CHECK_INTERVAL=60
DB_FILENAME="$DayzServerFolder/$DayzPlayerDbFile"
FIRST_RUN=1

while true; do
  if [ $FIRST_RUN == 0 ]; then
    echo "Waiting $CHECK_INTERVAL to start the file check..."
    sleep $CHECK_INTERVAL
  fi
  FIRST_RUN=0

  echo "Excluindo registros de backups > 24h..."
  DELETED=$(sqlite3 "$PLAYERS_BECO_C1_DB" <<EOF
BEGIN;
DELETE FROM players_coord_backup
WHERE TimeStamp < datetime('now', '-1 day');
SELECT changes();
COMMIT;
EOF
)

  # Mostra o resultado
  echo "Registros excluídos: $DELETED"

  echo "Buscando players online no banco $PLAYERS_BECO_C1_DB..."
  while IFS="|" read -r PlayerId PlayerName SteamID SteamName DataConnect; do

    echo "Analisando Player $PlayerName (SteamName)..."
    CURRENT_DATE=$(date "+%Y-%m-%d %H:%M:%S")
    BACKUP=$(sqlite3 "$DB_FILENAME" "SELECT hex(Data) FROM Players where UID = '$PlayerId';")
    if [ "$BACKUP" == "" ]; then
      echo "Player Data esta em branco. Aguardando 30 segundos..."
      continue
    fi
    bytes_dbversion=${BACKUP:0:4}

    hex_position_x=${BACKUP:4:8}
    float=$(echo $hex_position_x | xxd -r -p | od -An -t fF | tr -d ' ')
    LastPositionX=$float
    if [ "$LastPositionX" == "" ]; then
      echo "A posicao X esta em branco. Aguardando 30 segundos..."
      continue
    fi

    hex_position_z=${BACKUP:12:8}
    float=$(echo $hex_position_z | xxd -r -p | od -An -t fF | tr -d ' ')
    LastPositionZ=$float

    hex_position_y=${BACKUP:20:8}
    float=$(echo $hex_position_y | xxd -r -p | od -An -t fF | tr -d ' ')
    LastPositionY=$float
    if [ "$LastPositionY" == "" ]; then
      echo "A posicao Y esta em branco. Aguardando 30 segundos..."
      continue
    fi

    echo "LastPositionX: $LastPositionX"
    echo "LastPositionZ: $LastPositionZ"
    echo "LastPositionY: $LastPositionY"

    last_record=$(sqlite3 -separator '|' "$PLAYERS_BECO_C1_DB" "
    SELECT 
        PlayerCoordId,
        CoordX,
        CoordZ,
        CoordY 
    FROM 
        players_coord
    WHERE 
        PlayerID = '$PlayerId'
    ORDER BY 
        PlayerCoordId DESC
    LIMIT 1;
    ")

    # Separar os valores retornados
    IFS='|' read -r PlayerCoordId CoordX CoordZ CoordY <<<"$last_record"

    echo "CurrentePositionX: $CoordX"
    echo "CurrentePositionZ: $CoordZ"
    echo "CurrentePositionY: $CoordY"

    if [[ "$LastPositionX" == "$CoordX" && "$LastPositionY" == "$CoordY" ]]; then
      echo "A posição não foi alterada! Indo para o próximo..."
      continue
    fi

    echo "A posição foi alterada. Inserindo novas coordenadas no banco de dados..."

    MAX_ATTEMPTS=5
    ATTEMPT=1

    while [ $ATTEMPT -le $MAX_ATTEMPTS ]; do
      echo "Tentativa $ATTEMPT de $MAX_ATTEMPTS..."

      sqlite3 "$PLAYERS_BECO_C1_DB" <<EOF
PRAGMA foreign_keys = ON;
BEGIN IMMEDIATE TRANSACTION;

INSERT INTO players_coord (PlayerID, CoordX, CoordZ, CoordY, Data)
VALUES ('$PlayerId', $LastPositionX, $LastPositionZ, $LastPositionY, '$CURRENT_DATE');

INSERT INTO players_coord_backup (PlayerCoordId, Backup, TimeStamp)
VALUES (
    (SELECT last_insert_rowid()),
    X'$(echo -n "$BACKUP" | xxd -p | tr -d '\n')',
    datetime('now', 'localtime')
);

COMMIT;
EOF

      if [ $? -eq 0 ]; then
        echo "Inserção concluída com sucesso."
        break
      else
        echo "Falha na tentativa $ATTEMPT. Aguardando para tentar novamente..."
        sleep $((ATTEMPT * 2)) # tempo de espera crescente
        ((ATTEMPT++))
      fi
    done

    if [ $ATTEMPT -gt $MAX_ATTEMPTS ]; then
      echo "Erro: não foi possível inserir os dados após $MAX_ATTEMPTS tentativas."
      exit 1
    fi

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
done