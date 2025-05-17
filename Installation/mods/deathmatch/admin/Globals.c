string DeathMatchConfigJsonFile = "$mission:admin/files/deathmatch_config.json";
string LoadoutDefaultJsonFile = "$mission:admin/loadouts/default.json";
string LoadoutAdminJsonFile = "$mission:admin/loadouts/admin.json";
string LoadoutPlayersIdsJsonFile = "$mission:admin/loadouts/players_ids.json";
string LoadoutPlayersFolder = "$mission:admin/loadouts/players/";
string ExternalCommandsFile = "$mission:admin/files/commands_to_execute.txt";
string ExternalActionsFile = "$mission:admin/files/external_actions.txt";
string MessagesToSendoFile = "$mission:admin/files/messages_to_send.txt";
string MessagesPrivateToSendoFile = "$mission:admin/files/messages_private_to_send.txt";
string AdminIdsFile = "$mission:admin/files/admin_ids.txt";
string UrlAppPython = "http://beco.servegame.com:54321/";

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
