#!/bin/bash

source ./config.sh

DB="$AppFolder/$AppPlayerBecoC1DbFile"
CURRENT_DATE=$(date "+%d/%m/%Y %H:%M:%S")

ResumoContent=""
Content=""

# Data inicial dos registros
FirstDate=$(sqlite3 "$DB" "SELECT Data FROM players_killfeed ORDER BY Data ASC LIMIT 1;")
if [[ -z "$FirstDate" ]]; then
  ResumoContent="💀 **Ranking de kills (Sem dados coletados!):**"$'\n'
  Content="💀 Ranking de kills (Sem dados coletados!):"$'\n'
else
  ResumoContent+="Obs: Dados coletados a partir de $FirstDate"$'\n'
  Content+="Obs: Dados coletados a partir de $FirstDate"$'\n'

  ResumoContent="💀 **Ranking de kills ($FirstDate à $CURRENT_DATE):**"$'\n'
  Content="💀 Ranking de kills ($FirstDate à $CURRENT_DATE):"$'\n'
fi
ResumoContent+=""$'\n'
Content+=""$'\n'

# Emojis para Top 1 a Top 10
Emojis=("🥇 Top 1" "🥈 Top 2" "🥉 Top 3" "🏅 Top 4" "🏅 Top 5" "🏅 Top 6" "🏅 Top 7" "🏅 Top 8" "🏅 Top 9" "🏅 Top 10")

build_player_block() {
  local rank="$1"
  local name="$2"
  local steam="$3"
  local kills="$4"
  local weapon="$5"
  local longshot="$6"
  local weaponlong="$7"
  local damage="$8"
  local head="$9"
  local torso="${10}"
  local larm="${11}"
  local rarm="${12}"
  local lleg="${13}"
  local rleg="${14}"

  local jogadores_word="jogador"
  [[ "$kills" -gt 1 ]] && jogadores_word="jogadores"

  cat <<EOF
${Emojis[$rank]} - $name matou $kills $jogadores_word
 🔫 Arma preferida: $weapon
 🎯 Tiro de maior distância: ${longshot%.*} metros ($weaponlong)
 💥 Dano total causado: $damage
 🤕 Tiros na cabeça: $head%
 🦺 Tiros no corpo: $torso%
 💪 Tiros no braço esquerdo: $larm%
 💪 Tiros no braço direito: $rarm%
 🦵 Tiros na perna esquerda: $lleg%
 🦵 Tiros na perna direita: $rleg%
...
EOF
}

