#!/bin/bash
cd "/home/dayzadmin/servers/dayz-server/scripts/"
source config.sh
set -euo pipefail

echo "[INFO] Iniciando update do servidor DayZ..."

# Executa o wipe se estiver habilitado
if [[ "$DayzWipeOnRestart" -eq "1" ]]; then
    echo "[INFO] Executando wipe..."
    cd "/home/dayzadmin/servers/dayz-server/scripts" && ./wipe.sh
    echo "[INFO] Wipe concluído."
fi

# Atualiza o servidor via SteamCMD
echo "[INFO] Atualizando servidor via SteamCMD..."
cd "/home/dayzadmin/servers/dayz-server"
/home/dayzadmin/servers/steamcmd/steamcmd.sh +force_install_dir "/home/dayzadmin/servers/dayz-server/" +login gfbalestrin2 +app_update 223350 +quit


#cd "$DayzServerFolder/mpmissions/dayzOffline.chernarusplus/"
#rm init.c
#if [[ "$DayzDeathmatch" == "1" ]]; then
#  echo "Baixando init.c para Deathmatch..."
#  curl -o init.c https://raw.githubusercontent.com/gfbalestrin/dayz-server-beco-c1/refs/heads/main/Installation/mods/deathmatch/init.c
#else
#  echo "Baixando init.c..."
#  curl -o init.c https://raw.githubusercontent.com/gfbalestrin/dayz-server-beco-c1/refs/heads/main/dayz-server/mpmissions/dayzOffline.chernarusplus/init.c

#fi
#chown "dayzadmin:dayzadmin" init.c

# if [[ "$DayzDeathmatch" -eq "1" ]]; then
#     XML_FILE="/home/dayzadmin/servers/dayz-server/mpmissions/dayzOffline.chernarusplus/db/globals.xml"

#     # Backup antes de editar
#     cp "$XML_FILE" "$XML_FILE.bak"

#     # Lista de alterações: nome do parâmetro e novo valor
#     declare -A updates=(
#     ["AnimalMaxCount"]="10"
#     ["CleanupLifetimeDeadAnimal"]="30"
#     ["CleanupLifetimeDeadInfected"]="30"
#     ["CleanupLifetimeDeadPlayer"]="60"
#     ["CleanupLifetimeDefault"]="30"
#     ["CleanupLifetimeLimit"]="20"
#     ["CleanupLifetimeRuined"]="20"
#     )

#     # Itera sobre o mapa de alterações
#     for name in "${!updates[@]}"; do
#     new_value="${updates[$name]}"

#     # Atualiza o valor usando xmlstarlet
#     xmlstarlet ed -L \
#         -u "/variables/var[@name='$name']/@value" \
#         -v "$new_value" "$XML_FILE"

#     echo "Atualizado: $name = $new_value"
#     done

#     echo "Alterações concluídas. Backup criado em $XML_FILE.bak"
# fi

echo "[INFO] Update concluído com sucesso."
