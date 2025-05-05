#include "$CurrentDir:mpmissions/dayzOffline.chernarusplus/admin/AdminLoadout.c"
#include "$CurrentDir:mpmissions/dayzOffline.chernarusplus/admin/VehicleSpawner.c"

void main()
{
	//INIT ECONOMY--------------------------------------
	Hive ce = CreateHive();
	if ( ce )
		ce.InitOffline();

	//DATE RESET AFTER ECONOMY INIT-------------------------
	int year, month, day, hour, minute;
	int reset_month = 9, reset_day = 20;
	GetGame().GetWorld().GetDate(year, month, day, hour, minute);

	if ((month == reset_month) && (day < reset_day))
	{
		GetGame().GetWorld().SetDate(year, reset_month, reset_day, hour, minute);
	}
	else
	{
		if ((month == reset_month + 1) && (day > reset_day))
		{
			GetGame().GetWorld().SetDate(year, reset_month, reset_day, hour, minute);
		}
		else
		{
			if ((month < reset_month) || (month > reset_month + 1))
			{
				GetGame().GetWorld().SetDate(year, reset_month, reset_day, hour, minute);
			}
		}
	}
}

class SafeZoneData {
	string customMessage;
	string regionStr;
    vector areaMin;
    vector areaMax;
    ref array<vector> safeZones;

    void SafeZoneData() {
        safeZones = new array<vector>();
    }
}

class CustomMission: MissionServer
{
	float m_AdminCheckCooldown30 = 30.0;
	float m_AdminCheckTimer30 = 0.0;
	float m_AdminCheckCooldown60 = 60.0;
	float m_AdminCheckTimer60 = 0.0;
	string FixedMessage1 = "Você pode criar qualquer item pelo chat, por exemplo: /admin giveitem M67Grenade";
	string FixedMessage2 = "O comando pode demorar até 30 segundos para ser executado";

	string regionStr;
	string customMessage;
	vector areaMin;
    vector areaMax;
    ref array<vector> safeZones;	

	void CustomMission()
	{
		WriteToLog("Entrou no construtor CustomMission");
		ref SafeZoneData szData = LoadActiveRegionData("$mission:deathmatch_config.json");
		if (szData)
		{
			customMessage = szData.customMessage;
			regionStr = szData.regionStr;
			areaMin = szData.areaMin;
			areaMax = szData.areaMax;
			safeZones = szData.safeZones;
			WriteToLog("Carregou region: " + regionStr);
		}
		else
		{
			WriteToLog("Erro ao carregar dados da zona segura.");
		}
	}

	override void OnUpdate(float timeslice)
	{
		super.OnUpdate(timeslice);
		// A cada 30 segundos
		m_AdminCheckTimer30 += timeslice;
		if (m_AdminCheckTimer30 >= m_AdminCheckCooldown30)
		{
			m_AdminCheckTimer30 = 0.0;
			CheckAdminCommands();

			array<string> msgs = CheckMessages();
			
			// Checar todos os jogadores
			array<Man> players = new array<Man>;
			GetGame().GetPlayers(players);
			foreach (Man man : players)
			{
				PlayerBase player = PlayerBase.Cast(man);
				if (player)
				{
					CheckPlayerArea(player);	

					// Envia mensagens
					foreach (string msg : msgs)
					{
						player.MessageImportant(msg);
					}
				}
			}
		}
		// Cada 1 min
		m_AdminCheckTimer60 += timeslice;
		if (m_AdminCheckTimer60 >= m_AdminCheckCooldown60)
		{
			
			if (m_AdminCheckTimer60 >= m_AdminCheckCooldown60)
			{
				AppendMessage(customMessage);
				AppendMessage(FixedMessage1);
				AppendMessage(FixedMessage2);				
			}			

			m_AdminCheckTimer60 = 0.0;
		}
	}

