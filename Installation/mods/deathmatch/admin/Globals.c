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
ref VoteManager g_VoteManager;
ref map<string, int> playerVotes = new map<string, int>();  // playerID -> RegionId
ref map<int, int> voteCounts = new map<int, int>();         // RegionId -> contagem de votos
bool isVotingActive = false;
float votingDuration = 300.0; // 300 segundos = 5 minutos
ref Timer votingTimer;
