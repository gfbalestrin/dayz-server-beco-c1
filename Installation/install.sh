#!/bin/bash

DELAY=1

# Função de ajuda
usage() {
  echo "Uso: $0 [--skip-user] [--skip-steam]"
  exit 1
}

SKIP_USER=0
SKIP_STEAM=0

# Processa os argumentos
while [[ "$#" -gt 0 ]]; do
  case "$1" in
    --skip-user)
      SKIP_USER=1
      shift
      ;;
    --skip-steam)
      SKIP_STEAM=1
      shift
      ;;
    *)
      echo "Parâmetro desconhecido: $1"
      usage
      ;;
  esac
done

[[ "$SKIP_USER" -eq 1 ]] && echo "Flag --skip-user foi ativada"
[[ "$SKIP_STEAM" -eq 1 ]] && echo "Flag --skip-steam foi ativada"

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
apt -y install jq curl wget

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

# Verifica se o sistema é baseado em Debian
if grep -qi 'debian' /etc/os-release; then
    echo "Distribuição baseada em Debian detectada."
    
    # Obtém a versão do Debian
    VERSION=$(grep VERSION_ID /etc/os-release | cut -d '=' -f2 | tr -d '"')
    echo "Versão do Debian: $VERSION"
    if [[ "$VERSION" -gt 9 ]]; then
        echo "Versão do Debian é suportada."
    else
        echo "Versão do Debian não suportada. Apenas Debian 10, 11 e 12 são suportados."
        exit 1
    fi
else
    echo "Distribuição não é baseada em Debian."
    exit 1
fi

if [[ "$SKIP_USER" -eq 0 ]]; then
    # Verifica se o usuário já existe
    if id "$LinuxUserName" &>/dev/null; then
        echo "Erro: o usuário '$LinuxUserName' já existe."
        exit 1
    fi

    # Cria o usuário
    useradd -m -s /bin/bash "$LinuxUserName"

    # Define a senha
    echo "${LinuxUserName}:${LinuxUserPassword}" | chpasswd

    echo "Usuário '$LinuxUserName' criado com sucesso com a senha predefinida."

    # Adiciona o usuário ao grupo sudo
    usermod -aG sudo "$LinuxUserName"
    echo "Usuário '$LinuxUserName' adicionado ao grupo sudo."

    # Cria um arquivo sudoers dedicado para o usuário
    SUDOERS_FILE="/etc/sudoers.d/$LinuxUserName"

    echo "$LinuxUserName ALL=(ALL) NOPASSWD:ALL" > "$SUDOERS_FILE"
    chmod 440 "$SUDOERS_FILE"

    echo "✅ Usuário '$LinuxUserName' pode usar sudo sem senha."
else
    echo "❌ Usuário '$LinuxUserName' não foi criado, pois a flag --skip-user foi ativada."
fi

sleep $DELAY

if [[ "$SKIP_STEAM" -eq 0 ]]; then
    apt-get -y install lib32gcc-s1
    cd "/home/$LinuxUserName"
    mkdir -p "servers/steamcmd" && cd servers/steamcmd
    curl -sqL "https://steamcdn-a.akamaihd.net/client/installer/steamcmd_linux.tar.gz" | tar zxvf -    
    #chown -R "$LinuxUserName:$LinuxUserName" "/home/$LinuxUserName/servers"
    #su -c "./steamcmd.sh +force_install_dir "/home/$LinuxUserName/servers/dayz-server/" +login $SteamAccount +app_update 223350 +quit" $LinuxUserName
    sudo -u "$LinuxUserName" ./steamcmd.sh +force_install_dir "/home/$LinuxUserName/servers/dayz-server/" +login "$SteamAccount" +app_update 223350 +quit

    #./steamcmd.sh +force_install_dir "/home/$LinuxUserName/servers/dayz-server/" +login $SteamAccount +app_update 223350 +quit

    echo "SteamCMD instalado com sucesso."
else
    echo "❌ Steam não foi instalado, pois a flag --skip-steam foi ativada."
fi

sleep $DELAY

DayzFolder="/home/$LinuxUserName/servers/dayz-server"
echo "Entrando no diretório $DayzFolder"
cd "$DayzFolder"

ServerDZFile="$DayzFolder/serverDZ.cfg"

cp -Rap $ServerDZFile serverDZ.cfg.bkp

stringSearchHostname="hostname = \"EXAMPLE NAME\";"
stringReplaceHostname="hostname = \"$DayzServerName\";"
if ! grep -q "$stringSearchHostname" "$ServerDZFile"; then
    echo "Não foi possível encontrar a linha '$stringSearchHostname' no arquivo $ServerDZFile"
    echo "Copie o arquivo server-defaults/serverDZ.cfg para $ServerDZFile e edite-o manualmente."
    exit 1
fi

