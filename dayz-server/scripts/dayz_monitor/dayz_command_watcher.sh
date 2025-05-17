#!/bin/bash

# Carrega as variáveis

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PARENT_DIR="$(dirname "$SCRIPT_DIR")"
cd "$PARENT_DIR"
source ./config.sh

COMMAND_FILE="$DayzServerFolder/$DayzActionsToExecuteFile"

echo "Monitorando comandos do DayZ em $COMMAND_FILE..."
echo > "$COMMAND_FILE"

tail -F "$COMMAND_FILE" | while read -r line; do
    # Valida se é um JSON válido
    if ! echo "$line" | jq empty 2>/dev/null; then
        echo ">> Linha inválida (não é JSON): $line"
        continue
    fi

    # Extrai o campo "action"
    action=$(echo "$line" | jq -r '.action')

    case "$action" in
        reset_password)
            player_id=$(echo "$line" | jq -r '.player_id')

            echo ">> Resetando senha de $player_id"

            # Caminho absoluto para garantir que funcione via systemd
            result_json=$("$AppFolder/$AppScriptPlayerLoadoutManagerFile" --player-id "$player_id" --reset-password 2>&1)

            # Verifica se a saída é JSON válido
            if ! echo "$result_json" | jq -e . >/dev/null 2>&1; then
                echo "Erro: saída inválida do script de reset:"
                echo "$result_json"
                echo "$player_id;[ERROR] Erro interno ao resetar senha (formato inválido)" >> "$DayzServerFolder/$DayzMessagesPrivateToSendoFile"
                continue
            fi

            # Verifica erro no JSON retornado
            if echo "$result_json" | jq -e 'has("error")' >/dev/null; then
                erro=$(echo "$result_json" | jq -r '.error')
                echo "Erro do script: $erro"
                echo "$player_id;[ERROR] Erro ao resetar a senha: $erro" >> "$DayzServerFolder/$DayzMessagesPrivateToSendoFile"
                continue
            fi

            login=$(echo "$result_json" | jq -r '.login // empty')
            senha=$(echo "$result_json" | jq -r '.senha // empty')
            url=$(echo "$result_json" | jq -r '.url // empty')

            echo ">> Senha redefinida com sucesso para o jogador $player_id"
            echo "Login: $login"
            echo "Senha: $senha"
            echo "URL: $url"

            echo "$player_id;Nova senha gerada com sucesso. Acesse $url" >> "$DayzServerFolder/$DayzMessagesPrivateToSendoFile"
            echo "$player_id;Nova senha: $senha" >> "$DayzServerFolder/$DayzMessagesPrivateToSendoFile"
            ;;

        active_loadout)
            player_id=$(echo "$line" | jq -r '.player_id')
            loadout_name=$(echo "$line" | jq -r '.loadout_name')
            echo ">> Ativando loadout de $player_id para $loadout_name"

            result_json=$("$AppFolder/$AppScriptPlayerLoadoutManagerFile" --player-id "$player_id" --loadout-name "$loadout_name" --active)

            if echo "$result_json" | jq -e 'has("error")' >/dev/null; then
                erro=$(echo "$result_json" | jq -r '.error')
                echo "Erro ao ativar loadout: $erro"
                echo "$player_id;[ERROR] Erro ao ativar o loadout '$loadout_name': $erro" >> "$DayzServerFolder/$DayzMessagesPrivateToSendoFile"
                continue
            fi

            msg=$(echo "$result_json" | jq -r '.message')
            echo ">> Loadout ativado com sucesso: $msg"
            echo "$player_id;$msg" >> "$DayzServerFolder/$DayzMessagesPrivateToSendoFile"
            ;;

            restart_server)
                minutes=$(echo "$line" | jq -r '.minutes')
                message=$(echo "$line" | jq -r '.message')
                echo ">> Reinício do servidor em $minutes minuto(s): $message"

                # Validação mínima
                if ! [[ "$minutes" =~ ^[0-9]+$ ]] || [[ "$minutes" -le 0 ]]; then
                    echo ">> Valor inválido para minutos: $minutes"
                    continue
                fi

                echo "[ERROR] Atenção: o servidor será reiniciado em $minutes minuto(s)!" >> "$DayzServerFolder/$DayzMessagesToSendoFile"
                sleep 60
                systemctl restart dayz-server
                ;;


        *)
            echo ">> Ação desconhecida: $action"
            ;;
    esac
done
