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

        // Buscar o jogador
        PlayerBase target = null;
        array<Man> players = {};
        GetGame().GetPlayers(players);

        foreach (Man man : players)
        {
            PlayerBase player = PlayerBase.Cast(man);
            if (player && player.GetIdentity() && player.GetIdentity().GetId() == playerID)
                    target = player;
        }
        if (!target) continue;

        // Executar o comando
        switch (command)
        {
            case "teleport":
                if (tokens.Count() == 5)
                {
                    vector pos = Vector(tokens[2].ToFloat(), tokens[3].ToFloat(), tokens[4].ToFloat());
                    target.SetPosition(pos);
                    target.MessageStatus("🚀 Você foi teleportado");
                }
                break;
            case "heal":
                target.SetHealth("", "", 100);
                target.SetHealth("GlobalHealth", "Blood", 5000);
                target.SetHealth("GlobalHealth", "Shock", 0);
                target.GetStatEnergy().Set(4000);
                target.GetStatWater().Set(4000);
                target.MessageStatus("❤️ Você foi curado");
                break;
            case "kill":
                target.SetHealth("", "", 0);
                target.MessageStatus("💀 Você foi eliminado");
                break;
            case "godmode":
                target.SetAllowDamage(false);
                target.MessageStatus("⚡ God Mode ativado");
                break;
            case "ungodmode":
                target.SetAllowDamage(true);
                target.MessageStatus("🔓 God Mode desativado");
                break;
            case "giveitem":
                if (tokens.Count() == 3)
                {
                    string itemName = tokens[2];
                    EntityAI item = target.GetInventory().CreateInInventory(itemName);
                    if (!item)
                    {
                        // Tenta criar no chão se falhar no inventário
                        item = EntityAI.Cast(GetGame().CreateObject(itemName, target.GetPosition(), false, true));
                    }
                    if (item)
                        target.MessageStatus("🎁 Item recebido: " + itemName);
                    else
                        target.MessageStatus("⚠️ Erro ao criar item: " + itemName);
                }
                break;
            case "spawnvehicle":
                if (tokens.Count() == 3)
                {
                    string vehicleType = tokens[2];
                    SpawnVehicleWithParts(target, vehicleType);
                }
                break;
            case "ghostmode":
                target.SetInvisible(true);
                //target.DisableSimulation(true);
                target.MessageStatus("🕵️ Você está invisível e com simulacao desativada");
                break;
            case "unghostmode":
                target.SetInvisible(false);
                //target.DisableSimulation(false);
                target.MessageStatus("👁️ Você está visível e com simulacao ativada");
                break;
            case "kick":
                target.MessageStatus("Seu jogador está bugado. Se for kickado tente conectar novamente. Realizando ajuste...");
                PlayerIdentity identity = target.GetIdentity();
                GetGame().DisconnectPlayer(identity);
                break;
            case "desbug":
                target.MessageStatus("Seu jogador está bugado. Movimentando jogador para outra posicao...");
                // Captura a posição atual
                vector currentPos = target.GetPosition();

                // Gera um deslocamento aleatório de até ±1 metro nas direções X, Y e Z
                float offsetX = Math.RandomFloatInclusive(-1.0, 1.0);
                float offsetY = Math.RandomFloatInclusive(-0.5, 0.5); // Y geralmente precisa de menos variação
                float offsetZ = Math.RandomFloatInclusive(-1.0, 1.0);

                // Aplica o deslocamento
                vector newPos = currentPos + Vector(offsetX, offsetY, offsetZ);

                // Atualiza a posição do jogador
                target.SetPosition(newPos);

                // Opcional: garante sincronização com cliente
                target.SetOrientation(target.GetOrientation());  // força atualização da posição no cliente			
                target.Update();
                target.MessageStatus("Jogador movido para nova posição: " + newPos.ToString());	
                break;
            case "getposition":
                target.MessageStatus("Posição atual: " + target.GetPosition().ToString());
                WriteToLog(target.GetPosition().ToString(), "position.log");
                break;
            case "getposition":
                target.MessageStatus("Posição atual: " + target.GetPosition().ToString());
                WriteToLog(target.GetPosition().ToString(), "position.log");
                break;
            case "construct":
                if (tokens.Count() >= 3)
                {
                    float heightOffset = 1.0;
                    int containerCount = 4;
                    float containerLength = 6.0;
                    float rotationOffset = 0.0;

                    if (tokens.Count() >= 4)
                        heightOffset = tokens[3].ToFloat(); // Conversão correta de string para float

                    if (tokens.Count() >= 5)
                        containerCount = tokens[4].ToInt(); // Conversão correta de string para int
                    
                    if (tokens.Count() >= 6)
                        containerLength = tokens[5].ToFloat();

                    if (tokens.Count() >= 7)
                        rotationOffset = tokens[6].ToFloat();

                    string buildName = tokens[2];
                    CreateCustomObject(target, buildName, heightOffset, containerCount, containerLength, rotationOffset);
                }
                break;
            case "construct_retangle":
                // if (tokens.Count() >= 5)
                // {	
                // 	string minAreaStr = tokens[3];
                // 	string maxAreaStr = tokens[4];

                // 	minAreaStr.Replace(";", " ");
                // 	maxAreaStr.Replace(";", " ");

                // 	vector minArea = minAreaStr.ToVector();
                // 	vector maxArea = maxAreaStr.ToVector();

                // 	// Agora sim, passa vetores reais para a função
                // 	CreateRectangleFromCoords(minArea, maxArea, "Land_Container_1Bo", 6.0, 1.0);
                // 	CreateRectangleFromCoords(minArea, maxArea, "Land_Container_1Bo", 6.0, 3.5);
                // }
                array<vector> points = new array<vector>;
                points.Insert("4187.81 0 10610.63".ToVector()); // inferior esquerdo
                points.Insert("4336.88 0 10631.25".ToVector()); // inferior direito
                points.Insert("4314.38 0 10800.94".ToVector()); // superior direito
                points.Insert("4130.63 0 10757.81".ToVector()); // superior esquerdo
                CreateLinePathFromPoints(points, "Land_Container_1Bo", 6.0, 1.0);
                CreateLinePathFromPoints(points, "Land_Container_1Bo", 6.0, 3.5);
                break;

        }
    }

    CloseFile(file);
    FileHandle clearFile = OpenFile(path, FileMode.WRITE);
    if (clearFile != 0)
        CloseFile(clearFile); // abrir em modo WRITE já limpa o conteúdo
}