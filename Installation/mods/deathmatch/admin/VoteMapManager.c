class VoteMapManager
{
	private ref map<string, int> playerVotesMap = new ref map<string, int>();  // playerID -> RegionId
	private ref map<int, int> voteCountsMap = new ref map<int, int>();         // RegionId -> contagem de votos
	private ref Timer votingMapTimer;

	private bool isVotingMapActive = false;
	private float votingMapDuration = 300.0;
	private bool changeMapNow = false;

    void VoteMapManager()
    {
        votingMapTimer = new Timer(CALL_CATEGORY_GAMEPLAY);
    }

	void SetChangeMapNow(bool value)
	{
		changeMapNow = value;
	}
    bool GetStatusVotingMap()
	{
		return isVotingMapActive;
	}

	void IniciaVotacaoProximoMapa()
	{
		if (isVotingMapActive) return;

		isVotingMapActive = true;
		votingMapTimer.Run(votingMapDuration, this, "FinalizarVotacaoMapaTimer", null);
		string tempo = FormatTempo(votingMapDuration);

		BroadcastMessage("Votação iniciada! Você tem " + tempo + " para votar.", MessageColor.FRIENDLY);
		WriteToLog("Votação iniciada! Os jogadores têm " + tempo + " para votar.", LogFile.INIT, false, LogType.INFO);

		foreach (ref SafeZoneData mapV : maps)
		{
			BroadcastMessage(mapV.RegionId.ToString() + " - " + mapV.Region + " - digite no chat: !votemap " + mapV.RegionId.ToString(), MessageColor.FRIENDLY);
		}
	}

	void HandleVote(string playerID, int regionId)
	{
		if (!isVotingMapActive)
		{
			SendPrivateMessage(playerID, "A votação ainda não foi iniciada.", MessageColor.WARNING);
			return;
		}

		if (playerVotesMap.Contains(playerID))
		{
			SendPrivateMessage(playerID, "Você já votou nesta rodada.", MessageColor.WARNING);
			return;
		}

		playerVotesMap.Insert(playerID, regionId);

		int currentVotes = 0;
		if (voteCountsMap.Contains(regionId))
			currentVotes = voteCountsMap.Get(regionId);

		voteCountsMap.Set(regionId, currentVotes + 1);

		string mapName;
		foreach (ref SafeZoneData mapI : maps)
		{
			if (mapI && mapI.RegionId == regionId)
				mapName = mapI.Region;
		}

		SendPrivateMessage(playerID, "Voto registrado para o mapa (" + regionId + ") " + mapName, MessageColor.FRIENDLY);
		WriteToLog("VOTO: " + playerID + " votou em (" + regionId + ") " + mapName, LogFile.INIT, false, LogType.INFO);

		array<Man> playersOnline = new array<Man>();
        GetGame().GetPlayers(playersOnline);

        int totalOnline = 0;
        int totalVotaram = 0;

        // Coletar todos os players válidos
        foreach (Man man : playersOnline)
        {
            string id = GetPlayerId(man);
            if (id == "") continue;

            totalOnline++;

            if (playerVotesMap.Contains(id))
                totalVotaram++;
        }

        // DEBUG opcional
        WriteToLog("Jogadores online: " + totalOnline.ToString(), LogFile.INIT, false, LogType.DEBUG);
        WriteToLog("Jogadores que votaram: " + totalVotaram.ToString(), LogFile.INIT, false, LogType.DEBUG);

        if (totalOnline > 0 && totalVotaram == totalOnline)
        {
            if (votingMapTimer && votingMapTimer.IsRunning())
                votingMapTimer.Stop();

            WriteToLog("Todos os jogadores votaram. Encerrando votação.", LogFile.INIT, false, LogType.INFO);
            FinalizarVotacaoMapaTimer();
        }

	}

	void FinalizarVotacaoMapaTimer()
	{
		isVotingMapActive = false;

		int highest = -1;
		int winner = -1;

		foreach (int regionId, int count : voteCountsMap)
		{
			if (count > highest)
			{
				highest = count;
				winner = regionId;
			}
		}

		if (winner != -1)
		{
			string mapName;
			foreach (ref SafeZoneData mapW : maps)
			{
				if (mapW && mapW.RegionId == winner)
				{
					mapName = mapW.Region;
					nextMap = mapW;
				}
			}

			if (changeMapNow)
			{
				array<Man> playersOnline = new array<Man>();
				GetGame().GetPlayers(playersOnline);

				int totalOnline = 0;
				int votosNoVencedor = 0;

				foreach (Man man : playersOnline)
				{
					string id = GetPlayerId(man);
                    if (id == "") continue;

					totalOnline++;

					if (playerVotesMap.Contains(id) && playerVotesMap.Get(id) == winner)
						votosNoVencedor++;
				}

				WriteToLog("Total online: " + totalOnline.ToString(), LogFile.INIT, false, LogType.DEBUG);
				WriteToLog("Votos no vencedor: " + votosNoVencedor.ToString(), LogFile.INIT, false, LogType.DEBUG);

				if (votosNoVencedor == totalOnline && totalOnline > 0)
				{
					BroadcastMessage("Votação unânime! Reiniciando com o mapa: " + mapName, MessageColor.IMPORTANT);
					SetActiveRegionById(winner);
					AppendExternalAction("{\"action\": \"restart_server\", \"minutes\": 1, \"message\": \"Servidor será reiniciado em 1 minuto\"}");
				}
				else
				{
					BroadcastMessage("A votação não foi unânime. Nenhuma troca será feita.", MessageColor.WARNING);
				}
			}
			else
			{
                if (mapName == "")
	                mapName = "ID " + winner.ToString();

				BroadcastMessage("Mapa vencedor: " + winner + " - " + mapName + " com " + highest.ToString() + " votos.", MessageColor.FRIENDLY);
				SetActiveRegionById(winner);
			}
		}
		else
		{
			BroadcastMessage("Nenhum voto recebido. O próximo mapa será " + nextMap.Region, MessageColor.FRIENDLY);
		}

		ResetVotingMap();
	}

	void ResetVotingMap()
	{
		playerVotesMap.Clear();
		voteCountsMap.Clear();
		isVotingMapActive = false;
	}

	void ShowResultVotingMap(string playerID)
	{
		if (!isVotingMapActive)
		{
			SendPrivateMessage(playerID, "Nenhuma votação está ativa no momento.", MessageColor.WARNING);
			return;
		}

		SendPrivateMessage(playerID, "Resultado parcial da votação:", MessageColor.FRIENDLY);

		foreach (ref SafeZoneData mapS : maps)
		{
			int votos = 0;
			if (voteCountsMap.Contains(mapS.RegionId))
				votos = voteCountsMap.Get(mapS.RegionId);

			string linha = mapS.RegionId.ToString() + " - " + mapS.Region + " (" + votos.ToString() + " voto";
			if (votos != 1) linha += "s";
			linha += ")";

			SendPrivateMessage(playerID, linha, MessageColor.FRIENDLY);
		}
	}
    void CheckVotingStatus(string playerID)
    {
        // Votação ativa exibe resultados, senão exibe tutorial
        if (isVotingMapActive) {
            ShowResultVotingMap(playerID);
        } else {
            foreach (ref SafeZoneData mapL : maps) {
                string linha = mapL.RegionId.ToString() + " - " + mapL.Region;                
                SendPrivateMessage(playerID, linha, MessageColor.FRIENDLY);
            }
        }         
        SendPrivateMessage(playerID, "Uso: !votemap <ID do mapa>", MessageColor.WARNING);
    }

    void CheckIfVotingAndStart(string playerID, int regionId)
    {
        if (serverWillRestartSoon && changeMapNow)
        {
            SendPrivateMessage(playerID, "Não é possível abrir votação pois o servidor vai reiniciar em breve", MessageColor.WARNING);
            return;
        }
        if (!isVotingMapActive)
        {
            IniciaVotacaoProximoMapa();
            SetChangeMapNow(true);   
        }

        HandleVote(playerID, regionId);        
    }
}
