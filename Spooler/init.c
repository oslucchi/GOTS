/* Global includes */
#include	<stdio.h>
#include	<signal.h>
#include	<errno.h>
#include	<string.h>
#include	<sys/types.h>
#include	<dirent.h>
#include	<unistd.h>
#include	<netdb.h>
#include	<stdlib.h>
#include	<sys/shm.h>
#include	<sys/stat.h>

/* Local module includes */
#include	"gotspool.h"
#include	"debug.h"
#include	"cfgmng.h"
#include	"libshm.h"

/* Costants */
#define		ESC			((char) 27)

/* Macroes */               

/* Local struct & new type definition */

/* External Functions */
extern	char	gacHomeDir[];
extern	char	gacGCUXHome[];
extern	char	gcFlgReportQueued;

/* External Variables */

/* Global Variables */

/* Local Variables */
static	_strPrinterItem	strPrintItem;
static	_strPrinterItem	*pstrPrintItem;
static	_strReportItem	strReportItem;
static	_strReportItem	*pstrReportItem;
static	_strPaperItem	astrPaperItem[MAX_PAPER_DEFINED];



/*
 *
 *	Function:		LoadPapers
 *
 *	Description:	Carica i tipi carta
 *
 *	Parameters:		nessuno
 *
 *	Returns:		void
 *
 */
void	LoadPapers()
{
	FILE	*pfCfg;
	char	acFilePath[1024];
	char	acPaperPath[1024];
	struct	dirent	*ptrDirent;
	int		iCount = 0;
	DIR		*pDir;

	Debug(( DBG_L_START_F, "LoadPapers: Inizio Funzione\n" ));

	memset(&astrPaperItem[0], '\0', sizeof(astrPaperItem));
	sprintf(acPaperPath, "%s/papers", gacGCUXHome);
	if ((pDir = opendir(acPaperPath)) == NULL)
	{
		Debug(( DBG_L_FATAL,
				"LoadPapers: Impossibile caricare carta da '%s'. Errore %d\n",
				acPaperPath, errno ));
		return;
	}

	while((ptrDirent = readdir(pDir)) != NULL)
	{
		if (iCount >= MAX_PAPER_DEFINED)
		{
			Debug(( DBG_L_FATAL,
					"LoadPapers: Troppi formati carta definiti\n" ));
			closedir(pDir);
			return;
		}

		if (ptrDirent->d_name[0] == '.')
			continue;
		
		sprintf(acFilePath, "%s/%s", acPaperPath, ptrDirent->d_name);

		if ((pfCfg = fopen(acFilePath, "r")) == NULL)
		{
			Debug(( DBG_L_FATAL,
					"LoadPapers: Impossibile aprire '%s'. Errore %d\n",
					errno ));
			fclose(pfCfg);
			continue;
		}

		if (CfgGetFile(pfCfg) < 0)
		{
			Debug(( DBG_L_FATAL,
					"LoadPapers: Impossibile caricare '%s'. Errore %d\n",
					errno ));
			fclose(pfCfg);
			continue;
		}
		fclose(pfCfg);

		strcpy(astrPaperItem[iCount].acPaperName, 
				CfgGetValue(NULL, "NAME" ));
		astrPaperItem[iCount].iNumOfLines = 
							atoi(CfgGetValue(NULL, "LINES" ));
		strcpy(astrPaperItem[iCount].acLinesPerInch, 
				CfgGetValue(NULL, "LPITYPE" ));
		strcpy(astrPaperItem[iCount].acReset, 
				CfgGetValue(NULL, "RESET" ));
		astrPaperItem[iCount].iInchsPerPage = 
							atoi(CfgGetValue(NULL, "INCHS" ));

		Debug(( DBG_L_TRACE,
				"LoadPapers: caricata '%s' lin %d lpi '%s' rst '%s' ipp %d\n",
				astrPaperItem[iCount].acPaperName,
				astrPaperItem[iCount].iNumOfLines,
				astrPaperItem[iCount].acLinesPerInch,
				astrPaperItem[iCount].acReset,
				astrPaperItem[iCount].iInchsPerPage ));
		CfgUngetFile();
		iCount++;
	}

	closedir(pDir);
	Debug(( DBG_L_STOP_F, "LoadPapers: Fine Funzione\n" ));
	return;
}


