void CheckCommands()
{
    string path = ExternalCommandsFile;
    FileHandle file = OpenFile(path, FileMode.READ);
    if (file == 0) return;

    string line;
    while (FGets(file, line) > 0)
    {
        line = line.Trim();
        if (line == "") continue;

        TStringArray tokens = new TStringArray;
        line.Split(" ", tokens);
        if (tokens.Count() < 2) 
            continue;

        ExecuteCommand(tokens);
    }

    CloseFile(file);
    FileHandle clearFile = OpenFile(path, FileMode.WRITE);
    if (clearFile != 0)
        CloseFile(clearFile);
}

bool ExecuteCommand(TStringArray tokens)
{
    string playerID = tokens[0];
    string command = tokens[1];
    
    PlayerBase target = null;
    array<Man> players = {};
    GetGame().GetPlayers(players);

    foreach (Man man : players)
    {
        PlayerBase player = PlayerBase.Cast(man);
        if (player && player.GetIdentity() && player.GetIdentity().GetId() == playerID)
        {
            target = player;
            break;
        }
    }

    if (!target || !target.IsAlive()) 
        return false;
    
    if (tokens.Count() >= 3)
    {
        string params = tokens[2];
        for (int iC = 0; iC < tokens.Count(); iC++) {
            if (iC < 4)
                continue;

            params = params + " " + tokens[iC];
        }
        string commandFull = command + " " + params;
        WriteToLog("PlayerID " + target.GetIdentity().GetName() + " (" + playerID + ")" + " digitou comando " + commandFull, LogFile.INIT, false, LogType.INFO);
    } else {
        WriteToLog("PlayerID " + target.GetIdentity().GetName() + " (" + playerID + ")" + " digitou comando " + command, LogFile.INIT, false, LogType.INFO);
    }
    bool isAdmin = CheckIfIsAdmin(playerID);

    switch (command)
    {
        case "help":
            SendPrivateMessage(playerID, "!loadouts -> Lista loadouts configurados", MessageColor.FRIENDLY);
            SendPrivateMessage(playerID, "!loadout meuloadout1' -> Ativa meuloadout1", MessageColor.FRIENDLY);
            SendPrivateMessage(playerID, "!loadout reset -> Gera nova senha aleatória para acessar o sistema de loadout: " + UrlAppPython, MessageColor.FRIENDLY);
            SendPrivateMessage(playerID, "!maps -> Lista mapas disponíveis", MessageColor.FRIENDLY);
            SendPrivateMessage(playerID, "!votemap 1 -> Vota no mapa 1", MessageColor.FRIENDLY);
            SendPrivateMessage(playerID, "!players -> Lista jogadores online", MessageColor.FRIENDLY);
            SendPrivateMessage(playerID, "!votekick 12345679 -> Vota para kickar o jogador de ID 12345679", MessageColor.FRIENDLY);
            SendPrivateMessage(playerID, "!kill -> Cometer suicídio", MessageColor.FRIENDLY);
            break;
        case "teleport":
            if (!isAdmin)
            {
                SendPrivateMessage(playerID, "Você não possui permissão para executar esse comando", MessageColor.IMPORTANT);
                WriteToLog("Comando foi bloqueado para o jogador!", LogFile.INIT, false, LogType.ERROR);
                return false;
            }
            if (tokens.Count() == 5)
            {
                vector posT = Vector(tokens[2].ToFloat(), tokens[3].ToFloat(), tokens[4].ToFloat());
                target.SetPosition(posT);
                target.MessageStatus("Você foi teleportado");
            }
            break;

        case "heal":
            if (!isAdmin)
            {
                SendPrivateMessage(playerID, "Você não possui permissão para executar esse comando", MessageColor.IMPORTANT);
                WriteToLog("Comando foi bloqueado para o jogador!", LogFile.INIT, false, LogType.ERROR);
                return false;
            }
            target.SetHealth("", "", 100);
            target.SetHealth("GlobalHealth", "Blood", 5000);
            target.SetHealth("GlobalHealth", "Shock", 0);
            target.GetStatEnergy().Set(4000);
            target.GetStatWater().Set(4000);
            target.MessageStatus("Você foi curado");
            break;

        case "kill":
            target.SetHealth("", "", 0);
            target.MessageStatus("Você foi eliminado");
            break;

        case "godmode":
            if (!isAdmin)
            {
                SendPrivateMessage(playerID, "Você não possui permissão para executar esse comando", MessageColor.IMPORTANT);
                WriteToLog("Comando foi bloqueado para o jogador!", LogFile.INIT, false, LogType.ERROR);
                return false;
            }
            target.SetAllowDamage(false);
            target.MessageStatus("God Mode ativado");
            break;

        case "ungodmode":
            if (!isAdmin)
            {
                SendPrivateMessage(playerID, "Você não possui permissão para executar esse comando", MessageColor.IMPORTANT);
                WriteToLog("Comando foi bloqueado para o jogador!", LogFile.INIT, false, LogType.ERROR);
                return false;
            }
            target.SetAllowDamage(true);
            target.MessageStatus("God Mode desativado");
            break;

        case "giveitem":  
            if (!isAdmin)
            {
                SendPrivateMessage(playerID, "Você não possui permissão para executar esse comando", MessageColor.IMPORTANT);
                WriteToLog("Comando foi bloqueado para o jogador!", LogFile.INIT, false, LogType.ERROR);
                return false;
            }          
            if (tokens.Count() >= 3)
            {
                string itemName = tokens[2];
                int limit = 1;
                if (tokens.Count() == 4)
                {
                    if (IsInteger(tokens[3]))
                        limit = tokens[3].ToInt();
                }
                for (int n = 1; n <= limit; n++)
                {
                    EntityAI item = target.GetInventory().CreateInInventory(itemName);
                    if (!item)
                        item = EntityAI.Cast(GetGame().CreateObject(itemName, target.GetPosition(), false, true));

                    if (item)
                    {
                        target.MessageStatus("Item recebido: " + itemName);
                    }
                    else
                    {
                        target.MessageStatus("Erro ao criar item: " + itemName);
                        WriteToLog("Erro ao criar item: " + itemName, LogFile.INIT, false, LogType.ERROR);
                    }
                }
            }
                
            break;

        case "spawnvehicle":
            if (!isAdmin)
            {
                SendPrivateMessage(playerID, "Você não possui permissão para executar esse comando", MessageColor.IMPORTANT);
                WriteToLog("Comando foi bloqueado para o jogador!", LogFile.INIT, false, LogType.ERROR);
                return false;
            }
            if (tokens.Count() == 3)
            {
                string vehicleType = tokens[2];
                SpawnVehicleWithPartsToPlayer(target, vehicleType);
            }
            break;

        case "ghostmode":
            if (!isAdmin)
            {
                SendPrivateMessage(playerID, "Você não possui permissão para executar esse comando", MessageColor.IMPORTANT);
                WriteToLog("Comando foi bloqueado para o jogador!", LogFile.INIT, false, LogType.ERROR);
                return false;
            }
            target.SetInvisible(true);
            target.SetScale(0.0001);
            target.MessageStatus("Você está invisível");
            break;

        case "unghostmode":
            if (!isAdmin)
            {
                SendPrivateMessage(playerID, "Você não possui permissão para executar esse comando", MessageColor.IMPORTANT);
                WriteToLog("Comando foi bloqueado para o jogador!", LogFile.INIT, false, LogType.ERROR);
                return false;
            }
            target.SetInvisible(false);
            target.MessageStatus("Você está visível");
            break;

        case "kick":            
            PlayerIdentity identity = target.GetIdentity();
            target.MessageStatus("Seu jogador está bugado. Realizando ajuste...");
            GetGame().DisconnectPlayer(identity);
            break;

        case "desbug":
            vector currentPos = target.GetPosition();
            float offsetX = Math.RandomFloatInclusive(-1.0, 1.0);
            float offsetY = Math.RandomFloatInclusive(-0.5, 0.5);
            float offsetZ = Math.RandomFloatInclusive(-1.0, 1.0);
            vector newPos = currentPos + Vector(offsetX, offsetY, offsetZ);
            target.SetPosition(newPos);
            target.SetOrientation(target.GetOrientation());
            target.Update();
            target.MessageStatus("Posição ajustada: " + newPos.ToString());
            break;

        case "getposition":
            vector posP = target.GetPosition();
            target.MessageStatus("Posição atual: " + posP.ToString());
            WriteToLog(posP.ToString(), LogFile.POSITION, false);
            WriteToLog("Posição capturada: " + posP.ToString(), LogFile.INIT, false, LogType.DEBUG);
            break;

        case "construct":
            if (!isAdmin)
            {
                SendPrivateMessage(playerID, "Você não possui permissão para executar esse comando", MessageColor.IMPORTANT);
                WriteToLog("Comando foi bloqueado para o jogador!", LogFile.INIT, false, LogType.ERROR);
                return false;
            }
            if (tokens.Count() >= 3)
            {
                float heightOffset = 1.0;
                int containerCount = 4;
                float containerLength = 6.0;
                float rotationOffset = 0.0;

                if (tokens.Count() >= 4)
                    heightOffset = tokens[3].ToFloat();

                if (tokens.Count() >= 5)
                    containerCount = tokens[4].ToInt();
                
                if (tokens.Count() >= 6)
                    containerLength = tokens[5].ToFloat();

                if (tokens.Count() >= 7)
                    rotationOffset = tokens[6].ToFloat();

                string buildName = tokens[2];
                CreateCustomObject(target, buildName, heightOffset, containerCount, containerLength, rotationOffset);
            }
            break;        
        case "votemap":
            if (tokens.Count() < 3) {
                g_VoteMapManager.CheckVotingStatus(playerID);                 
                return false;
            }

            if (!IsInteger(tokens[2])) {
                SendPrivateMessage(playerID, "ID do mapa é inválido", MessageColor.WARNING);
                return false;
            }

            int regionId = tokens[2].ToInt();
            g_VoteMapManager.CheckIfVotingAndStart(playerID, regionId);
            break;   
        case "nextmap":  
            SendPrivateMessage(playerID, "O próximo mapa será " + nextMap.Region, MessageColor.FRIENDLY);
            break;
        case "maps":          
            foreach (ref SafeZoneData mapL : maps) {
                string linha = mapL.RegionId.ToString() + " - " + mapL.Region;                
                SendPrivateMessage(playerID, linha, MessageColor.FRIENDLY);
            }
            break;
        case "votekick":
            if (tokens.Count() < 3) {
                g_VoteKickManager.ListarJogadoresOnline(playerID);
                SendPrivateMessage(playerID, "Uso: !votekick <ID do jogador>", MessageColor.WARNING);
                return false;
            }

            if (!IsInteger(tokens[2])) {
                SendPrivateMessage(playerID, "ID inválido.", MessageColor.WARNING);
                return false;
            }

            string targetId = tokens[2];
            PlayerBase targetKick = null;
            foreach (Man manKick : players)
            {
                PlayerBase playerKick = PlayerBase.Cast(manKick);
                if (playerKick && playerKick.GetIdentity() && playerKick.GetIdentity().GetId() == targetId)
                {
                    targetKick = playerKick;
                    break;
                }
            }
            g_VoteKickManager.StartKickVote(playerID, targetId, targetKick.GetIdentity().GetName());
            break;
        case "players":          
            g_VoteKickManager.ListarJogadoresOnline(playerID);
            break;
        case "loadouts":
            ShowLoadoutsToPlayer(playerID);
            break;
        case "loadout":
            if (tokens.Count() < 3) {
                ShowLoadoutsToPlayer(playerID);
                return true;
            }
            if (tokens[2] == "reset")
            {
                WriteToLog("PlayerID " + target.GetIdentity().GetName() + " (" + playerID + ")" + " solicitou reset de senha", LogFile.INIT, false, LogType.INFO);
                SendPrivateMessage(playerID, "Você solicitou a geração de uma nova de senha de acesso! Aguarde um momento..." , MessageColor.WARNING);
                AppendExternalAction("{\"action\":\"reset_password\",\"player_id\":\"" + playerID + "\"}");
                return true;
            }

            string loadoutName = tokens[2];
            LoadoutPlayer loadout = GetLoadoutByName(playerID, loadoutName);
            if (!loadout)
            {
                SendPrivateMessage(playerID, "Nenhum loadout encontrado com esse nome" , MessageColor.WARNING);
                return false;
            }

            WriteToLog("PlayerID " + target.GetIdentity().GetName() + " (" + playerID + ")" + " solicitou ativacao do loadout " + loadoutName, LogFile.INIT, false, LogType.INFO);
            SendPrivateMessage(playerID, "Você solicitou a ativação de um lodout! Aguarde um momento..." , MessageColor.WARNING);
            ActiveLoadoutByName(playerID, loadoutName);
            AppendExternalAction("{\"action\":\"active_loadout\",\"player_id\":\"" + playerID + "\",\"loadout_name\":\"" + loadoutName + "\"}");

            break;
        case "settime":
            if (!isAdmin)
            {
                SendPrivateMessage(playerID, "Você não possui permissão para executar esse comando", MessageColor.IMPORTANT);
                WriteToLog("Comando bloqueado para não-admin: settime", LogFile.INIT, false, LogType.ERROR);
                return false;
            }

            if (tokens.Count() < 4 || !IsInteger(tokens[2]) || !IsInteger(tokens[3]))
            {
                SendPrivateMessage(playerID, "Uso: !settime <hora> <minuto> (ex: !settime 6 30)", MessageColor.WARNING);
                return false;
            }

            int newHour = tokens[2].ToInt();
            int newMinute = tokens[3].ToInt();

            if (newHour < 0 || newHour > 23 || newMinute < 0 || newMinute > 59)
            {
                SendPrivateMessage(playerID, "Hora ou minuto inválido. Use valores entre 0-23 e 0-59.", MessageColor.WARNING);
                return false;
            }

            int year, month, day, hour, minute;
            GetGame().GetWorld().GetDate(year, month, day, hour, minute);
            GetGame().GetWorld().SetDate(year, month, day, newHour, newMinute);

            string hourStr = "";
            if (newHour < 10)
                hourStr = "0";
            string minuteStr = "";
            if (newMinute < 10)
                minuteStr = "0";
            string horaFormatada = hourStr + newHour.ToString() + ":" + minuteStr + newMinute.ToString();

            SendPrivateMessage(playerID, "Horário do mundo ajustado para " + horaFormatada, MessageColor.FRIENDLY);
            WriteToLog("Admin " + playerID + " definiu horário do mundo para " + horaFormatada, LogFile.INIT, false, LogType.INFO);
            break;
        case "setweather":
            if (!isAdmin)
            {
                SendPrivateMessage(playerID, "Você não possui permissão para executar esse comando", MessageColor.IMPORTANT);
                return false;
            }

            if (tokens.Count() < 3)
            {
                SendPrivateMessage(playerID, "Uso: !setweather <clear | cloudy | rain | foggy | default>", MessageColor.WARNING);
                return false;
            }

            string clima = tokens[2];
            clima.ToLower();

            Weather weather = g_Game.GetWeather();

            if (clima == "clear")
            {
                // Remove todas as variações automáticas
                weather.GetRain().SetForecastChangeLimits(0, 0);
                weather.GetRain().SetForecastTimeLimits(0, 0);
                weather.GetRain().Set(0);

                weather.GetOvercast().SetForecastChangeLimits(0.01, 0.01);
                weather.GetOvercast().SetForecastTimeLimits(0, 0);
                weather.GetOvercast().Set(0.01);

                weather.GetFog().SetForecastChangeLimits(0, 0);
                weather.GetFog().SetForecastTimeLimits(0, 0);
                weather.GetFog().Set(0);

                weather.SetWindMaximumSpeed(0);
            }
            else if (clima == "cloudy")
            {
                weather.GetRain().SetForecastChangeLimits(0, 0);
                weather.GetRain().SetForecastTimeLimits(0, 0);
                weather.GetRain().Set(0);

                weather.GetOvercast().SetForecastChangeLimits(0.5, 0.5);
                weather.GetOvercast().SetForecastTimeLimits(0, 0);
                weather.GetOvercast().Set(0.5);

                weather.GetFog().SetForecastChangeLimits(0.1, 0.1);
                weather.GetFog().SetForecastTimeLimits(0, 0);
                weather.GetFog().Set(0.1);

                weather.SetWindMaximumSpeed(5);
            }
            else if (clima == "rain")
            {
                weather.GetRain().SetForecastChangeLimits(1.0, 1.0);
                weather.GetRain().SetForecastTimeLimits(0, 0);
                weather.GetRain().Set(1.0);

                weather.GetOvercast().SetForecastChangeLimits(1.0, 1.0);
                weather.GetOvercast().SetForecastTimeLimits(0, 0);
                weather.GetOvercast().Set(1.0);

                weather.GetFog().SetForecastChangeLimits(0.3, 0.3);
                weather.GetFog().SetForecastTimeLimits(0, 0);
                weather.GetFog().Set(0.3);

                weather.SetWindMaximumSpeed(20);
            }
            else if (clima == "foggy")
            {
                weather.GetRain().SetForecastChangeLimits(0, 0);
                weather.GetRain().SetForecastTimeLimits(0, 0);
                weather.GetRain().Set(0);

                weather.GetOvercast().SetForecastChangeLimits(0.3, 0.3);
                weather.GetOvercast().SetForecastTimeLimits(0, 0);
                weather.GetOvercast().Set(0.3);

                weather.GetFog().SetForecastChangeLimits(0.7, 0.7);
                weather.GetFog().SetForecastTimeLimits(0, 0);
                weather.GetFog().Set(0.7);

                weather.SetWindMaximumSpeed(5);
            }
            else if (clima == "default")
            {
                weather.GetRain().SetForecastChangeLimits(0.0, 1.0);
                weather.GetOvercast().SetForecastChangeLimits(0.0, 1.0);
                weather.GetFog().SetForecastChangeLimits(0.0, 0.3);

                weather.GetRain().SetForecastTimeLimits(1800, 3600);
                weather.GetOvercast().SetForecastTimeLimits(1800, 3600);
                weather.GetFog().SetForecastTimeLimits(1800, 3600);

                weather.SetWindMaximumSpeed(25);
            }
            else
            {
                SendPrivateMessage(playerID, "Clima desconhecido. Use: clear, cloudy, rain, foggy, default", MessageColor.WARNING);
                return false;
            }

            // Log e feedback
            float rain = weather.GetRain().GetActual();
            float overcast = weather.GetOvercast().GetActual();
            float fog = weather.GetFog().GetActual();
            float wind = weather.GetWindSpeed();

            string info = "Clima atual -> Rain: " + rain.ToString() + " | Overcast: " + overcast.ToString() + " | Fog: " + fog.ToString() + " | Wind: " + wind.ToString();
            SendPrivateMessage(playerID, "Clima ajustado para: " + clima, MessageColor.FRIENDLY);
            SendPrivateMessage(playerID, info, MessageColor.FRIENDLY);
            WriteToLog("Admin " + playerID + " ajustou o clima para " + clima + ". " + info, LogFile.INIT, false, LogType.INFO);
            break;



    }

    return true;
}

bool IsInteger(string s)
{
    if (s.Length() == 0)
        return false;

    for (int i = 0; i < s.Length(); i++)
    {
        string ch = s.Get(i);
        if (ch < "0" || ch > "9")
            return false;
    }

    return true;
}