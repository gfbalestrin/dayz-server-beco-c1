void WriteToLog(string content, string logfile = "init.log", bool internalCall = false)
{
	string fileName = "$profile:" + logfile;
	FileHandle file = OpenFile(fileName, FileMode.APPEND);

	if (file != 0)
	{
		int year, month, day, hour, minute;
		GetGame().GetWorld().GetDate(year, month, day, hour, minute);

		string timestamp = "[" + year.ToString() + "-" + month.ToString() + "-" + day.ToString() + " " + hour.ToString() + ":" + minute.ToString() + "]";
		FPrintln(file, timestamp + " " + content);
		CloseFile(file);
	}
	else
	{
		if (!internalCall)
		{
			Print("WriteToLog ERROR: Não foi possível abrir o arquivo de log: " + fileName);
			WriteToLog("Erro ao abrir o arquivo para escrita.", logfile, true);
		}
	}
}
void ResetLog(string logfile = "init.log")
{
	string fileName = "$profile:" + logfile;	
	FileHandle clearFile = OpenFile(fileName, FileMode.WRITE);
    if (clearFile != 0)
        CloseFile(clearFile);
	
	WriteToLog("Arquivo de log resetado... " + fileName);
}
