ref SafeZoneData LoadActiveRegionData(string path)
{
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

    int startObj = content.IndexOf("{");
    while (startObj != -1)
    {
        // Procurar fechamento do objeto
        string rest = content.Substring(startObj, content.Length() - startObj);
        int relEnd = rest.IndexOf("}");
        if (relEnd == -1) break;
        int endObj = startObj + relEnd;

        string objStr = content.Substring(startObj, endObj - startObj + 1);

        int idxActive = objStr.IndexOf("\"Active\":");
        if (idxActive != -1) {
            string boolStr = objStr.Substring(idxActive + 9, 5);
            boolStr.ToLower();
            if (boolStr.Contains("true")) {
                auto data = new SafeZoneData();

                // Region
                int idxRegion = objStr.IndexOf("\"Region\":");
                if (idxRegion != -1) {
                    string subRegion = objStr.Substring(idxRegion + 9, objStr.Length() - idxRegion - 9);
                    int sRelRegion = subRegion.IndexOf("\"") + 1;
                    string subRegion2 = subRegion.Substring(sRelRegion, subRegion.Length() - sRelRegion);
                    int eRelRegion = subRegion2.IndexOf("\"");
                    int sRegion = idxRegion + 9 + sRelRegion;
                    int eRegion = sRegion + eRelRegion;
                    data.regionStr = objStr.Substring(sRegion, eRegion - sRegion);
                }

                // CustomMessage
                int idxCustomMessage = objStr.IndexOf("\"CustomMessage\":");
                if (idxCustomMessage != -1) {
                    string subCustomMessage = objStr.Substring(idxCustomMessage + 16, objStr.Length() - idxCustomMessage - 16);
                    int sRelCustomMessage = subCustomMessage.IndexOf("\"") + 1;
                    string subCustomMessage2 = subCustomMessage.Substring(sRelCustomMessage, subCustomMessage.Length() - sRelCustomMessage);
                    int eRelCustomMessage = subCustomMessage2.IndexOf("\"");
                    int sCustomMessage = idxCustomMessage + 16 + sRelCustomMessage;
                    int eCustomMessage = sCustomMessage + eRelCustomMessage;
                    data.customMessage = objStr.Substring(sCustomMessage, eCustomMessage - sCustomMessage);
                }

                // AreaMin
                int idxMin = objStr.IndexOf("\"AreaMin\":");
                if (idxMin != -1) {
                    string subMin = objStr.Substring(idxMin + 10, objStr.Length() - idxMin - 10);
                    int sRel = subMin.IndexOf("\"") + 1;
                    string subMin2 = subMin.Substring(sRel, subMin.Length() - sRel);
                    int eRel = subMin2.IndexOf("\"");
                    int s = idxMin + 10 + sRel;
                    int e = s + eRel;
                    string minStr = objStr.Substring(s, e - s);
                    data.areaMin = minStr.ToVector();
                }

                // AreaMax
                int idxMax = objStr.IndexOf("\"AreaMax\":");
                if (idxMax != -1) {
                    string subMax = objStr.Substring(idxMax + 10, objStr.Length() - idxMax - 10);
                    int s2Rel = subMax.IndexOf("\"") + 1;
                    string subMax2 = subMax.Substring(s2Rel, subMax.Length() - s2Rel);
                    int e2Rel = subMax2.IndexOf("\"");
                    int s2 = idxMax + 10 + s2Rel;
                    int e2 = s2 + e2Rel;
                    string maxStr = objStr.Substring(s2, e2 - s2);
                    data.areaMax = maxStr.ToVector();
                }

                // SafeZones
                int idxSafe = objStr.IndexOf("\"SafeZones\":");
                if (idxSafe != -1) {
                    string subSafe = objStr.Substring(idxSafe, objStr.Length() - idxSafe);
                    int s3Rel = subSafe.IndexOf("[");
                    int e3Rel = subSafe.IndexOf("]");
                    int s3 = idxSafe + s3Rel;
                    int e3 = idxSafe + e3Rel;
                    string safeBlock = objStr.Substring(s3 + 1, e3 - s3 - 1);

                    array<string> entries = new array<string>();
                    safeBlock.Split(",", entries);

                    for (int i = 0; i + 2 < entries.Count(); i += 3) {
                        string vecStr = entries[i] + "," + entries[i + 1] + "," + entries[i + 2];
                        vecStr.Replace("\"", "");
                        vecStr.Trim();
                        // Criar array para armazenar os valores separados
                        TStringArray parts = new TStringArray();
                        vecStr.Split(",", parts);

                        // Verificar se há exatamente 3 partes
                        if (parts.Count() == 3) {
                            float x = parts[0].Trim().ToFloat();
                            float y = parts[1].Trim().ToFloat();
                            float z = parts[2].Trim().ToFloat();
                            data.safeZones.Insert(Vector(x, y, z));
                        }
                        
                    }
                }

                // WallZones
                int idxWall = objStr.IndexOf("\"WallZones\":");
                if (idxWall != -1) {
                    string subWall = objStr.Substring(idxWall, objStr.Length() - idxWall);
                    int s4Rel = subWall.IndexOf("[");
                    int e4Rel = subWall.IndexOf("]");
                    int s4 = idxWall + s4Rel;
                    int e4 = idxWall + e4Rel;
                    string wallBlock = objStr.Substring(s4 + 1, e4 - s4 - 1);

                    array<string> entries2 = new array<string>();
                    wallBlock.Split(",", entries2);

                    for (int j = 0; j + 2 < entries2.Count(); j += 3) {
                        string vecStr2 = entries2[j] + "," + entries2[j + 1] + "," + entries2[j + 2];
                        vecStr2.Replace("\"", "");
                        vecStr2.Trim();
                        // Criar array para armazenar os valores separados
                        TStringArray parts2 = new TStringArray();
                        vecStr2.Split(",", parts2);

                        // Verificar se há exatamente 3 partes
                        if (parts2.Count() == 3) {
                            float x2 = parts2[0].Trim().ToFloat();
                            float y2 = parts2[1].Trim().ToFloat();
                            float z2 = parts2[2].Trim().ToFloat();
                            data.wallZones.Insert(Vector(x2, y2, z2));
                        }
                        
                    }
                }

                return data;
            }
        }

        // Próximo objeto
        string rem = content.Substring(endObj + 1, content.Length() - endObj - 1);
        int relNext = rem.IndexOf("{");
        if (relNext != -1) {
            startObj = endObj + 1 + relNext;
        } else {
            startObj = -1;
        }
    }


    return null;
}
vector GetRandomSafeSpawnPosition(array<vector> safeZones)
{
    int index = Math.RandomInt(0, safeZones.Count());
    return safeZones[index]; // Retorna a coordenada aleatória
}

void CheckPlayerArea(PlayerBase player, vector areaMin, vector areaMax)
{
    vector pos = player.GetPosition();
    
    // Checa se o jogador está fora da área permitida
    if (pos[0] < areaMin[0] || pos[0] > areaMax[0] || pos[2] < areaMin[2] || pos[2] > areaMax[2])
    {
        // Aplica dano de corte (faz o jogador começar a sangrar)
        string ammoType = "MeleeSlash";
        player.ProcessDirectDamage(DT_CUSTOM, player, "", ammoType, "0 0 0", 5.0);

        player.MessageImportant("VOCÊ SAIU DA ZONA SEGURA, VOLTE IMEDIATAMENTE POIS SUA VIDA IRÁ REDUZIR!");
    } 
}