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
	vector areaMin = "2450 0 1090"; // X Y Z (Y não é usado nesse caso)
	vector areaMax = "3280 0 1462";

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
				}
			}
		}
	}

	void CheckPlayerArea(PlayerBase player)
	{
		vector pos = player.GetPosition();
		
		// Checa se o jogador está fora da área permitida
		if (pos[0] < areaMin[0] || pos[0] > areaMax[0] || pos[2] < areaMin[2] || pos[2] > areaMax[2])
		{
			// Exemplo: teleportar de volta ao centro da área
			vector safePos = "2686 0 1312";
			player.SetPosition(safePos);

			// Opcional: mensagem para o jogador
			player.MessageStatus("Você não pode sair da zona segura!");
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

		return m_player;
	}

	override void StartingEquipSetup(PlayerBase player, bool clothesChosen)
	{
		EntityAI itemClothing;
		EntityAI itemEnt;
		ItemBase itemBs;
		float rand;

		itemClothing = player.FindAttachmentBySlotName( "Body" );
		if ( itemClothing )
		{
			SetRandomHealth( itemClothing );
			
			itemEnt = itemClothing.GetInventory().CreateInInventory( "BandageDressing" );
			player.SetQuickBarEntityShortcut(itemEnt, 2);
			
			string chemlightArray[] = { "Chemlight_White", "Chemlight_Yellow", "Chemlight_Green", "Chemlight_Red" };
			int rndIndex = Math.RandomInt( 0, 4 );
			itemEnt = itemClothing.GetInventory().CreateInInventory( chemlightArray[rndIndex] );
			SetRandomHealth( itemEnt );
			player.SetQuickBarEntityShortcut(itemEnt, 1);

			rand = Math.RandomFloatInclusive( 0.0, 1.0 );
			if ( rand < 0.35 )
				itemEnt = player.GetInventory().CreateInInventory( "Apple" );
			else if ( rand > 0.65 )
				itemEnt = player.GetInventory().CreateInInventory( "Pear" );
			else
				itemEnt = player.GetInventory().CreateInInventory( "Plum" );
			player.SetQuickBarEntityShortcut(itemEnt, 3);
			SetRandomHealth( itemEnt );
		}
		
		itemClothing = player.FindAttachmentBySlotName( "Legs" );
		if ( itemClothing )
			SetRandomHealth( itemClothing );
		
		itemClothing = player.FindAttachmentBySlotName( "Feet" );
	}
};

Mission CreateCustomMission(string path)
{
	return new CustomMission();
}