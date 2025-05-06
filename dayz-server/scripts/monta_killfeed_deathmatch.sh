#!/bin/bash

source ./config.sh

PLAYERS_BECO_C1_DB="$AppFolder/$AppPlayerBecoC1DbFile"

CURRENT_DATE=`date "+%d/%m/%Y %H:%M:%S"`
Content="💀 **Ranking geral de kills (atualizado em $CURRENT_DATE):**\n\n"
Content+="..."
URL="https://discord.com/api/v10/channels/$DiscordChannelPlayersStatsChannelId/messages/1369177067366912121"
response=$(curl -s -X PATCH \
-H "Authorization: Bot $DiscordChannelPlayersStatsBotToken" \
-H "Content-Type: application/json" \
-d "{\"content\": \"$Content\"}" \
"$URL")
sleep 1
i=1
while IFS='|' read -r PlayerID PlayerName SteamID SteamName TotalKills TotalDamage PreferredWeapon Damage_Head_Perc Damage_Torso_Perc Damage_LeftArm_Perc Damage_RightArm_Perc Damage_LeftLeg_Perc Damage_RightLeg_Perc LongestShotMeters WeaponLongestShot; do

    [ -z "$PlayerName" ] && PlayerName="Desconhecido"
    [ -z "$PreferredWeapon" ] && Weapon="Soco"
    [ -z "$LongestShotMeters" ] && LongestShotMeters="0"
    [ -z "$TotalKills" ] && TotalKills="0"
    link_steam="**Desconhecido**"
    if [[ $SteamID != "" && $SteamName != "" ]]; then
        link_steam="[$SteamName](<https://steamcommunity.com/profiles/$SteamID>)"
    fi
    player_info="**$PlayerName** ($link_steam)"
    metros=$(echo $LongestShotMeters | cut -d '.' -f 1)

    if [ $i -eq 1 ]; then
        Content="🥇 Top 1 - $player_info matou $TotalKills jogadores \n 🔫 Arma preferida: $PreferredWeapon \n 🎯 Tiro de maior distância: $metros metros ($WeaponLongestShot) \n 💥 Dano total causado: $TotalDamage \n 🤕 Tiros na cabeça: $Damage_Head_Perc% \n 🦺 Tiros no corpo: $Damage_Torso_Perc% \n 💪 Tiros no braço esquerdo: $Damage_LeftArm_Perc% \n 💪 Tiros no braço direito: $Damage_RightArm_Perc% \n 🦵 Tiros na perna esquerda: $Damage_LeftLeg_Perc% \n 🦵 Tiros na perna direita: $Damage_RightLeg_Perc% \n"
        Content+="..."
        URL="https://discord.com/api/v10/channels/$DiscordChannelPlayersStatsChannelId/messages/1369177068822204436"
        response=$(curl -s -X PATCH \
        -H "Authorization: Bot $DiscordChannelPlayersStatsBotToken" \
        -H "Content-Type: application/json" \
        -d "{\"content\": \"$Content\"}" \
        "$URL")
        sleep 1
    elif [ $i -eq 2 ]; then
        Content="🥈 Top 2 - $player_info matou $TotalKills jogadores \n 🔫 Arma preferida: $PreferredWeapon \n 🎯 Tiro de maior distância: $metros metros ($WeaponLongestShot) \n 💥 Dano total causado: $TotalDamage \n 🤕 Tiros na cabeça: $Damage_Head_Perc% \n 🦺 Tiros no corpo: $Damage_Torso_Perc% \n 💪 Tiros no braço esquerdo: $Damage_LeftArm_Perc% \n 💪 Tiros no braço direito: $Damage_RightArm_Perc% \n 🦵 Tiros na perna esquerda: $Damage_LeftLeg_Perc% \n 🦵 Tiros na perna direita: $Damage_RightLeg_Perc% \n"
        Content+="..."
        URL="https://discord.com/api/v10/channels/$DiscordChannelPlayersStatsChannelId/messages/1369177069912723497"
        response=$(curl -s -X PATCH \
        -H "Authorization: Bot $DiscordChannelPlayersStatsBotToken" \
        -H "Content-Type: application/json" \
        -d "{\"content\": \"$Content\"}" \
        "$URL")
        sleep 1
    elif [ $i -eq 3 ]; then
    	Content="🥉 Top 3 - $player_info matou $TotalKills jogadores \n 🔫 Arma preferida: $PreferredWeapon \n 🎯 Tiro de maior distância: $metros metros ($WeaponLongestShot) \n 💥 Dano total causado: $TotalDamage \n 🤕 Tiros na cabeça: $Damage_Head_Perc% \n 🦺 Tiros no corpo: $Damage_Torso_Perc% \n 💪 Tiros no braço esquerdo: $Damage_LeftArm_Perc% \n 💪 Tiros no braço direito: $Damage_RightArm_Perc% \n 🦵 Tiros na perna esquerda: $Damage_LeftLeg_Perc% \n 🦵 Tiros na perna direita: $Damage_RightLeg_Perc% \n"
        Content+="..."
        URL="https://discord.com/api/v10/channels/$DiscordChannelPlayersStatsChannelId/messages/1369177070839533700"
        response=$(curl -s -X PATCH \
        -H "Authorization: Bot $DiscordChannelPlayersStatsBotToken" \
        -H "Content-Type: application/json" \
        -d "{\"content\": \"$Content\"}" \
        "$URL")
        sleep 1
    elif [ $i -eq 4 ]; then
        Content="🏅 Top 4 - $player_info matou $TotalKills jogadores \n 🔫 Arma preferida: $PreferredWeapon \n 🎯 Tiro de maior distância: $metros metros ($WeaponLongestShot) \n 💥 Dano total causado: $TotalDamage \n 🤕 Tiros na cabeça: $Damage_Head_Perc% \n 🦺 Tiros no corpo: $Damage_Torso_Perc% \n 💪 Tiros no braço esquerdo: $Damage_LeftArm_Perc% \n 💪 Tiros no braço direito: $Damage_RightArm_Perc% \n 🦵 Tiros na perna esquerda: $Damage_LeftLeg_Perc% \n 🦵 Tiros na perna direita: $Damage_RightLeg_Perc% \n"
        Content+="..."
        URL="https://discord.com/api/v10/channels/$DiscordChannelPlayersStatsChannelId/messages/1369177072370712577"
        response=$(curl -s -X PATCH \
        -H "Authorization: Bot $DiscordChannelPlayersStatsBotToken" \
        -H "Content-Type: application/json" \
        -d "{\"content\": \"$Content\"}" \
        "$URL")
        sleep 1
    elif [ $i -eq 5 ]; then
        Content="🏅 Top 5 - $player_info matou $TotalKills jogadores \n 🔫 Arma preferida: $PreferredWeapon \n 🎯 Tiro de maior distância: $metros metros ($WeaponLongestShot) \n 💥 Dano total causado: $TotalDamage \n 🤕 Tiros na cabeça: $Damage_Head_Perc% \n 🦺 Tiros no corpo: $Damage_Torso_Perc% \n 💪 Tiros no braço esquerdo: $Damage_LeftArm_Perc% \n 💪 Tiros no braço direito: $Damage_RightArm_Perc% \n 🦵 Tiros na perna esquerda: $Damage_LeftLeg_Perc% \n 🦵 Tiros na perna direita: $Damage_RightLeg_Perc% \n"
        Content+="..."
        URL="https://discord.com/api/v10/channels/$DiscordChannelPlayersStatsChannelId/messages/1369177073721020570"
        response=$(curl -s -X PATCH \
        -H "Authorization: Bot $DiscordChannelPlayersStatsBotToken" \
        -H "Content-Type: application/json" \
        -d "{\"content\": \"$Content\"}" \
        "$URL")
        sleep 1
    elif [ $i -eq 6 ]; then
        Content="🏅 Top 6 - $player_info matou $TotalKills jogadores \n 🔫 Arma preferida: $PreferredWeapon \n 🎯 Tiro de maior distância: $metros metros ($WeaponLongestShot) \n 💥 Dano total causado: $TotalDamage \n 🤕 Tiros na cabeça: $Damage_Head_Perc% \n 🦺 Tiros no corpo: $Damage_Torso_Perc% \n 💪 Tiros no braço esquerdo: $Damage_LeftArm_Perc% \n 💪 Tiros no braço direito: $Damage_RightArm_Perc% \n 🦵 Tiros na perna esquerda: $Damage_LeftLeg_Perc% \n 🦵 Tiros na perna direita: $Damage_RightLeg_Perc% \n"
        Content+="..."
        URL="https://discord.com/api/v10/channels/$DiscordChannelPlayersStatsChannelId/messages/1369177209616465940"
        response=$(curl -s -X PATCH \
        -H "Authorization: Bot $DiscordChannelPlayersStatsBotToken" \
        -H "Content-Type: application/json" \
        -d "{\"content\": \"$Content\"}" \
        "$URL")
        sleep 1
    elif [ $i -eq 7 ]; then
        Content="🏅 Top 7 - $player_info matou $TotalKills jogadores \n 🔫 Arma preferida: $PreferredWeapon \n 🎯 Tiro de maior distância: $metros metros ($WeaponLongestShot) \n 💥 Dano total causado: $TotalDamage \n 🤕 Tiros na cabeça: $Damage_Head_Perc% \n 🦺 Tiros no corpo: $Damage_Torso_Perc% \n 💪 Tiros no braço esquerdo: $Damage_LeftArm_Perc% \n 💪 Tiros no braço direito: $Damage_RightArm_Perc% \n 🦵 Tiros na perna esquerda: $Damage_LeftLeg_Perc% \n 🦵 Tiros na perna direita: $Damage_RightLeg_Perc% \n"
        Content+="..."
        URL="https://discord.com/api/v10/channels/$DiscordChannelPlayersStatsChannelId/messages/1369177211139002419"
        response=$(curl -s -X PATCH \
        -H "Authorization: Bot $DiscordChannelPlayersStatsBotToken" \
        -H "Content-Type: application/json" \
        -d "{\"content\": \"$Content\"}" \
        "$URL")
        sleep 1
    elif [ $i -eq 8 ]; then
        Content="🏅 Top 8 - $player_info matou $TotalKills jogadores \n 🔫 Arma preferida: $PreferredWeapon \n 🎯 Tiro de maior distância: $metros metros ($WeaponLongestShot) \n 💥 Dano total causado: $TotalDamage \n 🤕 Tiros na cabeça: $Damage_Head_Perc% \n 🦺 Tiros no corpo: $Damage_Torso_Perc% \n 💪 Tiros no braço esquerdo: $Damage_LeftArm_Perc% \n 💪 Tiros no braço direito: $Damage_RightArm_Perc% \n 🦵 Tiros na perna esquerda: $Damage_LeftLeg_Perc% \n 🦵 Tiros na perna direita: $Damage_RightLeg_Perc% \n"
        Content+="..."
        URL="https://discord.com/api/v10/channels/$DiscordChannelPlayersStatsChannelId/messages/1369177212502151252"
        response=$(curl -s -X PATCH \
        -H "Authorization: Bot $DiscordChannelPlayersStatsBotToken" \
        -H "Content-Type: application/json" \
        -d "{\"content\": \"$Content\"}" \
        "$URL")
        sleep 1
    elif [ $i -eq 9 ]; then
        Content="🏅 Top 9 - $player_info matou $TotalKills jogadores \n 🔫 Arma preferida: $PreferredWeapon \n 🎯 Tiro de maior distância: $metros metros ($WeaponLongestShot) \n 💥 Dano total causado: $TotalDamage \n 🤕 Tiros na cabeça: $Damage_Head_Perc% \n 🦺 Tiros no corpo: $Damage_Torso_Perc% \n 💪 Tiros no braço esquerdo: $Damage_LeftArm_Perc% \n 💪 Tiros no braço direito: $Damage_RightArm_Perc% \n 🦵 Tiros na perna esquerda: $Damage_LeftLeg_Perc% \n 🦵 Tiros na perna direita: $Damage_RightLeg_Perc% \n"
        Content+="..."
        URL="https://discord.com/api/v10/channels/$DiscordChannelPlayersStatsChannelId/messages/1369177214196646020"
        response=$(curl -s -X PATCH \
        -H "Authorization: Bot $DiscordChannelPlayersStatsBotToken" \
        -H "Content-Type: application/json" \
        -d "{\"content\": \"$Content\"}" \
        "$URL")
        sleep 1
    elif [ $i -eq 10 ]; then
        Content="🏅 Top 10 - $player_info matou $TotalKills jogadores \n 🔫 Arma preferida: $PreferredWeapon \n 🎯 Tiro de maior distância: $metros metros ($WeaponLongestShot) \n 💥 Dano total causado: $TotalDamage \n 🤕 Tiros na cabeça: $Damage_Head_Perc% \n 🦺 Tiros no corpo: $Damage_Torso_Perc% \n 💪 Tiros no braço esquerdo: $Damage_LeftArm_Perc% \n 💪 Tiros no braço direito: $Damage_RightArm_Perc% \n 🦵 Tiros na perna esquerda: $Damage_LeftLeg_Perc% \n 🦵 Tiros na perna direita: $Damage_RightLeg_Perc% \n"
        Content+="..."
        URL="https://discord.com/api/v10/channels/$DiscordChannelPlayersStatsChannelId/messages/1369177215740284990"
        response=$(curl -s -X PATCH \
        -H "Authorization: Bot $DiscordChannelPlayersStatsBotToken" \
        -H "Content-Type: application/json" \
        -d "{\"content\": \"$Content\"}" \
        "$URL")
        sleep 1
    fi
    i=$((i+1))    
done < <(sqlite3 -separator '|' "$PLAYERS_BECO_C1_DB" "
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
ORDER BY TotalKills DESC, TotalDamage DESC
LIMIT 10;
")

FirstDate=$(sqlite3 "$PLAYERS_BECO_C1_DB" "SELECT Data FROM players_killfeed ORDER BY Data ASC LIMIT 1;")
Content="\n Obs: Dados coletados a partir de $FirstDate"
Content+="...\n"

URL="https://discord.com/api/v10/channels/$DiscordChannelPlayersStatsChannelId/messages/1369177217199902774"
response=$(curl -s -X PATCH \
-H "Authorization: Bot $DiscordChannelPlayersStatsBotToken" \
-H "Content-Type: application/json" \
-d "{\"content\": \"$Content\"}" \
"$URL")