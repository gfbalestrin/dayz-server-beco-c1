void WriteToLog(string content, string logfile = "init.log")
{
	string fileName = "$profile:" + logfile; // Caminho dentro da pasta do servidor
	FileHandle file = OpenFile(fileName, FileMode.APPEND);

	if (file != 0)
	{
		FPrintln(file, content); // Escreve a string com quebra de linha
		CloseFile(file);
	}
	else
	{
		WriteToLog("Erro ao abrir o arquivo para escrita.", logfile);
	}
}