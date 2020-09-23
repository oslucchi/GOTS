/* Global includes */
#include	<stdio.h>
#include	<errno.h>
#include	<signal.h>
#include	<string.h>
#include	<unistd.h>
#include	<string.h>
#include	<dirent.h>
#include	<time.h>
#include	<stdlib.h>
#include	<ctype.h>

/* Local module includes */
#include	"gotspool.h"
#include	"debug.h"
#include	"libshm.h"
#include	"cfgmng.h"

/* Costants */
#define	MNTPPR			0x01
#define	CHGSTT			0x02
#define	DELRPT			0x03
#define	CHGPRT			0x04
#define	RPTDMP			0x05
#define	REFRESH			0x06
#define	STARTMUL		0x0A
#define	STOPMUL			0x0B

/* Macroes */               

/* Local struct & new type definition */

/* External Functions */
extern	void	SigTrap(int);
extern	void	SendSock(char *pcMsgToSnd);
extern	int		RecvSock(char *pcBuf, int iBufSize);
extern	char	*StringTrimm(char *pcString);
extern	void	CloseReceiveSocket(void);
extern	void	AddClient(int iPid);

/* External Variables */
extern	int		giDbgLevel;
extern	long	glRetValue;
extern	char	*gpcPrgName;
extern	char	gacHomeDir[];
extern	char	gacGCUXHome[];

/* Global Variables */
char	gacUserId[21];

/* Local Variables */
static	_strPaperItem	astrPaperItem[MAX_PAPER_DEFINED];
static	char	cFlagMul;


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
	if ((pDir = opendir(acPaperPath))== NULL)
	{
		Debug(( DBG_L_FATAL,
				"LoadPapers: Impossibile caricare carta da '%s'. Errore %d\n",
				acPaperPath, errno ));
		return;
	}

	while((ptrDirent = readdir(pDir))!= NULL)
	{
		if (iCount >= MAX_PAPER_DEFINED)
		{
			Debug(( DBG_L_FATAL,
					"LoadPapers: Troppi formati carta definiti\n" ));
			return;
		}

		if (ptrDirent->d_name[0] == '.')
			continue;
		
		sprintf(acFilePath, "%s/%s", acPaperPath, ptrDirent->d_name);

		if ((pfCfg = fopen(acFilePath, "r"))== NULL)
		{
			Debug(( DBG_L_FATAL,
					"LoadPapers: Impossibile aprire '%s'. Errore %d\n",
					errno ));
			continue;
		}

		if (CfgGetFile(pfCfg) < 0)
		{
			Debug(( DBG_L_FATAL,
					"LoadPapers: Impossibile caricare '%s'. Errore %d\n",
					errno ));
			continue;
		}
		fclose(pfCfg);

		strcpy(astrPaperItem[iCount].acPaperName, 
				(char *) CfgGetValue(NULL, "NAME"));
		astrPaperItem[iCount].iNumOfLines = 
							atoi(CfgGetValue(NULL, "LINES"));
		Debug(( DBG_L_TRACE,
				"LoadPapers: caricata '%s' linee %d\n",
				astrPaperItem[iCount].acPaperName,
				astrPaperItem[iCount].iNumOfLines ));
		CfgUngetFile();
		iCount++;
	}

	Debug(( DBG_L_STOP_F, "LoadPapers: Fine Funzione\n" ));
	return;
}


/*
 *
 *	Function:		SendReports
 *
 *	Description:	Invia record con i record caricati in SHM al client
 *
 *	Parameters:		nessuno
 *
 *	Returns:		void
 *
 */