/*
 *
 *	Function:		LpGetStartSequence
 *
 *	Description:	Valorizza la variabile passata per parametro con la 
 *					sequenza di escape per la stampante / carta
 *
 *	Parameters:		pointer al buffer per la sequenza, nome della stampante
 *					nome della carta
 *
 *	Returns:		void
 *
 */
void	LpGetStartSequence(char *pcStartSequence, 
							char *pcPrinter, char *pcPaper, int *piSeqLen)
{
	int		iPaperIdx;
	int		iPrinterIdx;
	char	*pcLPISeq;
	_strPrinterItem	*pstrPrinterItem;

	Debug(( DBG_L_START_F, "LpGetStartSequence: Inizio Funzione\n" ));

	*piSeqLen = 0;

	Debug(( DBG_L_DUMP_V, "LpGetStartSequence: cerco sequenza per %s/%s\n",
			pcPrinter, pcPaper ));
	for(iPaperIdx = 0; iPaperIdx < MAX_PAPER_DEFINED; iPaperIdx++)
	{
		if (strcmp(astrPaperItem[iPaperIdx].acPaperName, pcPaper) == 0)
		{
			break;
		}
	}

	if (iPaperIdx == MAX_PAPER_DEFINED)
	{
		Debug(( DBG_L_DUMP_V,
				"LpGetStartSequence: carta %s NON TROVATA\n", pcPaper ));
		*pcStartSequence = '\0';
		return;
	}

	if ((pstrPrinterItem = SearchPrinter(pcPrinter, &iPrinterIdx)) == NULL)
	{
		Debug(( DBG_L_DUMP_V,
				"LpGetStartSequence: stampante %s NON TROVATA\n", pcPrinter ));
		*pcStartSequence = '\0';
		return;
	}

	if (strcmp(astrPaperItem[iPaperIdx].acLinesPerInch, "LPI8") == 0)
	{
		pcLPISeq = pstrPrinterItem->acLPI8;
		Debug(( DBG_L_TRACE, "LpGetStartSequence: la carta e' LPI8 (%s)\n",
				pcLPISeq ));
	}
	else if (strcmp(astrPaperItem[iPaperIdx].acLinesPerInch, "LPI6") == 0)
	{
		pcLPISeq = pstrPrinterItem->acLPI6;
		Debug(( DBG_L_TRACE, "LpGetStartSequence: la carta e' LPI6 (%s)\n",
				pcLPISeq ));
	}
	else
	{
		Debug(( DBG_L_TRACE,
				"LpGetStartSequence: la carta e' default (2)\n" ));
		pcLPISeq = "2";
	}
	Debug(( DBG_L_TRACE,
			"LpGetStartSequence: pcLPISeq = '%s'\n", pcLPISeq ));

	if (astrPaperItem[iPaperIdx].iInchsPerPage == 0)
	{
		sprintf(pcStartSequence, "%cC%c%c%s",
				 ESC, (char) astrPaperItem[iPaperIdx].iNumOfLines,
				 ESC, pcLPISeq);
		Debug(( DBG_L_DUMP_V,
				"LpGetStartSequence: sequenza per %s/%s "
				"'ESC C %d ESC %s'\n",
				pcPaper, pcPrinter, 
				astrPaperItem[iPaperIdx].iNumOfLines, pcLPISeq ));
		*piSeqLen = strlen(pcStartSequence);
	}
	else
	{
		sprintf(pcStartSequence, "%cC%c%c%s%cC%c%c",
				 ESC, (char) astrPaperItem[iPaperIdx].iNumOfLines,
				 ESC, pcLPISeq,
				 ESC, (char) 0, astrPaperItem[iPaperIdx].iInchsPerPage);
		Debug(( DBG_L_DUMP_V,
				"LpGetStartSequence: sequenza per %s/%s "
				"'ESC C %d ESC %s ESC C 0 %d'\n",
				pcPaper, pcPrinter, 
				astrPaperItem[iPaperIdx].iNumOfLines,
				pcLPISeq, astrPaperItem[iPaperIdx].iInchsPerPage ));
		*piSeqLen = strlen(pcStartSequence) + 2;
	}

	Debug(( DBG_L_STOP_F, "LpGetStartSequence: Fine Funzione\n" ));
	return;
}


/*
 *
 *	Function:		LoadReport
 *
 *	Description:	Carica il contenuto di un file di parametri per il report
 *
 *	Parameters:		path del report da aprire
 *
 *	Returns:		void
 *
 */
