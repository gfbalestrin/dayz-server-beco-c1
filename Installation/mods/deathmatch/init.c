#include "$CurrentDir:mpmissions/dayzOffline.chernarusplus/admin/Log.c"
#include "$CurrentDir:mpmissions/dayzOffline.chernarusplus/admin/Construction.c"
#include "$CurrentDir:mpmissions/dayzOffline.chernarusplus/admin/Commands.c"
#include "$CurrentDir:mpmissions/dayzOffline.chernarusplus/admin/PlayersLoadout.c"
#include "$CurrentDir:mpmissions/dayzOffline.chernarusplus/admin/VehicleSpawner.c"
#include "$CurrentDir:mpmissions/dayzOffline.chernarusplus/admin/Classes.c"
#include "$CurrentDir:mpmissions/dayzOffline.chernarusplus/admin/DeathMatchConfig.c"
#include "$CurrentDir:mpmissions/dayzOffline.chernarusplus/admin/Messages.c"

void main()
{
	WriteToLog("main(): Inicializando servidor...");

	Hive ce = CreateHive();
	if ( ce )
	{
		WriteToLog("main(): Hive criado com sucesso. Iniciando offline...");
		ce.InitOffline();
	}
	else
	{
		WriteToLog("main(): Falha ao criar Hive.");
	}

	int year, month, day, hour, minute;
	int reset_month = 9, reset_day = 20;
	GetGame().GetWorld().GetDate(year, month, day, hour, minute);
	WriteToLog("main(): Data atual -> " + year + "/" + month + "/" + day);

	if ((month == reset_month) && (day < reset_day))
	{
		WriteToLog("main(): Ajustando data para " + reset_month + "/" + reset_day);
		GetGame().GetWorld().SetDate(year, reset_month, reset_day, hour, minute);
	}
	else if ((month == reset_month + 1) && (day > reset_day))
	{
		WriteToLog("main(): Ajustando data para " + reset_month + "/" + reset_day);
		GetGame().GetWorld().SetDate(year, reset_month, reset_day, hour, minute);
	}
	else if ((month < reset_month) || (month > reset_month + 1))
	{
		WriteToLog("main(): Ajustando data para " + reset_month + "/" + reset_day);
		GetGame().GetWorld().SetDate(year, reset_month, reset_day, hour, minute);
	}
}

class CustomMission: MissionServer
{
	string DeathMatchConfigJsonFile = "$mission:deathmatch_config.json";
	ref array<string> FixedMessages;
	float m_AdminCheckCooldown10 = 10.0;
	float m_AdminCheckTimer10 = 0.0;
	float m_AdminCheckCooldown60 = 60.0;
	float m_AdminCheckTimer60 = 0.0;

	string regionStr;
	string customMessage;
	ref array<vector> spawnZones;	
	ref array<vector> wallZones;

	void CustomMission()
	{
		ResetLog();
		WriteToLog("CustomMission(): Inicializando CustomMission");

		FixedMessages = new array<string>;
		FixedMessages.Insert("Você pode criar qualquer item pelo chat, por exemplo: !giveitem M67Grenade");

		ref SafeZoneData szData = LoadActiveRegionData(DeathMatchConfigJsonFile);
		if (szData)
		{
			WriteToLog("CustomMission(): SafeZoneData carregado");
			ToggleActiveRegion(DeathMatchConfigJsonFile);

			customMessage = szData.CustomMessage;
			regionStr = szData.Region;

			if (szData.SpawnZones)
			{
				spawnZones = szData.GetSpawnZoneVectors();
				WriteToLog("CustomMission(): spawnZones carregadas");
				foreach (vector spawnZone : spawnZones) {
					WriteToLog("spawnZone: " + spawnZone.ToString());
				}
			}
			else
			{
				WriteToLog("CustomMission(): spawnZones nulas, inicializando vazia");
				spawnZones = new array<vector>;
			}

			if (szData.WallZones)
			{
				wallZones = szData.GetWallZoneVectors();
				WriteToLog("CustomMission(): wallZones carregadas");
				foreach (vector wallZone : wallZones) {
					WriteToLog("wallZone: " + wallZone.ToString());
				}
			}
			else
			{
				WriteToLog("CustomMission(): wallZones nulas, inicializando vazia");
				wallZones = new array<vector>;
			}

			if (wallZones.Count() > 0)
			{
				WriteToLog("CustomMission(): Construindo wallzones (" + wallZones.Count() + ")");
				array<vector> points = new array<vector>;
				for (int i = 0; i < wallZones.Count(); i++)
				{
					points.Insert(wallZones[i]);
				}
				CreateLinePathFromPoints(points, "Land_Container_1Bo", 6.0, 1.0);
				CreateLinePathFromPoints(points, "Land_Container_1Bo", 6.0, 3.5);
				WriteToLog("CustomMission(): Wallzones construídas com sucesso");
			}
		}
		else
		{
			WriteToLog("CustomMission(): Erro ao carregar SafeZoneData");
		}
	}
	