void	SendReports()
{
	char	acOutBuf[256];
	_strReportItem	*pstrReportItem;
	_strPrinterItem	*pstrPrintItem;
	struct	tm	*pstrTm;

	Debug(( DBG_L_START_F, "SendReports: Inizio funzione\n" ));

	RestartPrinterList();

	while((pstrPrintItem = GetNextPrinter()) != NULL)
	{
		ResetReportList(pstrPrintItem->acPrinterName);

		while((pstrReportItem = 
					GetNextReport(pstrPrintItem->acPrinterName))!= NULL)
		{
			if (strcmp(gacUserId, "SYSADM") &&
				 strcmp(pstrReportItem->acUserName, gacUserId))
			{
				/*
				 * L'utente proprietario del report e' diverso da quello
				 * collegato.
				 * Non lo invio
				 */
				Debug(( DBG_L_DUMP_V, 
						"SendReports: rpt %s di %s non inviato a %s\n",
						pstrReportItem->acReportName,
						pstrReportItem->acUserName,
						gacUserId ));
				continue;
			}
				
			Debug(( DBG_L_DUMP_V, "SendReports: rpt %s\n",
					pstrReportItem->acReportName ));
			pstrTm = localtime(&(pstrReportItem->lCreationTime));
			sprintf(acOutBuf,
					 "96%-8.8s%-64.64s%-8.8s%-8.8s%-8.8s%-16.16s%-8.8s%-7.7s"\
					 "%-32.32s%c%3.3d%3.3d%3.3d%3.3d%2.2d/%2.2d/%4.4d "\
					 "%2.2d:%2.2d:%2.2d%6.6d%-8.8s",
					 pstrPrintItem->acPrinterName,
					 pstrReportItem->acReportPath,
					 pstrReportItem->acReportName,
					 pstrReportItem->acJobNum,
					 pstrReportItem->acJobSon,
					 pstrReportItem->acUserName,
					 pstrReportItem->acPaper,
					 pstrReportItem->acDevice,
					 pstrReportItem->acUnixId,
					 SPACE(pstrReportItem->cPrio),
					 (int) pstrReportItem->sSheetOpt,
					 (int) pstrReportItem->sSkipOpt,
					 (int) pstrReportItem->sDispoOpt,
					 (int) pstrReportItem->sNumOfCopies,
					 pstrTm->tm_mday,
					 pstrTm->tm_mon + 1,
					 pstrTm->tm_year + 1900,
					 pstrTm->tm_hour,
					 pstrTm->tm_min,
					 pstrTm->tm_sec,
					 pstrReportItem->iLines,
					 pstrReportItem->acJobName);
			SendSock(acOutBuf);
		}
	}

	Debug(( DBG_L_STOP_F, "SendReports: Fine funzione\n" ));
}



/*
 *
 *	Function:		SendPapers
 *
 *	Description:	Invia record con le carte configurate al client
 *
 *	Parameters:		nessuno
 *
 *	Returns:		void
 *
 */
void	SendPapers()
{
	int		iCount = 0;
	char	acOutBuf[80];

	Debug(( DBG_L_START_F, "SendPapers: Inizio funzione\n" ));
	while(astrPaperItem[iCount].acPaperName[0] != '\0')
	{
		sprintf(acOutBuf, "95%-8.8s", astrPaperItem[iCount].acPaperName);
		SendSock(acOutBuf);
		iCount++;
	}
	Debug(( DBG_L_STOP_F, "SendPapers: Fine funzione\n" ));
}



/*
 *
 *	Function:		SendPrinters
 *
 *	Description:	Invia record con le stampanti configurate al client
 *
 *	Parameters:		nessuno
 *
 *	Returns:		void
 *
 */
void	SendPrinters()
{
	char	acOutBuf[256];
	_strPrinterItem	*pstrPrinterItem;

	Debug(( DBG_L_START_F, "SendPrinters: Inizio funzione\n" ));

	RestartPrinterList();

	while((pstrPrinterItem = GetNextPrinter()) != NULL)
	{
		sprintf(acOutBuf, "94%-8.8s%-8.8s%-8.8s%-8.8s%-8.8s%-64.64s",
				 pstrPrinterItem->acPrinterName,
				 pstrPrinterItem->acPrintingRpt,
				 pstrPrinterItem->acPrintingRptJ,
				 pstrPrinterItem->acPrintingRptS,
				 pstrPrinterItem->acCurrentPaper,
				 pstrPrinterItem->acDescription);
		SendSock(acOutBuf);
	}

	Debug(( DBG_L_STOP_F, "SendPrinters: Fine funzione\n" ));
}