	ref SafeZoneData LoadActiveRegionData(string path)
	{
		FileHandle file = OpenFile(path, FileMode.READ);
		if (!file) {
			WriteToLog("Arquivo não encontrado: " + path);
			return null;
		}

		string content, line;
		while (FGets(file, line) > 0) {
			content += line;
		}
		CloseFile(file);

		int startObj = content.IndexOf("{");
		while (startObj != -1)
		{
			// Procurar fechamento do objeto
			string rest = content.Substring(startObj, content.Length() - startObj);
			int relEnd = rest.IndexOf("}");
			if (relEnd == -1) break;
			int endObj = startObj + relEnd;

			string objStr = content.Substring(startObj, endObj - startObj + 1);

			int idxActive = objStr.IndexOf("\"Active\":");
			if (idxActive != -1) {
				string boolStr = objStr.Substring(idxActive + 9, 5);
				boolStr.ToLower();
				if (boolStr.Contains("true")) {
					auto data = new SafeZoneData();

					// Region
					int idxRegion = objStr.IndexOf("\"Region\":");
					if (idxRegion != -1) {
						string subRegion = objStr.Substring(idxRegion + 9, objStr.Length() - idxRegion - 9);
						int sRelRegion = subRegion.IndexOf("\"") + 1;
						string subRegion2 = subRegion.Substring(sRelRegion, subRegion.Length() - sRelRegion);
						int eRelRegion = subRegion2.IndexOf("\"");
						int sRegion = idxRegion + 9 + sRelRegion;
						int eRegion = sRegion + eRelRegion;
						data.regionStr = objStr.Substring(sRegion, eRegion - sRegion);
					}

					// CustomMessage
					int idxCustomMessage = objStr.IndexOf("\"CustomMessage\":");
					if (idxCustomMessage != -1) {
						string subCustomMessage = objStr.Substring(idxCustomMessage + 16, objStr.Length() - idxCustomMessage - 16);
						int sRelCustomMessage = subCustomMessage.IndexOf("\"") + 1;
						string subCustomMessage2 = subCustomMessage.Substring(sRelCustomMessage, subCustomMessage.Length() - sRelCustomMessage);
						int eRelCustomMessage = subCustomMessage2.IndexOf("\"");
						int sCustomMessage = idxCustomMessage + 16 + sRelCustomMessage;
						int eCustomMessage = sCustomMessage + eRelCustomMessage;
						data.customMessage = objStr.Substring(sCustomMessage, eCustomMessage - sCustomMessage);
					}

					// AreaMin
					int idxMin = objStr.IndexOf("\"AreaMin\":");
					if (idxMin != -1) {
						string subMin = objStr.Substring(idxMin + 10, objStr.Length() - idxMin - 10);
						int sRel = subMin.IndexOf("\"") + 1;
						string subMin2 = subMin.Substring(sRel, subMin.Length() - sRel);
						int eRel = subMin2.IndexOf("\"");
						int s = idxMin + 10 + sRel;
						int e = s + eRel;
						string minStr = objStr.Substring(s, e - s);
						data.areaMin = minStr.ToVector();
					}

					// AreaMax
					int idxMax = objStr.IndexOf("\"AreaMax\":");
					if (idxMax != -1) {
						string subMax = objStr.Substring(idxMax + 10, objStr.Length() - idxMax - 10);
						int s2Rel = subMax.IndexOf("\"") + 1;
						string subMax2 = subMax.Substring(s2Rel, subMax.Length() - s2Rel);
						int e2Rel = subMax2.IndexOf("\"");
						int s2 = idxMax + 10 + s2Rel;
						int e2 = s2 + e2Rel;
						string maxStr = objStr.Substring(s2, e2 - s2);
						data.areaMax = maxStr.ToVector();
					}

					// SafeZones
					int idxSafe = objStr.IndexOf("\"SafeZones\":");
					if (idxSafe != -1) {
						string subSafe = objStr.Substring(idxSafe, objStr.Length() - idxSafe);
						int s3Rel = subSafe.IndexOf("[");
						int e3Rel = subSafe.IndexOf("]");
						int s3 = idxSafe + s3Rel;
						int e3 = idxSafe + e3Rel;
						string safeBlock = objStr.Substring(s3 + 1, e3 - s3 - 1);

						array<string> entries = new array<string>();
						safeBlock.Split(",", entries);

						for (int i = 0; i + 2 < entries.Count(); i += 3) {
							string vecStr = entries[i] + "," + entries[i + 1] + "," + entries[i + 2];
							vecStr.Replace("\"", "");
							vecStr.Trim();
							// Criar array para armazenar os valores separados
							TStringArray parts = new TStringArray();
							vecStr.Split(",", parts);

							// Verificar se há exatamente 3 partes
							if (parts.Count() == 3) {
								float x = parts[0].Trim().ToFloat();
								float y = parts[1].Trim().ToFloat();
								float z = parts[2].Trim().ToFloat();
								data.safeZones.Insert(Vector(x, y, z));
							}
							
						}
					}

					return data;
				}
			}

			// Próximo objeto
			string rem = content.Substring(endObj + 1, content.Length() - endObj - 1);
			int relNext = rem.IndexOf("{");
			if (relNext != -1) {
				startObj = endObj + 1 + relNext;
			} else {
				startObj = -1;
			}
		}


		return null;
	}

