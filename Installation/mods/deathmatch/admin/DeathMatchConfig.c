ref SafeZoneData LoadActiveRegionData(string path)
{
	WriteToLog("Carregando arquivo JSON: " + path);

	ref array<ref SafeZoneData> list;
	JsonFileLoader<array<ref SafeZoneData>>.JsonLoadFile(path, list);

	foreach (ref SafeZoneData data : list) {
		if (data && data.Active) {
			WriteToLog("Região ativa encontrada:");
			WriteToLog("Region: " + data.Region);
			WriteToLog("Mensagem personalizada: " + data.CustomMessage);
			WriteToLog("SpawnZones: " + data.SpawnZones.Count().ToString());
			WriteToLog("WallZones: " + data.WallZones.Count().ToString());            

			return data;
		}
	}

	WriteToLog("Nenhuma região ativa encontrada.");
	return null;
}

void ToggleActiveRegion(string path)
{
    WriteToLog("Carregando JSON de regiões: " + path);

    ref array<ref SafeZoneData> zones;
    JsonFileLoader<array<ref SafeZoneData>>.JsonLoadFile(path, zones);

    int activeIndex = -1;
    for (int i = 0; i < zones.Count(); i++) {
        if (zones[i].Active) {
            zones[i].Active = false;
            activeIndex = i;
            break;
        }
    }

    int nextIndex = (activeIndex + 1) % zones.Count(); // loop circular
    zones[nextIndex].Active = true;

    JsonFileLoader<array<ref SafeZoneData>>.JsonSaveFile(path, zones);

    WriteToLog("Região ativa alterada para: " + zones[nextIndex].Region);
}

void SetActiveRegionById(string path, int regionId)
{
    WriteToLog("Carregando JSON de regiões: " + path);

    ref array<ref SafeZoneData> zones;
    JsonFileLoader<array<ref SafeZoneData>>.JsonLoadFile(path, zones);

    bool found = false;

    for (int i = 0; i < zones.Count(); i++) {
        if (zones[i].RegionId == regionId) {
            zones[i].Active = true;
            found = true;
        } else {
            zones[i].Active = false;
        }
    }

    if (found) {
        JsonFileLoader<array<ref SafeZoneData>>.JsonSaveFile(path, zones);
        WriteToLog("Região com RegionId " + regionId.ToString() + " foi marcada como ativa.");
    } else {
        WriteToLog("RegionId " + regionId.ToString() + " não encontrado no arquivo.");
    }
}



void ExtractVectorArray(string json, string key, out array<vector> output)
{
    output = new array<vector>();

    int idx = json.IndexOf(key);
    if (idx == -1) return;

    string sub = json.Substring(idx, json.Length() - idx);
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

vector GetRandomSafeSpawnPosition(array<vector> spawnZones)
{
    if (spawnZones.Count() == 0) {
        WriteToLog("Nenhuma zona segura disponível para spawn.");
        return "0 0 0";
    }

    int index = Math.RandomInt(0, spawnZones.Count());
    WriteToLog("Posição segura selecionada: " + spawnZones[index].ToString());
    return spawnZones[index];
}

bool IsInsidePolygon(vector point, array<vector> polygon)
{
	if (polygon.Count() < 3)
		return false;

	bool inside = false;

	for (int i = 0; i < polygon.Count(); i++)
	{
		int j;
		if (i == 0)
			j = polygon.Count() - 1;
		else
			j = i - 1;

		vector pi = polygon[i];
		vector pj = polygon[j];

		if (((pi[2] > point[2]) != (pj[2] > point[2])) && (point[0] < (pj[0] - pi[0]) * (point[2] - pi[2]) / ((pj[2] - pi[2]) + 0.0001) + pi[0]))
		{
			inside = !inside;
		}
	}

	return inside;
}


void CheckPlayerAreaPolygonal(PlayerBase player, array<vector> wallZones)
{
	if (!player || wallZones.Count() < 3)
		return;

	vector pos = player.GetPosition();

	bool inside = IsInsidePolygon(pos, wallZones);

	if (!inside)
	{
		float damage = 50.0;
		player.DecreaseHealth("GlobalHealth", "Health", damage);
		WriteToLog("[SAFEZONE] Jogador " + player.GetIdentity().GetName() + " saiu da zona segura.");
	}
}