/*
 *
 *	Function:		GetFunctionRequired
 *
 *	Description:	Ritorna il codice della funzione richiesta sul messaggio
 *
 *	Parameters:		messaggio ricevuto dal client
 *
 *	Returns:		codice della funzione
 *
 */
int		GetFunctionRequired(char *pcBuf)
{
	char	acFnctCode[3];

	Debug(( DBG_L_START_F, "GetFunctionRequired: Inizio funzione\n" ));

	memset(acFnctCode, '\0', sizeof(acFnctCode));
	strncpy(acFnctCode, pcBuf + 5, 2);

	Debug(( DBG_L_DUMP_V, "GetFunctionRequired: funzione %d\n",
			atoi(acFnctCode) ));

	Debug(( DBG_L_STOP_F, "GetFunctionRequired: Fine funzione\n" ));
	return(atoi(acFnctCode));
}


/*
 *
 *	Function:		DumpReport
 *
 *	Description:	Invia su socket il report richiesto per la visualizzazione
 *
 *	Parameters:		messaggio ricevuto dal client
 *
 *	Returns:		codice della funzione
 *
 */
void	DumpReport(char *pcBuf)
{
	Debug(( DBG_L_START_F, "DumpReport: Inizio funzione\n" ));
	SendSock("90OK");
	Debug(( DBG_L_STOP_F, "DumpReport: Fine funzione\n" ));
}


/*
 *
 *	Function:		GetSpoolPid
 *
 *	Description:	recupera il PID di GCSPOOL per future signal
 *
 *	Parameters:		nessuno
 *
 *	Returns:		process Id
 *
 */
int		GetSpoolPid()
{
	char	acPidPath[128];
	FILE	*pFPidPath;
	static	int		iPid = 0;

	sprintf(acPidPath, "%s/lock/gotspool", gacGCUXHome);

	if ((pFPidPath = fopen(acPidPath, "r"))!= NULL)
	{
		fscanf(pFPidPath, "%d", &iPid);
		Debug(( DBG_L_DUMP_V, "GetSpoolPid: Pid = %d\n", iPid ));
		if (kill(iPid, 0) == -1)
		{
			iPid = 0;
		}
		fclose(pFPidPath);
	}
	else
	{
		Debug(( DBG_L_FATAL, "GetSpoolPid: Impossibile aprire lock file '%s'\n",
				acPidPath ));
	}
	return(iPid);
}


/*
 *
 *	Function:		MountPaper
 *
 *	Description:	Effettua il monta carta sulla stampante indicata
 *
 *	Parameters:		messaggio ricevuto dal client
 *
 *	Returns:		void
 *
 */