	override void OnEvent(EventType eventTypeId, Param params)
	{
		super.OnEvent(eventTypeId, params);

		if (eventTypeId == ChatMessageEventTypeID)
		{
			ChatMessageEventParams chatParams = ChatMessageEventParams.Cast(params);
			if (!chatParams) {
				WriteToLog("[DEBUG] chatParams cast falhou.");
				return;
			}

			WriteToLog("[DEBUG] param1: " + chatParams.param1);
			WriteToLog("[DEBUG] param2: " + chatParams.param2);
			WriteToLog("[DEBUG] param3: " + chatParams.param3);

			int channel = chatParams.param1;          // canal (ex: 0 = Global)
			string playerName = chatParams.param2;    // nome do jogador
			string text = chatParams.param3;          // mensagem digitada			

			if (text == "")
            	return;
			
			if (text.Length() == 0 || text.Get(0) != "!")
				return;

			PlayerBase player = GetPlayerByName(playerName);
			if (!player) {
				WriteToLog("[DEBUG] Player não identificado.");
				return;
			}

			TStringArray tokensCommands = new TStringArray;
			text.Split(" ", tokensCommands);
			if (tokensCommands.Count() < 2)
				return;
			
			tokensCommands[0] = tokensCommands[0].Substring(1, tokensCommands[0].Length() - 1);
			string playerID = player.GetIdentity().GetId();
			TStringArray tokens = new TStringArray;
			tokens.Insert(playerID);
			for (int i = 0; i < tokensCommands.Count(); i++)
				tokens.Insert(tokensCommands.Get(i));
			ExecuteCommand(tokens);
		}
	}

	PlayerBase GetPlayerByName(string name)
	{
		array<Man> players = new array<Man>();
		GetGame().GetPlayers(players);

		foreach (Man man : players)
		{
			PlayerBase player = PlayerBase.Cast(man);
			if (player && player.GetIdentity() && player.GetIdentity().GetName() == name)
			{
				return player;
			}
		}
		return null;
	}


	PlayerBase GetPlayerByID(string id)
	{
		// Registra no log a busca
		WriteToLog("GetPlayerByID(): Procurando jogador com ID: " + id);
		array<Man> players = {};
		GetGame().GetPlayers(players); // Pega todos os jogadores no servidor

		// Itera sobre todos os jogadores para encontrar aquele com o ID fornecido
		foreach (Man man : players)
		{
			PlayerBase player = PlayerBase.Cast(man); // Tenta converter o jogador
			if (player && player.GetIdentity() && player.GetIdentity().GetId() == id)
			{
				// Se encontrar o jogador com o ID correto, registra e retorna o jogador
				WriteToLog("GetPlayerByID(): Jogador encontrado: " + player.GetIdentity().GetName());
				return player;
			}
		}

		// Se não encontrar, registra no log
		WriteToLog("GetPlayerByID(): Jogador não encontrado");
		return null;
	}


