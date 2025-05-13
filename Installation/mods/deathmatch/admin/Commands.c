// Forward declaration para evitar erro de função indefinida
void CreateRectangleFromCoords(vector minArea, vector maxArea, string objectType, float spacing, float heightOffset);

void CheckAdminCommands()
{
    string path = "$mission:admin_cmds.txt";
    FileHandle file = OpenFile(path, FileMode.READ);
    if (file == 0) return;

    string line;
    while (FGets(file, line) > 0)
    {
        line = line.Trim();
        if (line == "") continue;

        TStringArray tokens = new TStringArray;
        line.Split(" ", tokens);
        if (tokens.Count() < 2) continue;

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

        if (!target || !target.IsAlive()) continue;

        switch (command)
        {
            case "teleport":
                if (tokens.Count() == 5)
                {
                    vector posT = Vector(tokens[2].ToFloat(), tokens[3].ToFloat(), tokens[4].ToFloat());
                    target.SetPosition(posT);
                    target.MessageStatus("🚀 Você foi teleportado");
                    WriteToLog("teleport " + posT.ToString());
                }
                break;

            case "heal":
                target.SetHealth("", "", 100);
                target.SetHealth("GlobalHealth", "Blood", 5000);
                target.SetHealth("GlobalHealth", "Shock", 0);
                target.GetStatEnergy().Set(4000);
                target.GetStatWater().Set(4000);
                target.MessageStatus("❤️ Você foi curado");
                WriteToLog("heal");
                break;

            case "kill":
                target.SetHealth("", "", 0);
                target.MessageStatus("💀 Você foi eliminado");
                WriteToLog("kill");
                break;

            case "godmode":
                target.SetAllowDamage(false);
                target.MessageStatus("⚡ God Mode ativado");
                WriteToLog("godmode");
                break;

            case "ungodmode":
                target.SetAllowDamage(true);
                target.MessageStatus("🔓 God Mode desativado");
                WriteToLog("ungodmode");
                break;

            case "giveitem":
                if (tokens.Count() == 3)
                {
                    string itemName = tokens[2];
                    EntityAI item = target.GetInventory().CreateInInventory(itemName);
                    if (!item)
                        item = EntityAI.Cast(GetGame().CreateObject(itemName, target.GetPosition(), false, true));

                    if (item)
                    {
                        target.MessageStatus("🎁 Item recebido: " + itemName);
                        WriteToLog("giveitem");
                    }
                    else
                    {
                        target.MessageStatus("⚠️ Erro ao criar item: " + itemName);
                        WriteToLog("giveitem_failed");
                    }
                }
                break;

            case "spawnvehicle":
                if (tokens.Count() == 3)
                {
                    string vehicleType = tokens[2];
                    SpawnVehicleWithParts(target, vehicleType);
                    WriteToLog("spawnvehicle");
                }
                break;

            case "ghostmode":
                target.SetInvisible(true);
                target.MessageStatus("🕵️ Você está invisível");
                WriteToLog("ghostmode");
                break;

            case "unghostmode":
                target.SetInvisible(false);
                target.MessageStatus("👁️ Você está visível");
                WriteToLog("unghostmode");
                break;

            case "kick":
                PlayerIdentity identity = target.GetIdentity();
                target.MessageStatus("Seu jogador está bugado. Realizando ajuste...");
                GetGame().DisconnectPlayer(identity);
                WriteToLog("kick");
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
                WriteToLog("desbug", newPos.ToString());
                break;

            case "getposition":
                vector posP = target.GetPosition();
                target.MessageStatus("Posição atual: " + posP.ToString());
                WriteToLog(posP.ToString(), "position.log");
                WriteToLog("getposition", posP.ToString());
                break;

            case "construct":
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
                    WriteToLog("construct", buildName);
                }
                break;

            case "construct_retangle":
                if (tokens.Count() >= 5)
                {	
                    string minAreaStr = tokens[3];
                    string maxAreaStr = tokens[4];

                    minAreaStr.Replace(";", " ");
                    maxAreaStr.Replace(";", " ");

                    vector minArea = minAreaStr.ToVector();
                    vector maxArea = maxAreaStr.ToVector();

                    CreateRectangleFromCoords(minArea, maxArea, "Land_Container_1Bo", 6.0, 1.0);
                    CreateRectangleFromCoords(minArea, maxArea, "Land_Container_1Bo", 6.0, 3.5);
                    WriteToLog("construct_retangle", minArea.ToString() + " -> " + maxArea.ToString());
                }
                else
                {
                    // Fallback para retângulo fixo se não passar coordenadas
                    array<vector> points = new array<vector>;
                    points.Insert("4187.81 0 10610.63".ToVector());
                    points.Insert("4336.88 0 10631.25".ToVector());
                    points.Insert("4314.38 0 10800.94".ToVector());
                    points.Insert("4130.63 0 10757.81".ToVector());

                    CreateLinePathFromPoints(points, "Land_Container_1Bo", 6.0, 1.0);
                    CreateLinePathFromPoints(points, "Land_Container_1Bo", 6.0, 3.5);
                    WriteToLog("construct_retangle", "usou pontos fixos");
                }
                break;
        }
    }

    CloseFile(file);
    FileHandle clearFile = OpenFile(path, FileMode.WRITE);
    if (clearFile != 0)
        CloseFile(clearFile);
}