void	MountPaper(char *pcBuf)
{
	char	acFilePath[256];
	char	acPaper[9];
	char	acPrinter[9];
	_strPrinterItem	*pstrPrinterItem;
	FILE	*pFCfg;

	Debug(( DBG_L_START_F, "MountPaper: Inizio funzione\n" ));

	pcBuf += 7;
	memset(acPaper, '\0', sizeof(acPaper));
	memset(acPrinter, '\0', sizeof(acPrinter));
	sscanf(pcBuf, "%8c%8c", acPrinter, acPaper);
	StringTrimm(acPrinter);
	StringTrimm(acPaper);

	RestartPrinterList();

	while((pstrPrinterItem = GetNextPrinter()) != NULL)
	{
		if (strcmp(pstrPrinterItem->acPrinterName, acPrinter) != 0)
		{
			continue;
		}
		// pstrPrinterItem->cMntPaperSent = 0;
		strcpy(pstrPrinterItem->acCurrentPaper, acPaper);

		sprintf(acFilePath, "%s/printers/%s", gacGCUXHome, acPrinter);
		if ((pFCfg = fopen(acFilePath, "r"))== NULL)
		{
			Debug(( DBG_L_TRACE,
					"MountPaper: Impossibile aprire file printer '%s'\n",
					acFilePath ));
		}
		else
		{
			CfgGetFile(pFCfg);
			fclose(pFCfg);
			CfgUpdValue(NULL, "PAPER", acPaper);
			CfgUpdFile(acFilePath);
			Debug(( DBG_L_TRACE,
					"MountPaper: modificata carta a '%s' per printer '%s'\n",
					acPaper, acFilePath ));
		}

		SendSock("90OK");
		SendSock("91");
		SendPrinters();
		SendPapers();
		SendReports();
		SendSock("92");
		kill(GetSpoolPid(), SIGHUP);
		return;
	}

	SendSock("90KO");
	Debug(( DBG_L_STOP_F, "MountPaper: Fine funzione\n" ));
}


/*
 *
 *	Function:		ChangeStatus
 *
 *	Description:	Cambia lo stato di un report (H/K/D)
 *
 *	Parameters:		messaggio ricevuto dal client
 *
 *	Returns:		void
 *
 */
void	ChangeStatus(char *pcBuf)
{
	char	acReportId[33];
	char	acPrinter[9];
	int		iStatus;
	_strReportItem	*pstrReportLocal;

	Debug(( DBG_L_START_F, "ChangeStatus: Inizio funzione\n" ));

	pcBuf += 7;
	memset(acReportId, '\0', sizeof(acReportId));
	memset(acPrinter, '\0', sizeof(acPrinter));
	sscanf(pcBuf, "%32c%8c%3d", acReportId, acPrinter, &iStatus);
	StringTrimm(acReportId);
	StringTrimm(acPrinter);

	Debug(( DBG_L_DUMP_V,
			"ChangeStatus: Cambio di stato per stampante '%s' repo '%s' (%d)\n",
			acPrinter, acReportId, iStatus ));

	if ((pstrReportLocal = SearchReport(acReportId, acPrinter))!= NULL)
	{
		pstrReportLocal->sDispoOpt = (short) iStatus;

		if (! cFlagMul)
		{
			SendSock("90OK");
			SendSock("91");
			SendPrinters();
			SendPapers();
			SendReports();
			SendSock("92");
		}
	}
	else
	{
		if (! cFlagMul)
		{
			SendSock("90KO");
		}
	}

	if (! cFlagMul)
	{
		kill(GetSpoolPid(), SIGHUP);
	}

	Debug(( DBG_L_STOP_F, "ChangeStatus: Fine funzione\n" ));
}


/*
 *
 *	Function:		DeleteReport
 *
 *	Description:	Cancella dalla coda il report richiesto
 *
 *	Parameters:		messaggio ricevuto dal client
 *
 *	Returns:		void
 *
 */
