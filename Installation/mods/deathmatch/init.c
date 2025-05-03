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


class CustomMission: MissionServer
{
	float m_AdminCheckCooldown = 30.0;
	float m_AdminCheckTimer = 0.0;
	// Coordenadas do canto inferior esquerdo e superior direito do retângulo
	vector areaMin = "2350 0 1000"; // inferior esquerdo
	vector areaMax = "3000 0 1450"; // superior direito

	

	override void OnUpdate(float timeslice)
	{
		super.OnUpdate(timeslice);
		m_AdminCheckTimer += timeslice;

		if (m_AdminCheckTimer >= m_AdminCheckCooldown)
		{
			m_AdminCheckTimer = 0.0;
			CheckAdminCommands();
			
			// Checar todos os jogadores
			array<Man> players = new array<Man>;
			GetGame().GetPlayers(players);
			foreach (Man man : players)
			{
				PlayerBase player = PlayerBase.Cast(man);
				if (player)
				{
					CheckPlayerArea(player);

					player.MessageStatus("Para criar um item use: /admin giveitem <item_name>");
				}
			}
		}
	}

	vector GetRandomSafeSpawnPosition()
	{
		ref array<vector> safeZones = new array<vector>;
		// Zonas ao redor da prisão
		safeZones.Insert(Vector(2672.192383, 3.129421, 1374.600342));
		safeZones.Insert(Vector(2651.712646, 1.712951, 1395.944336));
		safeZones.Insert(Vector(2608.701416, 2.664377, 1389.977295));
		safeZones.Insert(Vector(2587.730713, 1.549090, 1411.275513));
		safeZones.Insert(Vector(2514.658203, 3.143692, 1437.058716));
		safeZones.Insert(Vector(2428.497070, 2.656586, 1389.492920));
		safeZones.Insert(Vector(2441.603516, 4.024718, 1367.078125));
		safeZones.Insert(Vector(2492.065918, 4.098767, 1341.991577));
		safeZones.Insert(Vector(2516.317627, 7.819347, 1311.331055));
		safeZones.Insert(Vector(2536.916992, 9.694158, 1265.117065));
		safeZones.Insert(Vector(2486.790039, 4.087596, 1217.682617));
		safeZones.Insert(Vector(2538.850098, 3.324883, 1217.530640));
		safeZones.Insert(Vector(2584.148438, 6.621020, 1254.476318));
		safeZones.Insert(Vector(2640.988770, 1.986329, 1231.470215));
		safeZones.Insert(Vector(2666.184570, 4.790472, 1290.482300));
		safeZones.Insert(Vector(2697.927734, 4.449666, 1280.248901));
		safeZones.Insert(Vector(2713.756836, 2.746556, 1230.137573));
		safeZones.Insert(Vector(2724.076904, 5.087080, 1168.243042));
		safeZones.Insert(Vector(2760.935791, 1.811343, 1153.496460));
		safeZones.Insert(Vector(2805.038086, 3.297328, 1134.619019));
		safeZones.Insert(Vector(2898.120361, 4.845843, 1177.733643));
		safeZones.Insert(Vector(2972.281982, 1.116114, 1146.199707));
		safeZones.Insert(Vector(2866.358887, 1.210863, 1215.002686));
		safeZones.Insert(Vector(2830.860352, 2.688663, 1264.097778));
		safeZones.Insert(Vector(2799.674072, 7.554385, 1313.999878));
		safeZones.Insert(Vector(2752.633301, 1.322555, 1342.637207));
		safeZones.Insert(Vector(2717.519775, 2.653562, 1344.919312));

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

            player.MessageStatus("Você saiu da zona segura e começou a sangrar!");
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
					WriteToLog(target.GetPosition().ToString());
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
			m_player.MessageStatus("⚡ God Mode Ativado (Admin)");
			GiveAdminLoadout(m_player);
		}
		m_player.SetAllowDamage(false);

		GiveSurvivorLoadout(m_player);
		m_player.SetHealth("", "", 100);
		m_player.SetHealth("GlobalHealth", "Blood", 5000);
		m_player.SetHealth("GlobalHealth", "Shock", 0);
		m_player.GetStatEnergy().Set(4000);
		m_player.GetStatWater().Set(4000);
		m_player.MessageStatus("Você foi curado");

		// Obtenha uma posição aleatória da zona segura
		vector safePosition = GetRandomSafeSpawnPosition();

		// Define a posição do jogador para a coordenada da zona segura
		m_player.SetPosition(safePosition);

		// Mensagem opcional de boas-vindas
		m_player.MessageStatus("Você foi teletransportado para uma zona segura!");

		m_player.SetAllowDamage(true);

		return m_player;
	}

	void WriteToLog(string content)
	{
		string fileName = "$profile:init.log"; // Caminho dentro da pasta do servidor
		FileHandle file = OpenFile(fileName, FileMode.APPEND);

		if (file != 0)
		{
			FPrintln(file, content); // Escreve a string com quebra de linha
			CloseFile(file);
		}
		else
		{
			Print("Erro ao abrir o arquivo para escrita.");
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