class VoteMapManager
{
	private ref map<string, int> playerVotesMap = new ref map<string, int>();
	private ref map<int, int> voteCountsMap = new ref map<int, int>();

	void VoteMapManager()
	{
		votingMapTimer = new Timer(CALL_CATEGORY_GAMEPLAY);
	}

	void HandleVote(string playerID, int regionId)
	{
		if (playerVotesMap.Contains(playerID)) {
			SendPrivateMessage(playerID, "Você já votou nesta rodada.", MessageColor.WARNING);
			return;
		}

        if (!isVotingMapActive)
        {
            SendPrivateMessage(playerID, "A votação ainda não foi iniciada.", MessageColor.WARNING);
            return;
        }

		playerVotesMap.Insert(playerID, regionId);

		int currentVotes = 0;
		if (voteCountsMap.Contains(regionId))
			currentVotes = voteCountsMap.Get(regionId);

		voteCountsMap.Set(regionId, currentVotes + 1);
        string mapName;
        foreach (ref SafeZoneData mapI : maps) {
            WriteToLog("Mapa: " + mapI.Region);
            if (mapI && mapI.RegionId == regionId) {                
                mapName = mapI.Region;
            }
        }

		SendPrivateMessage(playerID, "Voto registrado para o mapa (" + regionId + ") " + mapName, MessageColor.FRIENDLY);
		WriteToLog("VOTO: " + playerID + " votou em (" + regionId + ") " + mapName);

        // Verifica se todos os jogadores online já votaram
        array<Man> playersOnline = new array<Man>();
        GetGame().GetPlayers(playersOnline);

        int totalOnline = 0;
        int totalVotaram = 0;

        foreach (Man man : playersOnline)
        {
            PlayerBase player = PlayerBase.Cast(man);
            if (!player || !player.GetIdentity()) continue;

            string id = player.GetIdentity().GetPlainId();
            totalOnline++;

            if (playerVotesMap.Contains(id))
                totalVotaram++;
        }

        if (totalOnline > 0 && totalVotaram == totalOnline)
        {
            // Todos os jogadores online já votaram, finaliza a votação
            votingMapTimer.Stop();  // Para o timer, se estiver rodando
            FinalizarVotacaoMapaTimer();
        }
	}

    void IniciaVotacaoProximoMapa()
    {
        if (isVotingMapActive)
            return;
        
        isVotingMapActive = true;
        votingMapTimer.Run(votingMapDuration, this, "FinalizarVotacaoMapaTimer", null);
        string tempo = FormatTempo(votingMapDuration);

        BroadcastMessage("Votação iniciada! Você tem " + tempo + " para votar.", MessageColor.FRIENDLY);
        WriteToLog("Votação iniciada! Os jogadores tem " + tempo + " para votar. Mapas disponíveis: ");

        foreach (ref SafeZoneData mapV : maps) {
            BroadcastMessage(mapV.RegionId.ToString() + " - " + mapV.Region + " - digite no chat: !votemap " + mapV.RegionId.ToString(), MessageColor.FRIENDLY);
        }
        
    }

	void FinalizarVotacaoMapaTimer()
	{
		isVotingMapActive = false;

		int highest = -1;
		int winner = -1;
		foreach (int regionId, int count : voteCountsMap) {
			if (count > highest) {
				highest = count;
				winner = regionId;
			}
		}

		if (winner != -1) {     
            string mapName;    
            foreach (ref SafeZoneData mapW : maps) {
                WriteToLog("Mapa: " + mapW.Region);
                if (mapW && mapW.RegionId == regionId) {
                    mapName = mapW.Region;
                    nextMap = mapW;
                }
            }

            WriteToLog("Mapa vencedor: (" + regionId + ") " + mapName + " com " + highest.ToString() + " votos.");
			BroadcastMessage("Mapa vencedor: " + regionId + " - " + mapName + " com " + highest.ToString() + " votos.", MessageColor.FRIENDLY);
			SetActiveRegionById(regionId);
            
		} else {
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
        if (!isVotingMapActive) {
            SendPrivateMessage(playerID, "Nenhuma votação está ativa no momento.", MessageColor.WARNING);
            return;
        }

        SendPrivateMessage(playerID, "Resultado parcial da votação:", MessageColor.FRIENDLY);

        foreach (ref SafeZoneData mapS : maps) {
            int votos = 0;
            if (voteCountsMap.Contains(mapS.RegionId)) {
                votos = voteCountsMap.Get(mapS.RegionId);
            }

            string linha = mapS.RegionId.ToString() + " - " + mapS.Region + " (" + votos.ToString() + " voto";
            if (votos != 1) linha += "s";
            linha += ")";
            
            SendPrivateMessage(playerID, linha, MessageColor.FRIENDLY);
        }
    }
    void CheckCurrentVotingMap(string playerID)
    {
        if (isVotingMapActive) {
            ShowResultVotingMap(playerID);
        } else {
            SendPrivateMessage(playerID, "Nenhuma votação está ativa no momento.", MessageColor.WARNING);
        }        
    }
}