int		DeleteReport(char *pcBuf)
{
	char	acReportId[33];
	char	acPathToDelete[128];
	char	acPrinter[9];
	char	*pcToRepoFile;
	char	*pcToDot;
	_strReportItem	*pstrReportLocal;

	Debug(( DBG_L_START_F, "DeleteReport: Inizio funzione\n" ));

	pcBuf += 7;
	memset(acReportId, '\0', sizeof(acReportId));
	memset(acPrinter, '\0', sizeof(acPrinter));
	sscanf(pcBuf, "%32c%8c", acReportId, acPrinter);
	StringTrimm(acReportId);
	StringTrimm(acPrinter);

	if (DeleteReportFromQueue(acReportId, acPrinter) == 0)
	{
		Debug(( DBG_L_TRACE,
				"DeleteReport: Cancellazione di %s %s effettuata\n", 
				acReportId, acPrinter ));
		if ((pcToRepoFile = strrchr(acReportId, '/'))!= NULL)
		{
			pcToRepoFile++;
		}
		else
		{
			pcToRepoFile = acReportId;
		}
		pcToDot = strchr(pcToRepoFile, '.');
		*pcToDot = '\0';

		/*
		 * Cancello il file di parametri.
		 */
		sprintf(acPathToDelete, "%s/spool/%s.LDD",
				 gacGCUXHome, pcToRepoFile);
		Debug(( DBG_L_TRACE,
				"DeleteReport: Cancellazione del file di parametri %s\n", 
				acPathToDelete ));
		remove(acPathToDelete);

		/*
		 * Cancello il file di stampa.
		 */
		sprintf(acPathToDelete, "%s/spool/%s.PRN",
				 gacGCUXHome, pcToRepoFile);
		Debug(( DBG_L_TRACE,
				"DeleteReport: Cancellazione del file di stampa %s\n", 
				acPathToDelete ));
		remove(acPathToDelete);

		if (! cFlagMul)
		{
			SendSock("90OK");
			SendSock("91");
			SendPrinters();
			SendPapers();
			SendReports();
			SendSock("92");
		}
	}
	else
	{
		Debug(( DBG_L_TRACE,
				"DeleteReport: Cancellazione di %s %s NON effettuata\n", 
				acReportId, acPrinter ));
		if (! cFlagMul)
		{
			SendSock("90KO");
		}
	}
	Debug(( DBG_L_STOP_F, "DeleteReport: Fine funzione\n" ));
}


/*
 *
 *	Function:		ChangePrinter
 *
 *	Description:	Cambia il device di stampa per un report
 *
 *	Parameters:		messaggio ricevuto dal client
 *
 *	Returns:		void
 *
 */
void	ChangePrinter(char *pcBuf)
{
	char	acReportId[33];
	char	acPathToDelete[128];
	char	acOldPrinter[9];
	char	acNewPrinter[9];
	char	*pcToRepoFile;
	char	*pcToDot;
	_strReportItem	*pstrReportLocal;

	Debug(( DBG_L_START_F, "ChangePrinter: Inizio funzione\n" ));

	pcBuf += 7;
	memset(acReportId, '\0', sizeof(acReportId));
	memset(acOldPrinter, '\0', sizeof(acOldPrinter));
	memset(acNewPrinter, '\0', sizeof(acNewPrinter));
	sscanf(pcBuf, "%32c%8c%8c", acReportId, acOldPrinter, acNewPrinter);
	StringTrimm(acReportId);
	StringTrimm(acOldPrinter);
	StringTrimm(acNewPrinter);

	if (MoveReportBetweenQueue(acReportId, 
								 acOldPrinter, acNewPrinter) == 0)
	{
		Debug(( DBG_L_TRACE,
			"ChangePrinter: Spostamento di '%s' da '%s' a '%s' completato\n", 
				acReportId, acOldPrinter, acNewPrinter ));

		if (! cFlagMul)
		{
			SendSock("90OK");
			SendSock("91");
			SendPrinters();
			SendPapers();
			SendReports();
			SendSock("92");
		}
	}
	else
	{
		Debug(( DBG_L_TRACE,
			"ChangePrinter: Spostamento di '%s' da '%s' a '%s' abortito\n", 
				acReportId, acOldPrinter, acNewPrinter ));

		if (! cFlagMul)
		{
			SendSock("90KO");
		}
	}
	Debug(( DBG_L_STOP_F, "ChangePrinter: Fine funzione\n" ));
}



/*
 *
 *	Function:		HandleIncomingConn
 *
 *	Description:	Gestisce il protocollo comunicazione con il client
 *
 *	Parameters:		nessuno
 *
 *	Returns:		void
 *
 */
