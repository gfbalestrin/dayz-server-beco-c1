array<string> CheckMessages()
{
    array<string> msgs = new array<string>();
    string path = "$mission:messages_to_send.txt";

    WriteToLog("Verificando mensagens em: " + path);

    FileHandle file = OpenFile(path, FileMode.READ);
    if (file == 0) {
        WriteToLog("Arquivo de mensagens não encontrado ou falha ao abrir: " + path);
        return msgs;
    }

    string line;
    int count = 0;

    while (FGets(file, line) > 0) {
        line = line.Trim();
        if (line != "") {
            msgs.Insert(line);
            count++;
        }
    }

    CloseFile(file);
    WriteToLog("Mensagens lidas: " + count.ToString());

    // Limpa o conteúdo do arquivo após leitura
    FileHandle clearFile = OpenFile(path, FileMode.WRITE);
    if (clearFile != 0) {
        CloseFile(clearFile); // Modo WRITE limpa o arquivo
        WriteToLog("Arquivo de mensagens limpo após leitura.");
    } else {
        WriteToLog("Falha ao limpar o arquivo de mensagens: " + path);
    }

    return msgs;
}

void AppendMessage(string message)
{
    string path = "$mission:messages_to_send.txt";

    if (message == "") {
        WriteToLog("Mensagem vazia não foi adicionada.");
        return;
    }

    FileHandle file = OpenFile(path, FileMode.APPEND);

    if (file != 0) {
        FPrintln(file, message);
        CloseFile(file);
        WriteToLog("Mensagem adicionada: " + message);
    } else {
        WriteToLog("Erro ao abrir o arquivo para append: " + path);
    }
}
