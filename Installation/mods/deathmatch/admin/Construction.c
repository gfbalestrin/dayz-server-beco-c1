void CreateConstruction(PlayerBase player, string objectName)
{
    if (!player) return;

    vector playerPos = player.GetPosition();
    vector orientation = player.GetOrientation();

    float angle = orientation[0];
    float distance = 2.0;

    float rad = angle * Math.DEG2RAD;
    vector forward = Vector(Math.Sin(rad), 0, Math.Cos(rad));

    vector spawnPos = playerPos + (forward * distance);

    // Troque por um objeto garantido
    Object wall = GetGame().CreateObject(objectName, spawnPos, false, false);

    if (!wall)
    {
        WriteToLog("ERRO: Objeto não foi criado!");
        player.MessageStatus("ERRO: Objeto não foi criado!");
    }        
    else{
        WriteToLog("SUCESSO: Objeto criado na posição: " + spawnPos);
        player.MessageStatus("SUCESSO: Objeto criado na posição: " + spawnPos);
    }
        
}
void CreatePrisonWall(PlayerBase player)
{
    if (!player) return;

    vector basePos = player.GetPosition();
    float playerAngle = player.GetOrientation()[0];      // Direção que o jogador olha
    float wallLength = 12.0;
    int wallCount = 4;

    float rad = playerAngle * Math.DEG2RAD;
    vector forward = Vector(Math.Sin(rad), 0, Math.Cos(rad)); // Direção para frente

    float wallAngle = playerAngle + 90; // Gira cada muro de lado

    for (int i = 0; i < wallCount; i++)
    {
        vector offset = forward * (i * wallLength);
        vector spawnPos = basePos + offset;

        // Corrige altura para alinhar com o terreno
        float groundY = GetGame().SurfaceY(spawnPos[0], spawnPos[2]);
        spawnPos[1] = groundY;

        Object wall = GetGame().CreateObject("Land_Prison_Wall_Large", spawnPos, false, true);
        if (wall)
        {
            wall.SetPosition(spawnPos);
            wall.SetOrientation(Vector(wallAngle, 0, 0)); // Gira o muro 90°
            WriteToLog("SUCESSO: Objeto criado na posição: " + spawnPos);
            player.MessageStatus("SUCESSO: Objeto criado na posição: " + spawnPos);
        }
        else
        {
            WriteToLog("ERRO: Falha ao criar muro na posição " + spawnPos);
            player.MessageStatus("ERRO: Falha ao criar muro na posição " + spawnPos);
        }
    }
}


void CreateCustomObject(PlayerBase player, string buildName, float heightOffset = 1.0, int containerCount = 4, float containerLength = 6.0, float rotationOffset = 0.0)
{
    if (!player) return;
    if (buildName == "") {
        player.MessageStatus("ERRO: Nome de objeto inválido.");
        return;
    }

    vector basePos = player.GetPosition();
    float playerAngle = player.GetOrientation()[0];
    float finalAngle = playerAngle + rotationOffset;

    float rad = playerAngle * Math.DEG2RAD;
    vector forward = Vector(Math.Sin(rad), 0, Math.Cos(rad)); // Direção de avanço

    for (int i = 0; i < containerCount; i++)
    {
        vector offset = forward * (i * containerLength);
        vector spawnPos = basePos + offset;

        // Corrige altura com SurfaceY + offset vertical
        float groundY = GetGame().SurfaceY(spawnPos[0], spawnPos[2]);
        spawnPos[1] = groundY + heightOffset;

        Object obj = GetGame().CreateObject(buildName, spawnPos, false, true);
        if (obj)
        {
            obj.SetPosition(spawnPos);
            obj.SetOrientation(Vector(finalAngle, 0, 0)); // Aplica rotação parametrizada
            WriteToLog("SUCESSO: Objeto " + buildName + " criado em " + spawnPos);
            player.MessageStatus("SUCESSO: Objeto " + buildName + " criado na posição: " + spawnPos);
        }
        else
        {
            WriteToLog("ERRO: Falha ao criar " + buildName + " em " + spawnPos);
            player.MessageStatus("ERRO: Falha ao criar " + buildName + " em " + spawnPos);
        }
    }
}

// Cria objetos ao longo de uma linha entre dois pontos
void CreateObjectsAlongLine(vector startPos, vector endPos, string objectName, float spacing, float heightOffset)
{
    vector direction = endPos - startPos;
    float length = direction.Length();
    int count = Math.Floor(length / spacing);
    direction.Normalize();

    float angle = Math.Atan2(direction[0], direction[2]) * Math.RAD2DEG;

    for (int i = 0; i <= count; i++)
    {
        vector pos = startPos + (direction * (i * spacing));
        pos[1] = GetGame().SurfaceY(pos[0], pos[2]) + heightOffset;

        Object obj = GetGame().CreateObject(objectName, pos, false, true);
        if (obj)
        {
            obj.SetPosition(pos);
            obj.SetOrientation(Vector(angle, 0, 0));
            if (m_CreatedObjects) m_CreatedObjects.Insert(obj); // rastreia
        }
    }
}


// Cria objetos entre vários pontos sequenciais e fecha o caminho automaticamente
void CreateLinePathFromPoints(array<vector> points, string objectName, float spacing = 6.0, float heightOffset = 1.0)
{
    if (points.Count() < 2) return;

    for (int i = 0; i < points.Count() - 1; i++)
    {
        CreateObjectsAlongLine(points[i], points[i + 1], objectName, spacing, heightOffset);
    }

    // Fecha o polígono ligando o último ao primeiro
    CreateObjectsAlongLine(points[points.Count() - 1], points[0], objectName, spacing, heightOffset);
}








