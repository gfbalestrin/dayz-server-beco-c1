class VoteManager
{
	private ref map<string, int> playerVotes = new ref map<string, int>();
	private ref map<int, int> voteCounts = new ref map<int, int>();

	void VoteManager()
	{
		votingTimer = new Timer(CALL_CATEGORY_GAMEPLAY);
	}

	void HandleVote(string playerID, int regionId)
	{
		if (playerVotes.Contains(playerID)) {
			SendPrivateMessage(playerID, "Você já votou nesta rodada.");
			return;
		}

        if (!isVotingActive)
        {
            SendPrivateMessage(playerID, "A votação ainda não foi iniciada.");
            return;
        }

		playerVotes.Insert(playerID, regionId);

		int currentVotes = 0;
		if (voteCounts.Contains(regionId))
			currentVotes = voteCounts.Get(regionId);

		voteCounts.Set(regionId, currentVotes + 1);
        string mapName;
        foreach (ref SafeZoneData mapI : maps) {
            WriteToLog("Mapa: " + mapI.Region);
            if (mapI && mapI.RegionId == regionId) {                
                mapName = mapI.Region;
            }
        }

		SendPrivateMessage(playerID, "Voto registrado para o mapa (" + regionId + ") " + mapName);
		WriteToLog("VOTO: " + playerID + " votou em (" + regionId + ") " + mapName);
	}

    void IniciaVotacao()
    {
        if (isVotingActive)
            return;
        
        isVotingActive = true;
        votingTimer.Run(votingDuration, this, "FinalizarVotacaoTimer", null);
        string tempo = FormatTempo(votingDuration);

        BroadcastMessage("Votação iniciada! Você tem " + tempo + " para votar.", MessageColor.IMPORTANT);
        WriteToLog("Votação iniciada! Os jogadores tem " + tempo + " para votar. Mapas disponíveis: ");

        foreach (ref SafeZoneData mapV : maps) {
            BroadcastMessage(mapV.RegionId.ToString() + " - " + mapV.Region + " - digite no chat: !votemap " + mapV.RegionId.ToString(), MessageColor.FRIENDLY);
        }
        
    }

	void FinalizarVotacaoTimer()
	{
		isVotingActive = false;

		int highest = -1;
		int winner = -1;
		foreach (int regionId, int count : voteCounts) {
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

        ResetVoting();
	}
    void ResetVoting()
    {
        playerVotes.Clear();
        voteCounts.Clear();
        isVotingActive = false;
    }
    void EnviarResultadoParcialParaJogador(string playerID)
    {
        if (!isVotingActive) {
            SendPrivateMessage(playerID, "Nenhuma votação está ativa no momento.", MessageColor.WARNING);
            return;
        }

        SendPrivateMessage(playerID, "Resultado parcial da votação:", MessageColor.IMPORTANT);

        foreach (ref SafeZoneData mapS : maps) {
            int votos = 0;
            if (voteCounts.Contains(mapS.RegionId)) {
                votos = voteCounts.Get(mapS.RegionId);
            }

            string linha = mapS.RegionId.ToString() + " - " + mapS.Region + " (" + votos.ToString() + " voto";
            if (votos != 1) linha += "s";
            linha += ")";
            
            SendPrivateMessage(playerID, linha, MessageColor.FRIENDLY);
        }
    }
    void CheckCurrentVoting(string playerID)
    {
        if (isVotingActive) {
            EnviarResultadoParcialParaJogador(playerID);
            return;
        }

        SendPrivateMessage(playerID, "⚠️ Nenhuma votação está ativa no momento.", MessageColor.WARNING);
    }
}