ref SafeZoneData LoadActiveRegionData(string path)
{
    WriteToLog("Iniciando carregamento do arquivo: " + path);

    FileHandle file = OpenFile(path, FileMode.READ);
    if (!file) {
        WriteToLog("Arquivo não encontrado: " + path);
        return null;
    }

    string content, line;
    while (FGets(file, line) > 0) {
        content += line;
    }
    CloseFile(file);
    WriteToLog("Conteúdo do arquivo carregado.");

    int startObj = content.IndexOf("{");
    while (startObj != -1) {
        int relEnd = content.Substring(startObj).IndexOf("}");
        if (relEnd == -1) break;

        int endObj = startObj + relEnd;
        string objStr = content.Substring(startObj, endObj - startObj + 1);

        if (objStr.IndexOf("\"Active\":") != -1 && objStr.Substring(objStr.IndexOf("\"Active\":") + 9, 5).ToLower().Contains("true")) {
            auto data = new SafeZoneData();
            WriteToLog("Região ativa encontrada. Processando dados...");

            data.regionStr = ExtractJsonStringValue(objStr, "\"Region\":");
            WriteToLog("Region: " + data.regionStr);

            data.customMessage = ExtractJsonStringValue(objStr, "\"CustomMessage\":");
            WriteToLog("Mensagem personalizada: " + data.customMessage);

            data.areaMin = ExtractVectorFromJson(objStr, "\"AreaMin\":");
            WriteToLog("Área mínima: " + data.areaMin.ToString());

            data.areaMax = ExtractVectorFromJson(objStr, "\"AreaMax\":");
            WriteToLog("Área máxima: " + data.areaMax.ToString());

            ExtractVectorArray(objStr, "\"SafeZones\":", data.safeZones);
            WriteToLog("SafeZones carregadas: " + data.safeZones.Count().ToString());

            ExtractVectorArray(objStr, "\"WallZones\":", data.wallZones);
            WriteToLog("WallZones carregadas: " + data.wallZones.Count().ToString());

            return data;
        }

        startObj = content.Substring(endObj + 1).IndexOf("{");
        if (startObj != -1) {
            startObj = endObj + 1 + startObj;
        } else {
            startObj = -1;
        }
    }

    WriteToLog("Nenhuma região ativa encontrada.");
    return null;
}

string ExtractJsonStringValue(string json, string key)
{
    int idx = json.IndexOf(key);
    if (idx == -1) return "";

    string sub = json.Substring(idx + key.Length(), json.Length() - idx - key.Length());
    int sQuote = sub.IndexOf("\"") + 1;
    int eQuote = sub.Substring(sQuote).IndexOf("\"");

    if (sQuote == -1 || eQuote == -1) return "";

    return sub.Substring(sQuote, eQuote);
}

vector ExtractVectorFromJson(string json, string key)
{
    string raw = ExtractJsonStringValue(json, key);
    return raw.ToVector();
}

void ExtractVectorArray(string json, string key, out array<vector> output)
{
    output = new array<vector>();

    int idx = json.IndexOf(key);
    if (idx == -1) return;

    string sub = json.Substring(idx);
    int sBracket = sub.IndexOf("[");
    int eBracket = sub.IndexOf("]");
    if (sBracket == -1 || eBracket == -1 || eBracket <= sBracket) return;

    string rawBlock = sub.Substring(sBracket + 1, eBracket - sBracket - 1);

    array<string> entries = new array<string>();
    rawBlock.Split(",", entries);

    for (int i = 0; i + 2 < entries.Count(); i += 3) {
        string vecStr = entries[i] + "," + entries[i + 1] + "," + entries[i + 2];
        vecStr.Replace("\"", "");
        vecStr.Trim();

        TStringArray parts = new TStringArray();
        vecStr.Split(",", parts);

        if (parts.Count() == 3) {
            float x = parts[0].Trim().ToFloat();
            float y = parts[1].Trim().ToFloat();
            float z = parts[2].Trim().ToFloat();
            output.Insert(Vector(x, y, z));
        }
    }
}

vector GetRandomSafeSpawnPosition(array<vector> safeZones)
{
    if (safeZones.Count() == 0) {
        WriteToLog("Nenhuma zona segura disponível para spawn.");
        return "0 0 0";
    }

    int index = Math.RandomInt(0, safeZones.Count());
    WriteToLog("Posição segura selecionada: " + safeZones[index].ToString());
    return safeZones[index];
}

void CheckPlayerArea(PlayerBase player, vector areaMin, vector areaMax)
{
    vector pos = player.GetPosition();
    if (pos[0] < areaMin[0] || pos[0] > areaMax[0] || pos[2] < areaMin[2] || pos[2] > areaMax[2]) {
        string ammoType = "MeleeSlash";
        player.ProcessDirectDamage(DT_CUSTOM, player, "", ammoType, "0 0 0", 5.0);
        player.MessageImportant("VOCÊ SAIU DA ZONA SEGURA, VOLTE IMEDIATAMENTE POIS SUA VIDA IRÁ REDUZIR!");

        WriteToLog("Jogador " + player.GetIdentity().GetName() + " saiu da zona segura. Posição: " + pos.ToString());
    }
}