void	HandleIncomingConn()
{
	int		iPid;
	int		iCount = 0;
	int		iRet;
	char	acDummy[8];
	char	acBuf[1024];
	char	acBufDebugFile[256];

	iPid = fork();

	switch(iPid)
	{
	case -1:
		Debug(( DBG_L_DUMP_V,
				"HandleIncomingConn: errore %d in fork\n", errno ));
		break;
		
	case 0:
		/*
		 * Figlio
		 */
		break;
	
	default:
		/*
		 * Padre
		 */
		AddClient(iPid);
		CloseReceiveSocket();
		return;
	}

	/*
	 * Apro il nuovo file di Debug.
	 */
	DebugEnd(( ));

	memset(acBufDebugFile, '\0', sizeof(acBufDebugFile));
	sprintf(acBufDebugFile, "%s.%05d", gpcPrgName, getpid());
	while(acBufDebugFile[iCount] != '\0')
	{
		acBufDebugFile[iCount] = toupper(acBufDebugFile[iCount]);
		iCount++;
	}
	DebugInit((20, acBufDebugFile ));

	Debug(( DBG_L_START_F | DBG_TIMEONLY,
			"HandleIncomingConn: inizio funzione.\n" ));

	Debug(( DBG_L_START_F | DBG_TIMEONLY,
			"HandleIncomingConn: %s %s.\n", gacGCUXHome, gacHomeDir ));
	strcpy(gacGCUXHome, gacHomeDir);

	LoadPapers();

	Debug(( DBG_L_TRACE | DBG_TIMEONLY,
			"HandleIncomingConn: attaching to SHM\n" ));
	InitSHM(0);

	siginterrupt(SIGALRM, 1);
	signal(SIGALRM, SigTrap);
	signal(SIGTERM, SigTrap);
	signal(SIGCHLD, SIG_IGN);

	/*
	 * Ricevo l'identificativo dell'utente.
	 */
	alarm(3);
	iRet = RecvSock(acBuf, sizeof(acBuf));
	alarm(0);
	if (iRet != 0)
	{
		Debug(( DBG_L_TRACE,
				"HandleIncomingConn: Errore %d in ricezione\n", iRet ));
		Debug(( DBG_L_TRACE,
				"HandleIncomingConn: Imposto userid a SYSADM\n", iRet ));
		strcpy(gacUserId, "SYSADM");
	}
	else
	{
		sscanf(acBuf, "%7c%20c", acDummy, gacUserId);
		StringTrimm(gacUserId);
	}

	SendSock("91");
	SendPrinters();
	SendPapers();
	SendReports();
	SendSock("92");

	cFlagMul = FALSE;
	while(TRUE)
	{
		memset(acBuf, '\0', sizeof(acBuf));

		iRet = RecvSock(acBuf, sizeof(acBuf));
		if (iRet != 0)
		{
			Debug(( DBG_L_TRACE,
					"HandleIncomingConn: Errore %d in ricezione\n", iRet ));
			CloseReceiveSocket();
			exit(iRet);
		}

		switch(GetFunctionRequired(acBuf))
		{
		case MNTPPR:
			MountPaper(acBuf);
			break;
			
		case CHGSTT:
			ChangeStatus(acBuf);
			break;

		case DELRPT:
			DeleteReport(acBuf);
			break;
			
		case CHGPRT:
			ChangePrinter(acBuf);
			break;
			
		case RPTDMP:
			DumpReport(acBuf);
			break;
			
		case REFRESH:
			SendSock("91");
			SendPrinters();
			SendPapers();
			SendReports();
			SendSock("92");
			break;
			
		case STARTMUL:
			cFlagMul = TRUE;
			break;

		case STOPMUL:
			SendSock("90OK");
			SendSock("91");
			SendPrinters();
			SendPapers();
			SendReports();
			SendSock("92");
			kill(GetSpoolPid(), SIGHUP);
			cFlagMul = FALSE;
			break;
		}
	}

	Debug(( DBG_L_STOP_F, "HandleIncomingConn: Fine funzione\n" ));
}