void	LoadReport(char *pcReportPath)
{
	FILE	*pfCfg;
	struct	stat	strStat;


	Debug(( DBG_L_DUMP_V,
			"LoadReport: Loading report .... %s\n", pcReportPath ));

	if (stat(pcReportPath, &strStat) != 0)
	{
		strReportItem.lCreationTime = 0L;
	}
	else
	{
		strReportItem.lCreationTime = (long) strStat.st_mtime;
	}

	if ((pfCfg = fopen(pcReportPath, "r")) == NULL)
	{
		exit(EXIT_REPORTFILE_CANT_OPEN);
	}
	
	if (CfgGetFile(pfCfg) < 0)
	{
		exit(EXIT_BAD_REPORT_FILE);
	}
	fclose(pfCfg);

	Debug(( DBG_L_TRACE, "LoadReport: Inizio load ...\n" ));


	Debug(( DBG_L_DUMP_V,
			"LoadReport: USER %s\n", CfgGetValue(NULL, "USER") ));
	strcpy(strReportItem.acUserName,
			CfgGetValue(NULL, "USER"));

	Debug(( DBG_L_DUMP_V, 
			"LoadReport: DATA_FILE %s\n", CfgGetValue(NULL, "DATA_FILE") ));
	strcpy(strReportItem.acReportPath,
			CfgGetValue(NULL, "DATA_FILE"));

	Debug(( DBG_L_DUMP_V, "LoadReport: REPORT_NAME %s\n", 
			CfgGetValue(NULL, "REPORT_NAME") ));
	strcpy(strReportItem.acReportName, 
			CfgGetValue(NULL, "REPORT_NAME"));

	Debug(( DBG_L_DUMP_V, "LoadReport: PAPER_FORMAT %s\n", 
			CfgGetValue(NULL, "PAPER_FORMAT") ));
	strcpy(strReportItem.acPaper, 
			CfgGetValue(NULL, "PAPER_FORMAT"));

	Debug(( DBG_L_DUMP_V, "LoadReport: PRINTER_DEVICE %s\n", 
			CfgGetValue(NULL, "PRINTER_DEVICE") ));
	strcpy(strReportItem.acDevice, 
			CfgGetValue(NULL, "PRINTER_DEVICE"));

	Debug(( DBG_L_DUMP_V, "LoadReport: SHEET_OPTION %s\n", 
			CfgGetValue(NULL, "SHEET_OPTION") ));
	strReportItem.sSheetOpt = atoi(CfgGetValue(NULL, "SHEET_OPTION"));

	Debug(( DBG_L_DUMP_V, "LoadReport: SKIP_OPTION %s\n", 
			CfgGetValue(NULL, "SKIP_OPTION") ));
	strReportItem.sSkipOpt = atoi(CfgGetValue(NULL, "SKIP_OPTION"));


	Debug(( DBG_L_DUMP_V, "LoadReport: DISP_OPTION %s\n", 
			CfgGetValue(NULL, "DISP_OPTION") ));

	if (strcmp(CfgGetValue(NULL, "DISP_OPTION" ), "HOLD") == 0)
	{
		strReportItem.sDispoOpt = GCOS_DISPO_OPTION_HOLD;
	}
	else if (strcmp(CfgGetValue(NULL, "DISP_OPTION" ), "KEEP") == 0)
	{
		strReportItem.sDispoOpt = GCOS_DISPO_OPTION_KEEP;
	}
	else if (strcmp(CfgGetValue(NULL, "DISP_OPTION" ), "DELETE") == 0)
	{
		strReportItem.sDispoOpt = GCOS_DISPO_OPTION_DELE;
	}
	else
	{
		strReportItem.sDispoOpt = GCOS_DISPO_OPTION_DELE;
	}

	Debug(( DBG_L_DUMP_V, "LoadReport: NUM_OF_COPIES %s\n", 
			CfgGetValue(NULL, "NUM_OF_COPIES") ));
	strReportItem.sNumOfCopies = atoi(CfgGetValue(NULL, "NUM_OF_COPIES"));

	Debug(( DBG_L_DUMP_V, 
			"LoadReport: JOBNAME %s\n", CfgGetValue(NULL, "JOBNAME") ));
	strcpy(strReportItem.acJobName, CfgGetValue(NULL, "JOBNAME"));

	Debug(( DBG_L_DUMP_V, 
			"LoadReport: JOBSON %s\n", CfgGetValue(NULL, "JOBSON") ));
	strcpy(strReportItem.acJobSon, CfgGetValue(NULL, "JOBSON"));

	Debug(( DBG_L_DUMP_V, 
			"LoadReport: JOBPID %s\n", CfgGetValue(NULL, "JOBPID") ));
	strcpy(strReportItem.acJobNum, CfgGetValue(NULL, "JOBPID"));

	if (CfgGetValue(NULL, "UNIX_ID") == NULL)
	{
		Debug(( DBG_L_TRACE,
				"LoadReport: UNIX_ID  mancante impostato a 0\n" ));
		strcpy(strReportItem.acUnixId, "0");
	}
	else
	{
		Debug(( DBG_L_DUMP_V, "LoadReport: UNIX_ID %s\n", 
				CfgGetValue(NULL, "UNIX_ID") ));
		strcpy(strReportItem.acUnixId, CfgGetValue(NULL, "UNIX_ID"));
	}

	if (CfgGetValue(NULL, "LINES_IN_REPO") == NULL)
	{
		Debug(( DBG_L_TRACE,
				"LoadReport: LINES_IN_REPO mancante impostato a 0\n" ));
		strReportItem.iLines = 0;
	}
	else
	{
		Debug(( DBG_L_DUMP_V, "LoadReport: LINES_IN_REPO %s\n", 
				CfgGetValue(NULL, "LINES_IN_REPO") ));
		strReportItem.iLines = atoi( CfgGetValue(NULL, "LINES_IN_REPO"));
	}
	CfgUngetFile();

}