stringSearchPasswordAdmin="passwordAdmin = \"\";"
stringReplacePasswordAdmin="passwordAdmin = \"$DayzPasswordAdmin\";"
if ! grep -q "$stringSearchPasswordAdmin" "$ServerDZFile"; then
    echo "Não foi possível encontrar a linha '$stringSearchPasswordAdmin' no arquivo $ServerDZFile"
    echo "Copie o arquivo server-defaults/serverDZ.cfg para $ServerDZFile e edite-o manualmente."
    exit 1
fi

stringSearchMaxPlayers="maxPlayers = 60;"
stringReplaceMaxPlayers="maxPlayers = $DayzMaxPlayers;"
if ! grep -q "$stringSearchMaxPlayers" "$ServerDZFile"; then
    echo "Não foi possível encontrar a linha '$stringSearchMaxPlayers' no arquivo $ServerDZFile"
    echo "Copie o arquivo server-defaults/serverDZ.cfg para $ServerDZFile e reincie o script com as flags --skip-user e --skip-steam."
    exit 1
fi

echo "Editando o arquivo $ServerDZFile ..."

sed -i "s#${stringSearchHostname}#${stringReplaceHostname}#g" "$ServerDZFile"
sed -i "s#${stringSearchPasswordAdmin}#${stringReplacePasswordAdmin}#g" "$ServerDZFile"
sed -i "s#${stringSearchMaxPlayers}#${stringReplaceMaxPlayers}#g" "$ServerDZFile"

# Modificar
#disable3rdPerson=0;         // Toggles the 3rd person view for players (value 0-1)
sed -i "s#disable3rdPerson=0;#disable3rdPerson=1;#g" "$ServerDZFile"
#disableCrosshair=0;         // Toggles the cross-hair (value 0-1)
sed -i "s#disableCrosshair=0;#disableCrosshair=1;#g" "$ServerDZFile"
#lightingConfig = 0;         // 0 for brighter night setup, 1 for darker night setup
sed -i "s#lightingConfig=0;#lightingConfig=1;#g" "$ServerDZFile"
#serverTimeAcceleration=12;  // Accelerated Time (value 0-24)// This is a time multiplier for in-game time. In this case, the time would move 24 times faster than normal, so an entire day would pass in one hour.
sed -i "s#serverTimeAcceleration=12;#serverTimeAcceleration=6;#g" "$ServerDZFile"
#serverNightTimeAcceleration=1;  // Accelerated Nigh Time - The numerical value being a multiplier (0.1-64) and also multiplied by serverTimeAcceleration value. Thus, in case it is set to 4 and serverTimeAcceleration is set to 2, night time would move 8 times faster than normal. An entire night would pass in 3 hours.
sed -i "s#serverNightTimeAcceleration=1;#serverNightTimeAcceleration=4;#g" "$ServerDZFile"
#serverTimePersistent=0;     // Persistent Time (value 0-1)// The actual server time is saved to storage, so when active, the next server start will use the saved time value.
sed -i "s#serverTimePersistent=0;#serverTimePersistent=1;#g" "$ServerDZFile"

# Adicionar antes de 'class Missions'
motd="motd[] = { \"$DayzMotdMessage\" };"
sed -i "/class Missions/i $motd" "$ServerDZFile"
sed -i "/class Missions/i motdInterval = $DayzMotdIntervalSeconds;" "$ServerDZFile"
sed -i "/class Missions/i BattlEye = 1;" "$ServerDZFile" 

echo "Arquivo $ServerDZFile editado com sucesso."

DayzSettingXmlFile="$DayzFolder/dayzsetting.xml"
echo "Editando arquivo $DayzSettingXmlFile ..."
sleep $DELAY

stringSearchMaxCores="maxcores=\"2\""
stringReplaceMaxCores="maxcores=\"$DayzPcCpuMaxCores\""
if ! grep -q "$stringSearchMaxCores" "$DayzSettingXmlFile"; then
    echo "Não foi possível encontrar a linha '$stringSearchMaxCores' no arquivo $DayzSettingXmlFile"
    echo "Copie o arquivo server-defaults/dayzsetting.cfg para $DayzSettingXmlFile e reincie o script com as flags --skip-user e --skip-steam."
    exit 1
fi

stringSearchReservedcores="reservedcores=\"1\""
stringReplaceReservedcores="reservedcores=\"$DayzPcCpuReservedcores\""
if ! grep -q "$stringSearchReservedcores" "$DayzSettingXmlFile"; then
    echo "Não foi possível encontrar a linha '$stringSearchReservedcores' no arquivo $DayzSettingXmlFile"
    echo "Copie o arquivo server-defaults/dayzsetting.cfg para $DayzSettingXmlFile e reincie o script com as flags --skip-user e --skip-steam."
    exit 1
fi

echo "Arquivo $DayzSettingXmlFile editado com sucesso."