# Coleta e processa os dados
readarray -t rows < <(sqlite3 -separator '|' "$DB" "
WITH Kills AS (
    SELECT 
        PlayerIDKiller AS PlayerID,
        COUNT(*) AS TotalKills
    FROM players_killfeed
    GROUP BY PlayerIDKiller
),
Damage AS (
    SELECT 
        PlayerIDAttacker AS PlayerID,
        SUM(Damage) AS TotalDamage
    FROM players_damage
    GROUP BY PlayerIDAttacker
),
PreferredWeapon AS (
    SELECT 
        PlayerIDAttacker AS PlayerID,
        Weapon,
        COUNT(*) AS UsageCount
    FROM players_damage
    WHERE Weapon IS NOT NULL AND Weapon != ''
    GROUP BY PlayerIDAttacker, Weapon
),
TopWeapon AS (
    SELECT PlayerID, Weapon
    FROM (
        SELECT 
            PlayerID, 
            Weapon, 
            ROW_NUMBER() OVER (PARTITION BY PlayerID ORDER BY UsageCount DESC) AS rn
        FROM PreferredWeapon
    )
    WHERE rn = 1
),
DamagePerLocation AS (
    SELECT 
        PlayerIDAttacker AS PlayerID,
        LocalDamage,
        SUM(Damage) AS DamageAmount
    FROM players_damage
    GROUP BY PlayerIDAttacker, LocalDamage
),
TotalDamageForPercent AS (
    SELECT 
        PlayerIDAttacker AS PlayerID,
        SUM(Damage) AS TotalDamage
    FROM players_damage
    GROUP BY PlayerIDAttacker
),
DamagePercent AS (
    SELECT 
        d.PlayerID,
        d.LocalDamage,
        ROUND((d.DamageAmount / t.TotalDamage) * 100.0, 2) AS PercentDamage
    FROM DamagePerLocation d
    JOIN TotalDamageForPercent t ON d.PlayerID = t.PlayerID
),
DamagePivot AS (
    SELECT 
        PlayerID,
        MAX(CASE WHEN LocalDamage = 'Head' THEN PercentDamage END) AS Head,
        MAX(CASE WHEN LocalDamage = 'Torso' THEN PercentDamage END) AS Torso,
        MAX(CASE WHEN LocalDamage = 'LeftArm' THEN PercentDamage END) AS LeftArm,
        MAX(CASE WHEN LocalDamage = 'RightArm' THEN PercentDamage END) AS RightArm,
        MAX(CASE WHEN LocalDamage = 'LeftLeg' THEN PercentDamage END) AS LeftLeg,
        MAX(CASE WHEN LocalDamage = 'RightLeg' THEN PercentDamage END) AS RightLeg
    FROM DamagePercent
    GROUP BY PlayerID
),
MaxDistances AS (
    SELECT PlayerIDKiller AS PlayerID, MAX(DistanceMeter) AS MaxKillDistance FROM players_killfeed GROUP BY PlayerIDKiller
    UNION
    SELECT PlayerIDAttacker AS PlayerID, MAX(DistanceMeter) AS MaxKillDistance FROM players_damage GROUP BY PlayerIDAttacker
),
LongestShot AS (
    SELECT 
        PlayerID,
        MAX(MaxKillDistance) AS LongestShot
    FROM MaxDistances
    GROUP BY PlayerID
),
WeaponFromKills AS (
    SELECT PlayerIDKiller AS PlayerID, Weapon, DistanceMeter
    FROM players_killfeed
),
WeaponFromDamage AS (
    SELECT PlayerIDAttacker AS PlayerID, Weapon, DistanceMeter
    FROM players_damage
),
AllShots AS (
    SELECT * FROM WeaponFromKills
    UNION ALL
    SELECT * FROM WeaponFromDamage
),
LongestShotWeapon AS (
    SELECT 
        s.PlayerID,
        s.Weapon
    FROM AllShots s
    JOIN LongestShot l ON s.PlayerID = l.PlayerID AND s.DistanceMeter = l.LongestShot
    GROUP BY s.PlayerID
)

SELECT 
    p.PlayerID,
    p.PlayerName,
    p.SteamID,    
    p.SteamName,
    COALESCE(k.TotalKills, 0) AS TotalKills,
    COALESCE(d.TotalDamage, 0) AS TotalDamage,
    COALESCE(tw.Weapon, 'N/A') AS PreferredWeapon,
    COALESCE(dp.Head, 0) AS Damage_Head_Perc,
    COALESCE(dp.Torso, 0) AS Damage_Torso_Perc,
    COALESCE(dp.LeftArm, 0) AS Damage_LeftArm_Perc,
    COALESCE(dp.RightArm, 0) AS Damage_RightArm_Perc,
    COALESCE(dp.LeftLeg, 0) AS Damage_LeftLeg_Perc,
    COALESCE(dp.RightLeg, 0) AS Damage_RightLeg_Perc,
    COALESCE(ls.LongestShot, 0) AS LongestShotMeters,
    COALESCE(lsw.Weapon, 'N/A') AS WeaponLongestShot
FROM players_database p
LEFT JOIN Kills k ON p.PlayerID = k.PlayerID
LEFT JOIN Damage d ON p.PlayerID = d.PlayerID
LEFT JOIN TopWeapon tw ON p.PlayerID = tw.PlayerID
LEFT JOIN DamagePivot dp ON p.PlayerID = dp.PlayerID
LEFT JOIN LongestShot ls ON p.PlayerID = ls.PlayerID
LEFT JOIN LongestShotWeapon lsw ON p.PlayerID = lsw.PlayerID
WHERE COALESCE(k.TotalKills, 0) > 0 OR COALESCE(d.TotalDamage, 0) > 0
ORDER BY TotalKills DESC, TotalDamage DESC
LIMIT 10;
")

# Se não houver estatísticas, aborta envio
if [ ${#rows[@]} -eq 0 ]; then
  echo "Nenhuma estatística para enviar. Abortando envio ao Discord."
  exit 0
fi

# Loop de jogadores
for i in "${!rows[@]}"; do
  IFS='|' read -r PlayerID PlayerName SteamID SteamName TotalKills TotalDamage PreferredWeapon \
    Damage_Head_Perc Damage_Torso_Perc Damage_LeftArm_Perc Damage_RightArm_Perc \
    Damage_LeftLeg_Perc Damage_RightLeg_Perc LongestShotMeters WeaponLongestShot <<< "${rows[$i]}"

  [[ -z "$PlayerName" ]] && PlayerName="Desconhecido"
  [[ -z "$PreferredWeapon" ]] && PreferredWeapon="Desconhecido"
  [[ -z "$LongestShotMeters" ]] && LongestShotMeters="0"
  [[ -z "$TotalKills" ]] && TotalKills="0"

  if [[ -n "$SteamID" && -n "$SteamName" ]]; then
    link_steam="[$SteamName](<https://steamcommunity.com/profiles/$SteamID>)"
  else
    link_steam="**Desconhecido**"
  fi

  player_info="**$PlayerName** ($link_steam)"

  # Bloco resumido para mensagem do Discord
  jogadores_word="jogador"
  [[ "$TotalKills" -gt 1 ]] && jogadores_word="jogadores"
  ResumoContent+="${Emojis[$i]} - $player_info matou $TotalKills $jogadores_word com dano total de $TotalDamage"
  ResumoContent+=$'\n'

  # Bloco detalhado para arquivo .txt
  Content+=$(build_player_block "$i" "$player_info" "$link_steam" "$TotalKills" "$PreferredWeapon" \
    "$LongestShotMeters" "$WeaponLongestShot" "$TotalDamage" "$Damage_Head_Perc" "$Damage_Torso_Perc" \
    "$Damage_LeftArm_Perc" "$Damage_RightArm_Perc" "$Damage_LeftLeg_Perc" "$Damage_RightLeg_Perc")
  Content+=$'\n'
done

# Salva conteúdo completo no arquivo
#echo "$Content" > /tmp/ranking.txt
output_file="/tmp/ranking.txt"
echo -e '\xEF\xBB\xBF'"$Content" > "$output_file"

# Concatene tudo em uma variável
ResumoContent+=$'\n'
mensagem="${ResumoContent}📎 Detalhes completos no arquivo anexo."

# Gere o JSON escapado corretamente
json_payload=$(jq -n --arg content "$mensagem" '{content: $content}')

echo "$json_payload" | jq

curl -s -X POST \
  -H "Authorization: Bot $DiscordChannelPlayersStatsBotToken" \
  -F "payload_json=$json_payload" \
  -F "file=@/tmp/ranking.txt" \
  "https://discord.com/api/v10/channels/$DiscordChannelPlayersStatsChannelId/messages"

# Limpar base de dano e kill
DELETE_KILLFEED
DELETE_PLAYER_DAMAGE
rm $output_file