/*
 *
 *	Function:		LoadPrinter
 *
 *	Description:	Carica i parametri contenuti nel file descrittivo dell
 *					stampanti
 *
 *	Parameters:		Path del file stampante da caricare
 *
 *	Returns:		void
 *
 */
void	LoadPrinter(char *pcPrinterPath)
{
	char	acHostName[64];
	struct	hostent	*pstrHE;
	FILE	*pfCfg;

	Debug(( DBG_L_TRACE, "LoadPrinter: stampante .... %s\n", pcPrinterPath ));

	if ((pfCfg = fopen(pcPrinterPath, "r")) == NULL)
	{
		Debug(( DBG_L_FATAL,
				"LoadPrinter: errore %d in apertura file %s\n",
				errno, pcPrinterPath ));
		exit(EXIT_PRINTERFILE_CANT_OPEN);
	}

	if (CfgGetFile(pfCfg) < 0)
	{
		Debug(( DBG_L_FATAL,
				"LoadPrinter: Formato file %s non corretto.\n",
				pcPrinterPath ));
		exit(EXIT_BAD_PRINTER_FILE);
	}
	fclose(pfCfg);

	Debug(( DBG_L_TRACE, "LoadPrinter: Inizio caricamento\n" ));

	Debug(( DBG_L_TRACE, "LoadPrinter: PAPER set to ZZZZ\n" ));
	strcpy(strPrintItem.acCurrentPaper, "ZZZZ");

	Debug(( DBG_L_DUMP_V,
			"LoadPrinter: DEVICE %s\n", CfgGetValue(NULL, "DEVICE") ));
	strcpy(strPrintItem.acUnixDevice, CfgGetValue(NULL, "DEVICE"));

	Debug(( DBG_L_DUMP_V,
			"LoadPrinter: LP_DEVICE %s\n", CfgGetValue(NULL, "LP_DEVICE") ));
	strcpy(strPrintItem.acUnixLpName, CfgGetValue(NULL, "LP_DEVICE"));

	Debug(( DBG_L_DUMP_V, "LoadPrinter: DESCRIPTION %s\n",
			CfgGetValue(NULL, "DESCRIPTION") ));
	strcpy(strPrintItem.acDescription, CfgGetValue(NULL, "DESCRIPTION"));

	Debug(( DBG_L_DUMP_V, "LoadPrinter: MASTER %s\n",
			CfgGetValue(NULL, "MASTER") ));
	strcpy(strPrintItem.acMasterStation, CfgGetValue(NULL, "MASTER"));

	Debug(( DBG_L_DUMP_V, "LoadPrinter: LPI6 %s\n",
			CfgGetValue(NULL, "LPI6") ));
	strcpy(strPrintItem.acLPI6, CfgGetValue(NULL, "LPI6"));

	Debug(( DBG_L_DUMP_V, "LoadPrinter: LPI8 %s\n",
			CfgGetValue(NULL, "LPI8") ));
	strcpy(strPrintItem.acLPI8, CfgGetValue(NULL, "LPI8"));

	Debug(( DBG_L_DUMP_V, "LoadPrinter: LPDMASTER %s\n",
			CfgGetValue(NULL, "LPDMASTER") ));

	strcpy(strPrintItem.acLpdMaster, CfgGetValue(NULL, "LPDMASTER"));
	strcpy(acHostName, CfgGetValue(NULL, "LPDMASTER"));

	if ((pstrHE = gethostbyname(acHostName)) == NULL)
	{
		strPrintItem.lIpAddress = 0L;
		strcpy(strPrintItem.acLpdMaster, "");
	}
	else
	{
		memcpy(&strPrintItem.lIpAddress, 
				&(pstrHE->h_addr[0]), sizeof(long));
		strcpy(strPrintItem.acLpdMaster, CfgGetValue(NULL, "LPDMASTER"));
	}

	Debug(( DBG_L_TRACE, "LoadPrinter: Caricamento terminato\n" ));

	strcpy(strPrintItem.acPrintingRpt, "");
	strcpy(strPrintItem.acPrintingRptJ, "");
	strcpy(strPrintItem.acPrintingRptS, "");


	CfgUngetFile();
}


