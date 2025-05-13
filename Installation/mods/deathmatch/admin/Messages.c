array<string> CheckMessages()
{
    array<string> msgs = new array<string>();

    string path = "$mission:messages_to_send.txt";
    FileHandle file = OpenFile(path, FileMode.READ);
    if (file == 0) {
        return msgs;
    }

    string line;
    
    while (FGets(file, line) > 0)
    {
        line = line.Trim();
        if (line != "") {				
            msgs.Insert(line);
        }
    }		

    CloseFile(file);
    FileHandle clearFile = OpenFile(path, FileMode.WRITE);
    if (clearFile != 0)
        CloseFile(clearFile); // abrir em modo WRITE já limpa o conteúdo
    
    return msgs;
}

void AppendMessage(string message)
{
    if (message == "")
        return;

    string path = "$mission:messages_to_send.txt";
    FileHandle file = OpenFile(path, FileMode.APPEND);

    if (file != 0)
    {
        FPrintln(file, message);
        CloseFile(file);
    }
    else
    {
        WriteToLog("Erro ao abrir o arquivo para append: " + path);
    }
}