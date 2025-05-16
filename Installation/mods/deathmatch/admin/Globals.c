string DeathMatchConfigJsonFile = "$mission:deathmatch_config.json";
enum MessageColor
{
    STATUS,     // azul
    IMPORTANT,  // vermelho
    FRIENDLY,   // verde
    WARNING      // amarelo (via RPC)
}
ref array<ref SafeZoneData> maps;
ref SafeZoneData currentMap;
ref SafeZoneData nextMap;

// Votação
ref VoteMapManager g_VoteMapManager;
// Votação de mapa
ref map<string, int> playerVotesMap = new map<string, int>();  // playerID -> RegionId
ref map<int, int> voteCountsMap = new map<int, int>();         // RegionId -> contagem de votos
bool isVotingMapActive = false;
float votingMapDuration = 300.0; // 300 segundos = 5 minutos
ref Timer votingMapTimer;