	override void OnUpdate(float timeslice)
	{
		super.OnUpdate(timeslice);
		m_AdminCheckTimer10 += timeslice;
		m_AdminCheckTimer60 += timeslice;

		if (m_AdminCheckTimer10 >= m_AdminCheckCooldown10)
		{
			WriteToLog("OnUpdate(): Executando verificação a cada 10s");
			m_AdminCheckTimer10 = 0.0;

			CheckAdminCommands();
			array<string> msgs = CheckMessages();

			array<Man> players = new array<Man>;
			GetGame().GetPlayers(players);
			WriteToLog("OnUpdate(): Jogadores online: " + players.Count());

			foreach (Man man : players)
			{
				PlayerBase player = PlayerBase.Cast(man);
				if (player && player.GetIdentity())
				{
					WriteToLog("OnUpdate(): Verificando player: " + player.GetIdentity().GetName());

					if (wallZones)
						CheckPlayerAreaPolygonal(player, wallZones);

					if (msgs)
					{
						foreach (string msg : msgs)
						{
							if (msg != "")
								player.MessageImportant(msg);
						}
					}
				}
			}
		}

		if (m_AdminCheckTimer60 >= m_AdminCheckCooldown60)
		{
			WriteToLog("OnUpdate(): Executando mensagens fixas e customizadas (60s)");
			AppendMessage(customMessage);
			foreach (string msgFixed : FixedMessages)
			{
				AppendMessage(msgFixed);
			}
			m_AdminCheckTimer60 = 0.0;
		}
	}


	void SetRandomHealth(EntityAI itemEnt)
	{
		if (itemEnt)
		{
			float rndHlt = Math.RandomFloat(0.45, 0.65);
			itemEnt.SetHealth01("", "", rndHlt);
			WriteToLog("SetRandomHealth(): Item " + itemEnt.GetType() + " com vida aleatória: " + rndHlt);
		}
	}

	array<string> LoadAdminIDs(string filePath)
	{
		WriteToLog("LoadAdminIDs(): Carregando IDs do arquivo: " + filePath);
		array<string> ids = new array<string>;
		FileHandle file = OpenFile(filePath, FileMode.READ);

		if (file != 0)
		{
			string line;
			while (FGets(file, line) > 0)
			{
				line = line.Trim();
				if (line != "")
					ids.Insert(line);
			}
			CloseFile(file);
			WriteToLog("LoadAdminIDs(): IDs carregados: " + ids.Count());
		}
		else
		{
			WriteToLog("LoadAdminIDs(): Erro ao abrir o arquivo.");
		}
		return ids;
	}

	override PlayerBase CreateCharacter(PlayerIdentity identity, vector pos, ParamsReadContext ctx, string characterName)
	{
		WriteToLog("CreateCharacter(): Criando personagem para " + identity.GetName());

		Entity playerEnt = GetGame().CreatePlayer(identity, characterName, pos, 0, "NONE");
		if (!playerEnt)
		{
			WriteToLog("CreateCharacter(): Erro ao criar player!");
			return null;
		}

		Class.CastTo(m_player, playerEnt);
		if (!m_player)
		{
			WriteToLog("CreateCharacter(): Erro ao fazer cast para PlayerBase");
			return null;
		}

		GetGame().SelectPlayer(identity, m_player);

		array<string> adminIDs = LoadAdminIDs("$mission:admin_ids.txt");
		if (adminIDs.Find(identity.GetId()) != -1)
		{
			WriteToLog("CreateCharacter(): " + identity.GetName() + " é admin.");
			m_player.SetAllowDamage(false);
			GiveAdminLoadout(m_player);
		}
		else
		{
			WriteToLog("CreateCharacter(): " + identity.GetName() + " é jogador comum.");
			m_player.SetAllowDamage(false);

			if (!GiveCustomLoadout(m_player, identity.GetId()))
			{
				WriteToLog("CreateCharacter(): Loadout customizado não encontrado. Aplicando padrão.");
				GiveDefaultLoadout(m_player);
			}

			m_player.SetHealth("", "", 100);
			m_player.SetHealth("GlobalHealth", "Blood", 5000);
			m_player.SetHealth("GlobalHealth", "Shock", 0);
			m_player.GetStatEnergy().Set(4000);
			m_player.GetStatWater().Set(4000);

			for (int i = 0; i < spawnZones.Count(); i++)
			{
				WriteToLog("spawnZone: " + spawnZones[i].ToString());
			}

			vector safePosition = GetRandomSafeSpawnPosition(spawnZones);
			WriteToLog("CreateCharacter(): Posicionando jogador em: " + safePosition.ToString());
			m_player.SetPosition(safePosition);
			m_player.SetAllowDamage(true);
		}

		return m_player;
	}
};

Mission CreateCustomMission(string path)
{
	WriteToLog("CreateCustomMission(): Criando instância de CustomMission");
	return new CustomMission();
}
