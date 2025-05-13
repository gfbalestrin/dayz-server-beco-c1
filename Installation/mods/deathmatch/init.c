#include "$CurrentDir:mpmissions/dayzOffline.chernarusplus/admin/Log.c"
#include "$CurrentDir:mpmissions/dayzOffline.chernarusplus/admin/Commands.c"
#include "$CurrentDir:mpmissions/dayzOffline.chernarusplus/admin/PlayersLoadout.c"
#include "$CurrentDir:mpmissions/dayzOffline.chernarusplus/admin/VehicleSpawner.c"
#include "$CurrentDir:mpmissions/dayzOffline.chernarusplus/admin/Construction.c"
#include "$CurrentDir:mpmissions/dayzOffline.chernarusplus/admin/Classes.c"
#include "$CurrentDir:mpmissions/dayzOffline.chernarusplus/admin/DeathMatchConfig.c"
#include "$CurrentDir:mpmissions/dayzOffline.chernarusplus/admin/Messages.c"

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
	ref array<string> FixedMessages;

	float m_AdminCheckCooldown10 = 10.0;
	float m_AdminCheckTimer10 = 0.0;
	float m_AdminCheckCooldown60 = 60.0;
	float m_AdminCheckTimer60 = 0.0;

	string regionStr;
	string customMessage;
	vector areaMin;
    vector areaMax;
    ref array<vector> safeZones;	
	ref array<vector> wallZones;

	void CustomMission()
	{
		WriteToLog("Entrou no construtor CustomMission");
		FixedMessages = new array<string>;
        FixedMessages.Insert("Você pode criar qualquer item pelo chat, por exemplo: /admin giveitem M67Grenade");

		ref SafeZoneData szData = LoadActiveRegionData("$mission:deathmatch_config.json");
		if (szData)
		{
			customMessage = szData.customMessage;
			regionStr = szData.regionStr;
			areaMin = szData.areaMin;
			areaMax = szData.areaMax;
			safeZones = szData.safeZones;
			wallZones = szData.wallZones;
			if (wallZones.Count() > 0)
			{
				WriteToLog("O mapa possui wallzones..." + wallZones.Count());
				array<vector> points = new array<vector>;
				for (int i = 0; i < wallZones.Count(); i++)
					points.Insert(wallZones[i]); 

				CreateLinePathFromPoints(points, "Land_Container_1Bo", 6.0, 1.0);
				CreateLinePathFromPoints(points, "Land_Container_1Bo", 6.0, 3.5);
				WriteToLog("Wallzones construidas!");
			}

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
		m_AdminCheckTimer10 += timeslice;
		if (m_AdminCheckTimer10 >= m_AdminCheckCooldown10)
		{
			m_AdminCheckTimer10 = 0.0;
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
					CheckPlayerArea(player, areaMin, areaMax);	

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
				foreach (string msgFixed : FixedMessages)
				{
					AppendMessage(msgFixed);
				}
					
			}			

			m_AdminCheckTimer60 = 0.0;
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
		if (adminIDs.Find(identity.GetId()) != -1)
		{
			m_player.SetAllowDamage(false);
			GiveAdminLoadout(m_player);
		} else {
			m_player.SetAllowDamage(false);

			if (!GiveCustomLoadout(m_player, identity.GetId()))
				GiveDefaultLoadout(m_player);

			m_player.SetHealth("", "", 100);
			m_player.SetHealth("GlobalHealth", "Blood", 5000);
			m_player.SetHealth("GlobalHealth", "Shock", 0);
			m_player.GetStatEnergy().Set(4000);
			m_player.GetStatWater().Set(4000);

			vector safePosition = GetRandomSafeSpawnPosition(safeZones);
			m_player.SetPosition(safePosition);
			m_player.SetAllowDamage(true);
		}
		

		return m_player;
	}
};

Mission CreateCustomMission(string path)
{
	return new CustomMission();
}