	vector GetRandomSafeSpawnPosition()
	{
		int index = Math.RandomInt(0, safeZones.Count());
		return safeZones[index]; // Retorna a coordenada aleatória
	}

	void CheckPlayerArea(PlayerBase player)
	{
		vector pos = player.GetPosition();
		
		// Checa se o jogador está fora da área permitida
		if (pos[0] < areaMin[0] || pos[0] > areaMax[0] || pos[2] < areaMin[2] || pos[2] > areaMax[2])
		{
			// Aplica dano de corte (faz o jogador começar a sangrar)
            string ammoType = "MeleeSlash";
            player.ProcessDirectDamage(DT_CUSTOM, player, "", ammoType, "0 0 0", 5.0);

            player.MessageImportant("VOCÊ SAIU DA ZONA SEGURA, VOLTE IMEDIATAMENTE POIS SUA VIDA IRÁ REDUZIR!");
		} 
	}

	array<string> CheckMessages()
	{
		array<string> msgs = new array<string>();

		string path = "$mission:messages_to_send.txt";
        FileHandle file = OpenFile(path, FileMode.READ);
        if (file == 0) {
			return msgs;
		}

		string line;
		
        while (FGets(file, line) > 0)
        {
            line = line.Trim();
            if (line != "") {				
				msgs.Insert(line);
			}
		}		

		CloseFile(file);
		FileHandle clearFile = OpenFile(path, FileMode.WRITE);
		if (clearFile != 0)
			CloseFile(clearFile); // abrir em modo WRITE já limpa o conteúdo
		
		return msgs;
	}

	void AppendMessage(string message)
	{
		string path = "$mission:messages_to_send.txt";
		FileHandle file = OpenFile(path, FileMode.APPEND);

		if (file != 0)
		{
			FPrintln(file, message);
			CloseFile(file);
		}
		else
		{
			WriteToLog("Erro ao abrir o arquivo para append: " + path);
		}
	}

