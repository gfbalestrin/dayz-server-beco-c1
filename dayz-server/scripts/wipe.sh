#!/bin/bash

# Carrega as variáveis
source ./config.sh

if [[ "$DayzWipeOnRestart" != "1" ]]; then
    echo "Ignorando wipe devido a config do json"
    INSERT_CUSTOM_LOG "Ignorando wipe devido a config do json" "INFO" "$ScriptName"
    exit 0
fi

MpMission="dayzOffline.chernarusplus"
echo "=== Realizando wipe do servidor DayZ ==="
INSERT_CUSTOM_LOG "Realizando wipe do servidor DayZ" "INFO" "$ScriptName"

PROFILE_DIR="$DayzServerFolder/mpmissions/$MpMission/storage_1"
echo "PROFILE_DIR: $PROFILE_DIR"
INSERT_CUSTOM_LOG "PROFILE_DIR: $PROFILE_DIR" "INFO" "$ScriptName"
rm -rf "$PROFILE_DIR"

echo "Wipe completo!"
#ls -lh "$PROFILE_DIR"
sleep 10

