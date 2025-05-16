class VoteKickManager
{
	private string targetPlayerId;
	private string targetPlayerName;


	void VoteKickManager()
	{
		votingKickTimer = new Timer(CALL_CATEGORY_GAMEPLAY);
	}

	void StartKickVote(string callerId, string targetId, string targetName)
	{
		if (isVotingKickActive) {
			SendPrivateMessage(callerId, "Já existe uma votação de kick em andamento.", MessageColor.WARNING);
			return;
		}

		array<Man> players = new array<Man>();
		GetGame().GetPlayers(players);

		bool found = false;
		foreach (Man man : players) {
			PlayerBase player = PlayerBase.Cast(man);
			if (player && player.GetIdentity() && player.GetIdentity().GetPlainId() == targetId) {
				found = true;
				break;
			}
		}

		if (!found) {
			SendPrivateMessage(callerId, "O jogador não está online ou o ID está incorreto.", MessageColor.WARNING);
			return;
		}

		isVotingKickActive = true;
		targetPlayerId = targetId;
		targetPlayerName = targetName;
		playerVotesKick.Clear();

		votingKickTimer.Run(votingKickDuration, this, "FinalizarKickVote");

		BroadcastMessage("Votação para kickar " + targetPlayerName + " iniciada! Digite 1 para SIM ou 2 para NÃO.", MessageColor.WARNING);
		WriteToLog("Votação de kick iniciada por " + callerId + " contra " + targetPlayerId);
	}

	void HandleVote(string playerId, int vote)
	{
		if (!isVotingKickActive) {
			SendPrivateMessage(playerId, "Nenhuma votação de kick está ativa no momento.", MessageColor.WARNING);
			return;
		}

		if (playerId == targetPlayerId) {
			SendPrivateMessage(playerId, "Você não pode votar na votação para seu próprio kick.", MessageColor.WARNING);
			return;
		}

		if (playerVotesKick.Contains(playerId)) {
			SendPrivateMessage(playerId, "Você já votou.", MessageColor.WARNING);
			return;
		}

		if (vote != 1 && vote != 2) {
			SendPrivateMessage(playerId, "Voto inválido. Digite 1 para SIM ou 2 para NÃO.", MessageColor.WARNING);
			return;
		}

		playerVotesKick.Insert(playerId, vote == 1);

		SendPrivateMessage(playerId, "Seu voto foi registrado.", MessageColor.FRIENDLY);

		CheckIfAllVoted();
	}

	void CheckIfAllVoted()
	{
		array<Man> players = new array<Man>();
		GetGame().GetPlayers(players);

		int totalVoters = 0;
		foreach (Man man : players) {
			PlayerBase player = PlayerBase.Cast(man);
			if (!player || !player.GetIdentity()) continue;

			string id = player.GetIdentity().GetPlainId();
			if (id != targetPlayerId) totalVoters++;
		}

		if (playerVotesKick.Count() >= totalVoters) {
			votingKickTimer.Stop();
			FinalizarKickVote();
		}
	}

	void FinalizarKickVote()
	{
		int totalVotes = playerVotesKick.Count();
		int simVotes = 0;

		foreach (bool v : playerVotesKick) {
			if (v) simVotes++;
		}

		array<Man> players = new array<Man>();
		GetGame().GetPlayers(players);

		int totalVoters = 0;
		foreach (Man man : players) {
			PlayerBase player = PlayerBase.Cast(man);
			if (!player || !player.GetIdentity()) continue;

			string id = player.GetIdentity().GetPlainId();
			if (id != targetPlayerId) totalVoters++;
		}

		if (simVotes == totalVoters) {
			BroadcastMessage("Jogador " + targetPlayerName + " foi kickado por votação unânime!", MessageColor.IMPORTANT);
			KickPlayerById(targetPlayerId);
			WriteToLog("Jogador " + targetPlayerId + " kickado após votação.");
		} else {
			BroadcastMessage("Votação para kickar " + targetPlayerName + " falhou. Votos SIM: " + simVotes + "/" + totalVoters, MessageColor.WARNING);
		}

		ResetKickVote();
	}

	void ResetKickVote()
	{
		isVotingKickActive = false;
		playerVotesKick.Clear();
		targetPlayerId = "";
		targetPlayerName = "";
	}

	void KickPlayerById(string playerId)
	{
		array<Man> players = new array<Man>();
		GetGame().GetPlayers(players);

		foreach (Man man : players) {
			PlayerBase player = PlayerBase.Cast(man);
			if (player && player.GetIdentity() && player.GetIdentity().GetPlainId() == playerId) {
				GetGame().DisconnectPlayer(player.GetIdentity(), "Você foi kickado por votação.");
				return;
			}
		}
	}

    void ListarJogadoresOnline(string solicitanteId)
    {
        array<Man> players = new array<Man>();
        GetGame().GetPlayers(players);

        if (players.Count() <= 1) {
            SendPrivateMessage(solicitanteId, "Você é o único jogador online.", MessageColor.WARNING);
            return;
        }

        SendPrivateMessage(solicitanteId, "Jogadores online:", MessageColor.FRIENDLY);

        foreach (Man man : players) {
            PlayerBase player = PlayerBase.Cast(man);
            if (!player || !player.GetIdentity()) continue;

            string playerId = player.GetIdentity().GetPlainId();
            string playerName = player.GetIdentity().GetName();

            if (playerId == solicitanteId) continue; // Oculta o próprio jogador da lista

            SendPrivateMessage(solicitanteId, playerName + " - ID: " + playerId, MessageColor.FRIENDLY);
        }
    }

}