	PlayerBase GetPlayerByID(string id)
	{
		array<Man> players = {};
		GetGame().GetPlayers(players);

		foreach (Man man : players)
		{
			PlayerBase player = PlayerBase.Cast(man);
			if (player && player.GetIdentity() && player.GetIdentity().GetId() == id)
				return player;
		}
		return null;
	}
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
					target.DisableSimulation(true);
					target.MessageStatus("🕵️ Você está invisível e com simulacao desativada");
					break;
				case "unghostmode":
					target.SetInvisible(false);
					target.DisableSimulation(false);
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
			}
		}

        CloseFile(file);
		FileHandle clearFile = OpenFile(path, FileMode.WRITE);
		if (clearFile != 0)
			CloseFile(clearFile); // abrir em modo WRITE já limpa o conteúdo
	}

	void SetRandomHealth(EntityAI itemEnt)
	{
		if ( itemEnt )
		{
			float rndHlt = Math.RandomFloat( 0.45, 0.65 );
			itemEnt.SetHealth01( "", "", rndHlt );
		}
	}
	array<string> LoadAdminIDs(string filePath)
	{
        	array<string> ids = new array<string>;
	        FileHandle file = OpenFile(filePath, FileMode.READ);
        	if (file != 0)
	        {
        	    string line;
	            while (FGets(file, line) > 0)
        	    {
                	line = line.Trim();
	                if (line != "") ids.Insert(line);
        	    }
	            CloseFile(file);
        	}
	        return ids;
	}

	override PlayerBase CreateCharacter(PlayerIdentity identity, vector pos, ParamsReadContext ctx, string characterName)
	{
		Entity playerEnt;
		playerEnt = GetGame().CreatePlayer( identity, characterName, pos, 0, "NONE" );
		Class.CastTo( m_player, playerEnt );

		GetGame().SelectPlayer( identity, m_player );

		array<string> adminIDs = LoadAdminIDs("$mission:admin_ids.txt");
		if (adminIDs.Find(identity.GetId()) != -1 && identity.GetName() == "Admin")
		{
			m_player.SetAllowDamage(false);
			GiveAdminLoadout(m_player);
		}
		m_player.SetAllowDamage(false);

		GiveSurvivorLoadout(m_player);
		m_player.SetHealth("", "", 100);
		m_player.SetHealth("GlobalHealth", "Blood", 5000);
		m_player.SetHealth("GlobalHealth", "Shock", 0);
		m_player.GetStatEnergy().Set(4000);
		m_player.GetStatWater().Set(4000);

		// Obtenha uma posição aleatória da zona segura
		vector safePosition = GetRandomSafeSpawnPosition();

		// Define a posição do jogador para a coordenada da zona segura
		m_player.SetPosition(safePosition);

		m_player.SetAllowDamage(true);

		return m_player;
	}

	void WriteToLog(string content, string logfile = "init.log")
	{
		string fileName = "$profile:" + logfile; // Caminho dentro da pasta do servidor
		FileHandle file = OpenFile(fileName, FileMode.APPEND);

		if (file != 0)
		{
			FPrintln(file, content); // Escreve a string com quebra de linha
			CloseFile(file);
		}
		else
		{
			WriteToLog("Erro ao abrir o arquivo para escrita.");
		}
	}

	override void StartingEquipSetup(PlayerBase player, bool clothesChosen)
	{
		EntityAI itemClothing;
		EntityAI itemEnt;
		ItemBase itemBs;
		float rand;

		// itemClothing = player.FindAttachmentBySlotName( "Body" );
		// if ( itemClothing )
		// {
		// 	SetRandomHealth( itemClothing );
			
		// 	itemEnt = itemClothing.GetInventory().CreateInInventory( "BandageDressing" );
		// 	player.SetQuickBarEntityShortcut(itemEnt, 2);
			
		// 	string chemlightArray[] = { "Chemlight_White", "Chemlight_Yellow", "Chemlight_Green", "Chemlight_Red" };
		// 	int rndIndex = Math.RandomInt( 0, 4 );
		// 	itemEnt = itemClothing.GetInventory().CreateInInventory( chemlightArray[rndIndex] );
		// 	SetRandomHealth( itemEnt );
		// 	player.SetQuickBarEntityShortcut(itemEnt, 1);

		// 	rand = Math.RandomFloatInclusive( 0.0, 1.0 );
		// 	if ( rand < 0.35 )
		// 		itemEnt = player.GetInventory().CreateInInventory( "Apple" );
		// 	else if ( rand > 0.65 )
		// 		itemEnt = player.GetInventory().CreateInInventory( "Pear" );
		// 	else
		// 		itemEnt = player.GetInventory().CreateInInventory( "Plum" );
		// 	player.SetQuickBarEntityShortcut(itemEnt, 3);
		// 	SetRandomHealth( itemEnt );
		// }
		
		// itemClothing = player.FindAttachmentBySlotName( "Legs" );
		// if ( itemClothing )
		// 	SetRandomHealth( itemClothing );
		
		// itemClothing = player.FindAttachmentBySlotName( "Feet" );
	}
};

Mission CreateCustomMission(string path)
{
	return new CustomMission();
}