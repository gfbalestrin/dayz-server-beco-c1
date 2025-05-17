#!/bin/bash

LOG_FILE="/home/dayzadmin/servers/dayz-server/profiles/dayz-server.err"

echo "Monitorando logs em $LOG_FILE..."

tail -F "$LOG_FILE" | while read -r line; do
    if [[ "$line" == *"Player disconnected"* ]]; then
        echo "Jogador saiu: $line"
        # Coloque ação aqui, como logar ou limpar algo
    fi
done