DayzBeServerFile="$DayzFolder/battleye/beserver_x64.cfg"
echo "Configurando integração com RCtools no arquivo $DayzBeServerFile ..."
sleep $DELAY

echo "RConPassword $DayzRConPassword" > "$DayzBeServerFile"
echo "RConIP $DayzRConIP" >> "$DayzBeServerFile"
echo "RConPort $DayzRConPort" >> "$DayzBeServerFile"
echo "MaxPing $DayzMaxPing" >> "$DayzBeServerFile"
echo "RestrictRCon $DayzRestrictRCon" >> "$DayzBeServerFile"

echo "Arquivo $DayzBeServerFile editado com sucesso."

DayzMpmissionMessagesXml="$DayzFolder/mpmissions/$DayzMpmission/db/messages.xml"
cp -Rap $DayzMpmissionMessagesXml "$DayzFolder/mpmissions/$DayzMpmission/db/messages.xml.bkp"
echo "Editando arquivo $DayzMpmissionMessagesXml ..."
sleep $DELAY

awk '
/<\/messages>/ {
    print "<message>";
    print "    <deadline>240</deadline>";
    print "    <shutdown>1</shutdown>";
    print "    <text>O servidor vai ser reiniciado em #tmin minutos.</text>";
    print "</message>";
}
{ print }
' $DayzMpmissionMessagesXml > tmp.xml && mv tmp.xml $DayzMpmissionMessagesXml

echo "Arquivo $DayzMpmissionMessagesXml editado com sucesso."
DayzServerServiceFile="/etc/systemd/system/dayz-server.service"
echo "Configurando serviço no systemd $DayzServerServiceFile ..."
sleep $DELAY

echo > "$DayzServerServiceFile"

echo "[Unit]" >> "$DayzServerServiceFile"
echo "Description=DayZ Dedicated Server" >> "$DayzServerServiceFile"
echo "Wants=network-online.target" >> "$DayzServerServiceFile"
echo "After=syslog.target network.target nss-lookup.target network-online.target" >> "$DayzServerServiceFile"
echo "" >> "$DayzServerServiceFile"
echo "[Service]" >> "$DayzServerServiceFile"
echo "ExecStartPre=$DayzFolder/update.sh" >> "$DayzServerServiceFile"
echo "ExecStart=$DayzFolder/DayZServer -config=serverDZ.cfg -port=2302 -BEpath=$DayzFolder/battleye -profiles=profiles -dologs -adminlog -netlog -freezecheck -cpuCount=$DayzPcCpuMaxCores -limitFPS=$DayzLimitFPS" >> "$DayzServerServiceFile"
echo "ExecStartPost=+$DayzFolder/execute_script_pos.sh" >> "$DayzServerServiceFile"
echo "WorkingDirectory=$DayzFolder/" >> "$DayzServerServiceFile"
echo "LimitNOFILE=100000" >> "$DayzServerServiceFile"
echo "ExecReload=/bin/kill -s HUP \$MAINPID" >> "$DayzServerServiceFile"
echo "ExecStop=/bin/kill -s INT \$MAINPID" >> "$DayzServerServiceFile"
echo "User=$LinuxUserName" >> "$DayzServerServiceFile"
echo "Group=$LinuxUserName" >> "$DayzServerServiceFile"
echo "Restart=on-failure" >> "$DayzServerServiceFile"
echo "RestartSec=30s" >> "$DayzServerServiceFile"
echo "" >> "$DayzServerServiceFile"
echo "[Install]" >> "$DayzServerServiceFile"
echo "WantedBy=multi-user.target" >> "$DayzServerServiceFile"

echo "Configurando script de update $DayzFolder/update.sh ..."
echo "#!/bin/bash" > "$DayzFolder/update.sh"
if [[ "$DayzWipeOnRestart" -eq "1" ]]; then
    echo "cd /home/dayzadmin/servers/dayz-server/scripts/ && ./wipe.sh" >> "$DayzFolder/update.sh"
fi
echo "/home/$LinuxUserName/servers/steamcmd/steamcmd.sh +force_install_dir $DayzFolder/ +login $SteamAccount +app_update 223350 +quit" >> "$DayzFolder/update.sh"
chmod +x "$DayzFolder/update.sh"

echo "Configurando script de pós inicialização $DayzFolder/execute_script_pos.sh ..."
echo "#!/bin/bash" > "$DayzFolder/execute_script_pos.sh"
echo "" >> "$DayzFolder/execute_script_pos.sh"
chmod +x "$DayzFolder/execute_script_pos.sh"

chown -R "$LinuxUserName:$LinuxUserName" "/home/$LinuxUserName/servers"

#sudo -u dayzadmin /home/dayzadmin/servers/steamcmd/steamcmd.sh +login $SteamAccount

systemctl enable dayz-server.service
systemctl restart dayz-server.service
journalctl -f -u dayz-server.service
echo "Serviço do DayZ configurado e iniciado com sucesso."