/*
 *
 *	Function:		InitializePrintQueues
 *
 *	Description:	Inizializza le code per le stampanti
 *
 *	Parameters:		
 *
 *	Returns:		void
 *
 */
void	InitializePrintQueues()
{
	int		iResult;
	char	acPrintersPath[1024];
	char	acFilePath[1024];
	DIR		*pDir;
	struct	dirent	*ptrDirent;

	/*
	 * Le stampanti avranno sull'array offset da 1.
	 * La stampante a offset 0 sara' per default quella a cui assegnare i
	 * report che si riferiscono ad una PRT non esistente.
	 */

	/*
	 * Assegno la PRT di default per i report con PRT inesistente.
	 */
	strcpy(strPrintItem.acPrinterName, "ERRPRT");
	strcpy(strPrintItem.acCurrentPaper, "");
	strcpy(strPrintItem.acUnixDevice, "");
	strcpy(strPrintItem.acUnixLpName, "");
	strcpy(strPrintItem.acDescription, "Report per stampanti inesistenti");
	strcpy(strPrintItem.acCurrentPaper, "");
	strcpy(strPrintItem.acPrintingRpt, "");
	strcpy(strPrintItem.acPrintingRptJ, "");
	strcpy(strPrintItem.acPrintingRptS, "");
	iResult = AddPrinter(&strPrintItem);
	Debug(( DBG_L_DUMP_V, 
			"InitializePrintQueues: AddPrinter di '%s' ha ritornato '%d'\n",
			strPrintItem.acPrinterName,
			iResult ));

	sprintf(acPrintersPath, "%s/printers", gacHomeDir);
	if ((pDir = opendir(acPrintersPath)) == NULL)
	{
		// Unable to open printers dir
		Debug(( DBG_L_FATAL,
				"InitializePrintQueues: impossibile aprire %s (%d)\n",
				acPrintersPath, errno ));
		exit(EXIT_BAD_OPENDIR_PRINTERS);
	}

	while((ptrDirent = readdir(pDir)) != NULL)
	{
		/*
		 * Exclude . ..  and all .* namesfrom load actions
		 */
		if (ptrDirent->d_name[0] == '.')
			continue;

		sprintf(acFilePath, "%s/%s", acPrintersPath, ptrDirent->d_name);

		strcpy(strPrintItem.acPrinterName, ptrDirent->d_name);

		LoadPrinter(acFilePath);

		AddPrinter(&strPrintItem);
		Debug(( DBG_L_DUMP_V, 
				"InitializePrintQueues: AddPrinter di '%s' ha ritornato '%d'\n",
				strPrintItem.acPrinterName,
				iResult ));
	}
	closedir(pDir);
}


/*
 *
 *	Function:		RenameReports
 *
 *	Description:	Ripristina l'estensione PAR ai file di parametri LDD
 *
 *	Parameters:		
 *
 *	Returns:		void
 *
 */
