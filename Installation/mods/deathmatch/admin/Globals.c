string DeathMatchConfigJsonFile = "$mission:admin/files/deathmatch_config.json";
enum MessageColor
{
    STATUS,     // azul
    IMPORTANT,  // vermelho
    FRIENDLY,   // verde
    WARNING      // amarelo (via RPC)
}
enum LogType
{
    DEBUG,
    ERROR,
    INFO
}
enum LogFile
{
    INIT,
    POSITION
}
ref array<ref SafeZoneData> maps;
ref SafeZoneData currentMap;
ref SafeZoneData nextMap;

// Votação de mapa
ref VoteMapManager g_VoteMapManager;
ref map<string, int> playerVotesMap = new map<string, int>();  // playerID -> RegionId
ref map<int, int> voteCountsMap = new map<int, int>();         // RegionId -> contagem de votos
bool isVotingMapActive = false;
float votingMapDuration = 300.0; // 300 segundos = 5 minutos
ref Timer votingMapTimer;

// Votação de kick
ref VoteKickManager g_VoteKickManager;
ref map<string, bool> playerVotesKick = new map<string, bool>();  // playerID -> true (sim) / false (não)
private bool isVotingKickActive = false;
private float votingKickDuration = 120.0; // 2 minutos
private ref Timer votingKickTimer;