void	RenameReports()
{
	DIR		*pDir;
	char	acReportPath[1024];
	char	acReportOldName[1024];
	char	acReportNewName[1024];
	char	*pcToExt;
	struct	dirent	*ptrDirent;

	sprintf(acReportPath, "%s/spool", gacHomeDir);
	if ((pDir = opendir(acReportPath)) == NULL)
	{
		exit(EXIT_BAD_OPENDIR_REPORTS);
	}

	while((ptrDirent = readdir(pDir)) != NULL)
	{
		if (ptrDirent->d_name[0] == '.')
			continue;
		
		if ((pcToExt = strstr(ptrDirent->d_name, "cfA")) != NULL)
		{
			sprintf(acReportNewName, "%s/%s",
					 acReportPath, ptrDirent->d_name);
			remove(acReportNewName);
			continue;
		}

		if ((pcToExt = strstr(ptrDirent->d_name, ".LDD")) == NULL)
			continue;
		
		*pcToExt = '\0';
		memset(acReportOldName, '\0', sizeof(acReportOldName));
		sprintf(acReportOldName,
				 "%s/%s.LDD", acReportPath, ptrDirent->d_name);

		memset(acReportNewName, '\0', sizeof(acReportNewName));
		sprintf(acReportNewName,
				 "%s/%s.PAR", acReportPath, ptrDirent->d_name);

		rename(acReportOldName, acReportNewName);
	}
	closedir(pDir);
}


/*
 *
 *	Function:		LoadWaitingReports
 *
 *	Description:	Carica i report che sono in attesa nella directory di spool
 *
 *	Parameters:		
 *
 *	Returns:		void
 *
 */
void	LoadWaitingReports(char cFlgJustStarted)
{
	int		iRptCount = 0;
	int		iCount = 0;
	char	acReportPath[1024];
	char	acFilePath[1024];
	char	acReportBaseName[1024];
	char	*pcToExt;
	DIR		*pDir;
	struct	dirent	*ptrDirent;

	/*
	 * Scansione del buffer report.
	 */
	siginterrupt(SIGALRM, 0);

	sprintf(acReportPath, "%s/spool", gacHomeDir);
	Debug(( DBG_L_DUMP_V,
			"LoadWaitingReports: carico report da '%s'\n", acReportPath ));
	if ((pDir = opendir(acReportPath)) == NULL)
	{
		Debug(( DBG_L_FATAL,
				"LoadWaitingReports: errore %d in opendir\n", errno ));
		exit(EXIT_BAD_OPENDIR_REPORTS);
	}

	while((ptrDirent = readdir(pDir)) != NULL)
	{
		/*
		 * Exclude . ..  and all ! (*.PAR || *.LDD) names from load actions
		 */
		if (ptrDirent->d_name[0] == '.')
			continue;
		
		Debug(( DBG_L_DUMP_V,
				"LoadWaitingReports: letta entry '%s'\n", ptrDirent->d_name ));
		if ((pcToExt = strstr(ptrDirent->d_name, ".PAR")) == NULL)
			continue;
		
		memset(acReportBaseName, '\0', sizeof(acReportBaseName));
		sprintf(acReportBaseName, "%s/", acReportPath);
		strncat(acReportBaseName, ptrDirent->d_name, 
										 pcToExt - ptrDirent->d_name);

		sprintf(acFilePath, "%s/%s", acReportPath, ptrDirent->d_name);

		memset(&strReportItem, '\0', sizeof(_strReportItem));

		LoadReport(acFilePath);

		/* 
		 * 26 giugno 2001.
		 * I report caricati in fase di start-up hanno stato HOLD
		 */
		if (cFlgJustStarted)
		{
			strReportItem.sDispoOpt = GCOS_DISPO_OPTION_HOLD;
			strcpy(strReportItem.acUnixId, "0");
		}

		iRptCount++;

		/*
		 * assegno il nome del file di paramentri
		 */
		strcat(acReportBaseName, ".LDD");
		strcpy(strReportItem.acCfgFilePath, acReportBaseName);

		if ((iCount = AddReport(&strReportItem, 
								  strReportItem.acDevice)) < 0)
		{
			Debug(( DBG_L_TRACE, "LoadWaitingReports: errore durante load\n" ));
			continue;
		}
		Debug(( DBG_L_DUMP_V,
				"LoadWaitingReports: Caricato report %d (coda su %s %d)\n",
				iRptCount, strReportItem.acDevice, iCount ));

		rename(acFilePath, acReportBaseName);

		gcFlgReportQueued = TRUE;
	}
	closedir(pDir);
	siginterrupt(SIGALRM, 1);
}
