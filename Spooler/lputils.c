/* Global includes */
#include	<stdio.h>
#include	<signal.h>
#include	<errno.h>
#include	<string.h>
#include	<netdb.h>
#include	<netinet/in.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/file.h>
#include	<unistd.h>

/* Local module includes */
#include	"gotspool.h"
#include	"debug.h"
#include	"libshm.h"
#include	"cfgmng.h"

/* Costants */

/* Macroes */               

/* Local struct & new type definition */

/* External Functions */
extern	int		SendPaperMountRequest(char *pcMasterStation, char *pcPaper, char *pcPrinterName);
extern	void	LpGetStartSequence(char *pcStartSequence, char *pcPrinter, char *pcPaper, int *piSeqLen);

/* External Variables */
extern	char	*pcSHMPrinterBuf;
extern	char	*pcSHMReportBuf;
extern	char	acPrinterIdx[];
extern	char	acReportIdx[];
extern	char	gacGCUXHome[];
extern	char	gcFlgReportQueued;
extern	struct	servent	gstrLPServEnt;

/* Global Variables */

/* Local Variables */
static	char			acHostName[64];
static	int				iResvPort;
static	int				iLpdSock;
static	_strPrinterItem	*pstrPrintItem, *pstrPrintCurr;
static	_strReportItem	*pstrReportItem;
static	struct sockaddr_in	strLpdAddr;



/*
 *
 *	Function:		writen
 *
 *	Description:	scrive n bytes su file descriptor (cfr stevens)
 *
 *	Parameters:		file descriptor su cui scrivere,
 *					buffer da scrivere,
 *					numero di bytes attesi
 *
 *	Returns:		
 *
 */
int		writen(int iFd, char *pcBuf, int iBufLen)
{
	int		iLeft, iWritten;

	iLeft = iBufLen;
	while(iLeft > 0)
	{
		iWritten = write(iFd, pcBuf, iLeft);
		if (iWritten <= 0)
		{
			return(iWritten);
		}
		iLeft -= iWritten;
		pcBuf += iWritten;
	}
	return(iBufLen - iLeft);
}


/*
 *
 *	Function:		LpGetSeqNo
 *
 *	Description:	Logga il numero di sequenza usato per la stampa
 *
 *	Parameters:		nessuno
 *
 *	Returns:		numero di sequenza
 *
 */
int		LpGetSeqNo(void)
{
	static	int		iSeqNo;
	char	acPath[256];
	FILE	*pFIn;

	sprintf(acPath, "%s/spool/.seqno", gacGCUXHome);
	if ((pFIn = fopen(acPath, "r+")) == NULL)
	{
		Debug(( DBG_L_FATAL,
				"LpGetSeqNo: Errore %d in apertura '%s'\n", errno, acPath ));
		return(-1);
	}

	if (flock(fileno(pFIn ), LOCK_EX) != 0)
	{
		Debug(( DBG_L_FATAL,
				"LpGetSeqNo: Errore %d in flock\n", errno ));
		fclose(pFIn);
		return(-1);
	}
	
	if (fscanf(pFIn, "%d", &iSeqNo) != 1)
	{
		Debug(( DBG_L_FATAL,
				"LpGetSeqNo: Errore %d in fscanf\n", errno ));
		fclose(pFIn);
		return(-1);
	}

	rewind(pFIn);
	fprintf(pFIn, "%03d", (iSeqNo + 1) % 1000);
	fflush(pFIn);

	if (flock(fileno(pFIn ), LOCK_UN) != 0)
	{
		Debug(( DBG_L_FATAL,
				"LpGetSeqNo: Errore %d in unlock\n", errno ));
	}

	fclose(pFIn);
	return(iSeqNo);
}

/*
 *
 *	Function:		GetOneLine
 *
 *	Description:	Riceve una riga da socket
 *
 *	Parameters:		socket di lettura, buffer, size
 *
 *	Returns:		caratteri letti
 *
 */
int		GetOneLine(int	iSock, char *pcBuf, int iBufSize)
{
	char	cOneByte;
	char	cLoop = 1;
	int		iReadBytes;
	static	int		iCount;

	Debug(( DBG_L_START_F, "GetOneLine: inizio funzione\n" ));
	memset(pcBuf, '\0', iBufSize);
	iCount = 0;
	while(cLoop)
	{
		siginterrupt(SIGALRM, 1);
		alarm(6);
		iReadBytes = read(iSock, &cOneByte, 1);
		siginterrupt(SIGALRM, 0);
		alarm(0);
		switch(iReadBytes)
		{
		case 0:
			/*
			 * Raggiunta la fine file.
			 */
			Debug(( DBG_L_TRACE,
					"GetOneLine: il partner ha chiuso il socket\n" ));
			cLoop = 0;
			continue;
			break;

		case 1:
			break;
		
		case -1:
			Debug(( DBG_L_DUMP_V,
					"GetOneLine: errore %d in ricezione\n", errno ));
			cLoop = 0;
			continue;
			break;

		default:
			Debug(( DBG_L_DUMP_V,
					"GetOneLine: ricevuti %d bytes\n", iReadBytes ));
			cLoop = 0;
			continue;
			break;
		}
		iCount++;
		if (cOneByte == '\n')
			break;
		*(pcBuf + iCount - 1) = cOneByte;
	}

	Debug(( DBG_L_DUMP_V, "GetOneLine: ricevuto '%s'\n", pcBuf ));
	Debug(( DBG_L_STOP_F, "GetOneLine: fine funzione\n" ));

	return(iCount);
}


/*
 *
 *	Function:		GetLpdStatus
 *
 *	Description:	Modifica il flag di stato della stampante in base alla 
 *					risposta della spooler
 *
 *	Parameters:		stringa di stato
 *
 *	Returns:		nessuno
 *
 */
void	GetLpdStatus(char *acAnswer)
{
	if (strstr(acAnswer, "is down") != NULL)
	{
		pstrPrintItem->lUnixLpStatus &= ~ LPD_STATUS_PRINTER_UP;
	}
	if (strstr(acAnswer, "printing disabled") != NULL)
	{
		pstrPrintItem->lUnixLpStatus &= ~ LPD_STATUS_PRINTING_ENABLED;
	}
	if (strstr(acAnswer, "queue is turned off") != NULL)
	{
		pstrPrintItem->lUnixLpStatus &= ~ LPD_STATUS_QUEUEING_ENABLED;
	}
}


/*
 *
 *	Function:		LpdOpenConnection
 *
 *	Description:	Apre una connessione con il server lpd per la richiesta
 *					di informazioni
 *
 *	Parameters:		nessuno
 *
 *	Returns:		nessuno
 *
 */
int		LpdOpenConnection()
{
	struct	hostent	*pstrHostEnt;

	Debug(( DBG_L_START_F, "LpdOpenConnection: inizio funzione\n" ));


	strLpdAddr.sin_family = AF_INET;
	strLpdAddr.sin_port = gstrLPServEnt.s_port;

	memcpy((char *) &strLpdAddr.sin_addr, &(pstrPrintItem->lIpAddress), 
			sizeof(strLpdAddr.sin_addr ));

	Debug(( DBG_L_TRACE, "LpdOpenConnection: socket per porta riservata\n" ));

	/*
	 * Acquisizione di una porta riservata per la connessione a lpd
	 */
	iResvPort = gstrLPServEnt.s_port;
	iLpdSock = rresvport(&iResvPort);

	if (iLpdSock < 0)
	{
		Debug(( DBG_L_FATAL,
				"LpdOpenConnection: rresvport ha ritornato errore %d\n",
				errno ));
		return(EXIT_ERROR_RRESVPORT);
	}

	Debug(( DBG_L_DUMP_V,
			"LpdOpenConnection: attivo connessione a '%s' porta '%d'\n",
			pstrPrintItem->acLpdMaster, htons(gstrLPServEnt.s_port)));

	siginterrupt(SIGALRM, 1);
	alarm(4);
	if (connect(iLpdSock, (struct sockaddr *) &strLpdAddr,
				  sizeof(strLpdAddr)) < 0)
	{
		siginterrupt(SIGALRM, 0);
		alarm(0);
		Debug(( DBG_L_FATAL,
				"LpdOpenConnection: Connect fallita con errore %d\n",
				errno ));
		close(iLpdSock);
		return(EXIT_ERROR_LPD_CONNECT);
	}
	siginterrupt(SIGALRM, 0);
	alarm(0);
	Debug(( DBG_L_STOP_F, "LpdOpenConnection: fine funzione\n" ));
	return(0);
}


/*
 *
 *	Function:		ReportPrinted
 *
 *	Description:	Cancella dalla coda un report che risulta stampato se
 *					il suo stato era Delete, cancellandone anche il file
 *					di parametri, quello dati e quello di stampa (cfA...)
 *					In caso fosse a Keep ne cambia semplicemente lo stato a
 *					Hold
 *
 *	Parameters:		nome della stampante
 *
 *	Returns:		nessuno
 *
 */
void	ReportPrinted(char *pcPrinterName)
{
	char	acFilePath[256];
	char	acReportName[40];
	char	*pcToFileName, *pcToDot;
	FILE	*pFCtrl;

	Debug(( DBG_L_TRACE,
			"ReportPrinted: Cancello print file %s\n", 
			pstrReportItem->acUnixId ));

	sprintf(acFilePath, "%s/spool/%s",
			 gacGCUXHome, pstrReportItem->acUnixId);
	remove(acFilePath);

	if (pstrReportItem->sDispoOpt == GCOS_DISPO_OPTION_KEEP)
	{
		if ((pFCtrl = fopen(pstrReportItem->acCfgFilePath, "r" ))	
														== NULL)
		{
			Debug(( DBG_L_TRACE,
				"ReportPrinted: Impossibile cambiare UNIX_ID errore %d\n",
					errno ));
		}
		else
		{
			CfgGetFile(pFCtrl);
			fclose(pFCtrl);
			CfgUpdValue(NULL, "UNIX_ID", "0" );
			CfgUpdFile(pstrReportItem->acCfgFilePath);
			Debug(( DBG_L_DUMP_V,
					"ReportPrinted: UNIX_ID di %s sostituito con %s\n",
					pstrReportItem->acCfgFilePath, "0" ));
		}

		strcpy(pstrReportItem->acUnixId, "0");
		pstrReportItem->sDispoOpt = GCOS_DISPO_OPTION_HOLD;
		return;
	}

	/*
	 * Il report e' gia' stato mandato in stampa 
	 */
	strcpy(acReportName, pstrReportItem->acReportPath);

	if ((pcToFileName = strrchr(acReportName, '/')) == NULL)
	{
		pcToFileName = acReportName;
	}
	else
	{
		pcToFileName++;
	}

	Debug(( DBG_L_TRACE,
			"ReportPrinted: report '%s' di '%s' viene cancellato\n",
			pcToFileName, pcPrinterName ));

	DeleteReportFromQueue(pcToFileName, pcPrinterName);

	if ((pcToDot = strchr(pcToFileName, '.')) != NULL)
	{
		*pcToDot = '\0';
	}

	Debug(( DBG_L_DUMP_V,
			"ReportPrinted: File di stampa '%s'\n", pcToFileName ));

	sprintf(acFilePath,
			 "%s/spool/%s.LDD", gacGCUXHome, pcToFileName);
	Debug(( DBG_L_DUMP_V,
			"ReportPrinted: cancello '%s'\n", acFilePath ));
	remove(acFilePath);

	sprintf(acFilePath,
			 "%s/spool/%s.PRN", gacGCUXHome, pcToFileName);
	Debug(( DBG_L_DUMP_V,
			"ReportPrinted: cancello '%s'\n", acFilePath ));
	remove(acFilePath);
	return;
}


/*
 *
 *	Function:		LpCheckStatus
 *
 *	Description:	Interroga il line spooler e imposta lo stato di ogni
 *					stampante precedentemente caricata
 *
 *	Parameters:		nome della stampante da verificare. Se NULL tutte.
 *
 *	Returns:		nessuno
 *
 */
void	LpCheckStatus()
{
	char	acCmd[64];
	char	acAnswer[256];

	Debug(( DBG_L_START_F, "LpCheckStatus: inizio funzione\n" ));

	memset(acCmd, '\0', sizeof(acCmd ));
	sprintf(acCmd, "%c%s\n",
			 LPD_QUERY_STATUS, pstrPrintItem->acPrinterName);
	
	Debug(( DBG_L_TRACE, "LpCheckStatus: invio query per '%s' a lpd\n",
			pstrPrintItem->acPrinterName ));

	if (LpdOpenConnection() != 0)
	{
		pstrPrintItem->lUnixLpStatus &= ~LPD_STATUS_QUEUEING_ENABLED;
		pstrPrintItem->lUnixLpStatus &= ~LPD_STATUS_PRINTING_ENABLED;
		strcpy(pstrPrintItem->acCurrentPaper, "ZZZZ");
		return;
	}

	if (writen(iLpdSock, acCmd, strlen(acCmd)) != strlen(acCmd))
	{
		pstrPrintItem->lUnixLpStatus &= ~LPD_STATUS_QUEUEING_ENABLED;
		pstrPrintItem->lUnixLpStatus &= ~LPD_STATUS_PRINTING_ENABLED;

		Debug(( DBG_L_DUMP_V,
				"LpCheckStatus: errore %d in query per stampante '%s'\n",
				errno, pstrPrintItem->acPrinterName ));
		Debug(( DBG_L_TRACE,
				"LpCheckStatus: stampante '%s' disabilitata\n",
				pstrPrintItem->acPrinterName ));
		pstrPrintItem->lUnixLpStatus = 0L;
		close(iLpdSock);
		return;
	}

	pstrPrintItem->lUnixLpStatus |= LPD_STATUS_QUEUEING_ENABLED |
									LPD_STATUS_PRINTING_ENABLED;
	while(GetOneLine(iLpdSock, acAnswer, sizeof(acAnswer)) != 0)
	{
		pstrPrintItem->lUnixLpStatus |= LPD_STATUS_QUEUEING_ENABLED |
									    LPD_STATUS_PRINTING_ENABLED;

		Debug(( DBG_L_DUMP_V, "LpCheckStatus: ricevuto '%s'\n", acAnswer ));
		if (strstr(acAnswer, "unknown printer") != NULL)
		{
			pstrPrintItem->lUnixLpStatus = 0L;
			continue;
		}
		if (strstr(acAnswer, "Warning:") != NULL)
		{
			if (strstr(pstrPrintItem->acPrinterName, acAnswer) != NULL)
			{
				GetLpdStatus(acAnswer);
			}
			continue;
		}
		if (strstr(acAnswer, "Rank") != NULL)
		{
			/*
			 * Intestazione record di stampa.
			 */
			pstrPrintItem->lUnixLpStatus |= LPD_STATUS_PRINTING_REPORT;
			continue;
		}
		if ((strstr(acAnswer, "no entries") != NULL)||
			(strlen(acAnswer) == 0)||
			(strstr(acAnswer, "0 jobs") != NULL)||
			(strstr(acAnswer, "LPD") != NULL ))
		{

			if (pstrPrintItem->lUnixLpStatus & LPD_STATUS_PRINTING_REPORT)
			{
				ResetReportList(pstrPrintItem->acPrinterName);
				while((pstrReportItem = 
							GetNextReport(pstrPrintItem->acPrinterName)) 
																!= NULL)
				{
					Debug(( DBG_L_TRACE,
							"LpCheckStatus: stampante %s report %s cf %s\n",
							pstrPrintItem->acPrinterName,
							pstrReportItem->acReportName,
							pstrReportItem->acUnixId ));

					if (strcmp(pstrReportItem->acUnixId, "0") != 0)
					/*
					 * La stampante e' libera mentre a me risulta che
					 * era in corso la stampa di un report.
					 * Provvedo a cancellare l'unico report della sua lista
					 * che risulta in stampa.
					 */
						ReportPrinted(pstrPrintItem->acPrinterName);
				}
			}

			/*
			 * Modifico lo stato della stampante e risetto a NULL
			 * l'identificativo UNIX del report in stampa.
			 * Il prossimo EnqueueReport trovera' la stampante libera.
			 */
			pstrPrintItem->lUnixLpStatus &= ~ LPD_STATUS_PRINTING_REPORT;
			strcpy(pstrPrintItem->acPrintingRpt, "");
			strcpy(pstrPrintItem->acPrintingRptJ, "");
			strcpy(pstrPrintItem->acPrintingRptS, "");
			Debug(( DBG_L_TRACE, "LpCheckStatus: stampante LIBERA\n" ));
		}
		/*
		 * Per il momento non ci interessa piu' nient'altro
		 */
	}
	close(iLpdSock);

	Debug(( DBG_L_START_F, "LpCheckStatus: fine funzione\n" ));
	return;
}


/*
 *
 *	Function:		LpSendData
 *
 *	Description:	Invio del file
 *
 *	Parameters:		nome del file da stampare, numero di sequenza (stringa)
 *					lunghezza parametro sequenza
 *
 *	Returns:		0 per OK, -1 in caso di errore
 *
 */
int		LpSendData(char *pcFileName, char *pcStartSequence, int iSeqLen)
{
	char	acPath[512];
	char	acBuf[512];
	int		iLen;
	int		iReadBytes = 0;
	int		iSentBytes = 0;
	int		iFDLp;
	struct	stat	strBuf;

	sprintf(acPath, "%s/spool/%s", gacGCUXHome, pcFileName);

	if (stat(acPath, &strBuf) == -1)
	{
		Debug(( DBG_L_DUMP_V,
				"LpSendData: errore %d in stat su file dati %s\n",
				errno, acPath ));
		return(-1);
	}

	if ((iFDLp = open(acPath, O_RDONLY)) == -1)
	{
		Debug(( DBG_L_DUMP_V,
				"LpSendData: errore %d in apertura file dati %s\n",
				errno, acPath ));
		return(-1);
	}

	if (strlen(pcStartSequence) != 0)
	{
		if (writen(iLpdSock, pcStartSequence, iSeqLen) != iSeqLen)
		{
			Debug(( DBG_L_FATAL,
					"LpSendData: errore %d durante spedizione %s\n",
					errno, acPath ));
			close(iFDLp);
			return(-1);
		}
		iSentBytes += iSeqLen;
		Debug(( DBG_L_TRACE, "LpSendData: spedita start seq %d bytes\n",
				iSentBytes ));
	}

	Debug(( DBG_L_TRACE, "LpSendData: invio  %d bytes\n", strBuf.st_size ));
	memset(acBuf, '\0', sizeof(acBuf ));
	while(1)
	{
		if ((iLen = (strBuf.st_size > 512 ? 512 : strBuf.st_size)) == 0)
		{
			break;
		}
		strBuf.st_size -= iLen;

		if (read(iFDLp, acBuf, iLen) != iLen)
		{
			Debug(( DBG_L_FATAL,
					"LpSendData: errore %d in lettura da  %s\n",
					errno, acPath ));
			close(iFDLp);
			return(-1);
		}

		if (writen(iLpdSock, acBuf, iLen) != iLen)
		{
			Debug(( DBG_L_FATAL,
					"LpSendData: errore %d durante spedizione %s\n",
					errno, acPath ));
			close(iFDLp);
			return(-1);
		}
		iSentBytes += iLen;
		memset(acBuf, '\0', sizeof(acBuf ));
	}
	close(iFDLp);
	Debug(( DBG_L_TRACE, "LpSendData: spediti %d bytes\n", iSentBytes ));
	return(0);
}

/*
 *
 *	Function:		LpSendFile
 *
 *	Description:	Invia il file di controllo e il file di stampa a lpd
 *
 *	Parameters:		nome del file di controllo, nome del file da stampare,
 *					flag inizializzazione stampante
 *
 *	Returns:		0 per OK, 1 o errno in caso di errore
 *
 */
int		LpSendFile(char *pcControlFile, char *pcDataFile, char cFlgInitPrt)
{
	char	acCmd[64];
	char	acAnswer[256];
	char	acLocalReportPath[256];
	char	acStartSequence[32];
	int		iSeqLen;
	int		iRcvdBytes;
	FILE	*pFToLp, *pFCfg;
	struct	stat	strStatBuf;
	static	int		iRetValue;

	Debug(( DBG_L_START_F, "LpSendFile: inizio funzione\n" ));

	sleep(1);
	if ((iRetValue = LpdOpenConnection()) != 0)
	{
		return(iRetValue);
	}

	/*
	 * Spedizione della richiesta di invio file di stampa
	 */
	sprintf(acCmd, "%c%s\n",
			 LPD_SUBMIT_REPORT,
			 pstrPrintItem->acPrinterName);
	Debug(( DBG_L_TRACE,
			"LpSendFile: invio richiesta di stampa '%s'\n", acCmd ));
	/*
	 * Invio buffer per file di controllo
	 */
	if (writen(iLpdSock, acCmd, strlen(acCmd)) != strlen(acCmd))
	{
		Debug(( DBG_L_DUMP_V,
				"LpSendFile: errore %d in query per stampante '%s'\n",
				errno, pstrPrintItem->acPrinterName ));
		Debug(( DBG_L_TRACE,
				"LpSendFile: stampante '%s' disabilitata\n",
				pstrPrintItem->acPrinterName ));
		pstrPrintItem->lUnixLpStatus = 0L;
		close(iLpdSock);
		return(1);
	}

	siginterrupt(SIGALRM, 1);
	alarm(5);
	/*
	 * Ricezione acknolegde da lpd.
	 */
	if (read(iLpdSock, acAnswer, 1) != 1)
	{
		Debug(( DBG_L_DUMP_V,
				"LpSendFile: errore %d in read acknoledge\n", errno ));
		close(iLpdSock);
		siginterrupt(SIGALRM, 0);
		alarm(0);
		return(1);
	}
	siginterrupt(SIGALRM, 0);
	alarm(0);

	if (acAnswer[0] != '\0')
	{
		if (GetOneLine(iLpdSock, &acAnswer[1], sizeof(acAnswer)- 1)< 1)
		{
			Debug(( DBG_L_DUMP_V,
					"LpSendFile: errore 0x%02X %d in ACK da server\n",
					(int) acAnswer[0], errno ));
		}
		else
		{
			Debug(( DBG_L_DUMP_V,
					"LpSendFile: errore 0x%02X %s in ACK da server\n",
					(int) acAnswer[0], &acAnswer[1] ));
		}
		close(iLpdSock);
		return(1);
	}

	Debug(( DBG_L_START_F, "LpSendFile: Stampa disponibile\n" ));

	/*
	 * Spedizione del file dati.
	 */

	memset(acStartSequence, '\0', sizeof(acStartSequence ));
	iSeqLen = 0;

	if (cFlgInitPrt)
	{
		/*
		 * Prendo la seuquenza iniziale da inviare alla stampante
		 */
		LpGetStartSequence(acStartSequence,
							pstrReportItem->acDevice,
							pstrReportItem->acPaper, &iSeqLen);
	}

	sprintf(acLocalReportPath, "%s/spool%s", 
			 gacGCUXHome, strrchr(pstrReportItem->acReportPath, '/' ));

	if (stat(acLocalReportPath, &strStatBuf) == -1)
	{
		Debug(( DBG_L_FATAL,
				"LpSendFile: errore %d in stat su '%s'\n",
				errno, acLocalReportPath ));
		close(iLpdSock);
		return(1);
	}

	if (strrchr(pcDataFile, '/') != NULL)
	{
		pcDataFile = (char *) (strrchr(pcDataFile, '/')+ 1);
	}
	sprintf(acCmd, "%c%ld %s\n",
			 LPD_SUBMIT_DATA_FILE,
			 strStatBuf.st_size + iSeqLen, pcDataFile);

	Debug(( DBG_L_TRACE,
			"LpSendFile: Invio comando per data file '%s'\n", acCmd ));
	/*
	 * Invio buffer per file di controllo
	 */
	if (writen(iLpdSock, acCmd, strlen(acCmd)) != strlen(acCmd))
	{
		Debug(( DBG_L_DUMP_V,
				"LpSendFile: errore %d in query per stampante '%s'\n",
				errno, pstrPrintItem->acPrinterName ));
		Debug(( DBG_L_TRACE,
				"LpSendFile: stampante '%s' disabilitata\n",
				pstrPrintItem->acPrinterName ));
		pstrPrintItem->lUnixLpStatus = 0L;
		close(iLpdSock);
		return(1);
	}

	/*
	 * Ricezione acknolegde da lpd.
	 */
	siginterrupt(SIGALRM, 1);
	alarm(5);
	if (read(iLpdSock, acAnswer, 1) != 1)
	{
		Debug(( DBG_L_DUMP_V,
				"LpSendFile: errore %d in read acknoledge\n", errno ));
		close(iLpdSock);
		siginterrupt(SIGALRM, 0);
		alarm(0);
		return(1);
	}
	siginterrupt(SIGALRM, 0);
	alarm(0);

	if (acAnswer[0] != '\0')
	{
		Debug(( DBG_L_DUMP_V,
				"LpSendFile: errore 0x%02X in ricezione ACK da server\n",
				(int) acAnswer[0] ));
		close(iLpdSock);
		return(1);
	}

	Debug(( DBG_L_TRACE,
			"LpSendFile: Invio data file '%s'\n",
			pstrReportItem->acReportPath ));
	/*
	 * Invio file dati
	 */
	if (LpSendData((char *)
					 (strrchr(pstrReportItem->acReportPath, '/') + 1),
					 acStartSequence, iSeqLen) != 0)
	{
		Debug(( DBG_L_DUMP_V,
				"LpSendFile: errore in fase di spedizione di '%s'\n",
				errno, pstrReportItem->acReportPath ));
		close(iLpdSock);
		return(1);
	}

	memset(acAnswer, '\0', sizeof(acAnswer ));
	/*
	 * Richiesta di acknoledge da server
	 */
	if (writen(iLpdSock, "", 1) != 1)
	{
		Debug(( DBG_L_DUMP_V,
				"LpSendFile: errore %d in query per stampante '%s'\n",
				errno, pstrPrintItem->acPrinterName ));
		Debug(( DBG_L_TRACE,
				"LpSendFile: stampante '%s' disabilitata\n",
				pstrPrintItem->acPrinterName ));
		pstrPrintItem->lUnixLpStatus = 0L;
		close(iLpdSock);
		return(1);
	}

	// siginterrupt(SIGALRM, 1);
	// alarm(5);
	if (read(iLpdSock, acAnswer, 1) != 1)
	{
		Debug(( DBG_L_DUMP_V,
				"LpSendFile: errore %d in read acknoledge (datafile)\n",
				errno ));
		close(iLpdSock);
		siginterrupt(SIGALRM, 0);
		alarm(0);
		return(1);
	}
	// siginterrupt(SIGALRM, 0);
	// alarm(0);

	if (acAnswer[0] != '\0')
	{
		Debug(( DBG_L_DUMP_V,
				"LpSendFile: errore 0x%02X in ricezione ACK da server\n",
				(int) acAnswer[0] ));
		close(iLpdSock);
		return(1);
	}


	/*
	 * Invio del file di controllo
	 */
	if (stat(pcControlFile, &strStatBuf) == -1)
	{
		Debug(( DBG_L_FATAL,
				"LpSendFile: errore %d in stat su '%s'\n",
				errno, pcControlFile ));
		close(iLpdSock);
		return(1);
	}

	if (strrchr(pcControlFile, '/') != NULL)
	{
		pcControlFile = (char *) (strrchr(pcControlFile, '/')+ 1);
	}

	sprintf(acCmd, "%c%ld %s\n",
			 LPD_SUBMIT_REPORT, strStatBuf.st_size, pcControlFile);
	
	Debug(( DBG_L_TRACE,
			"LpSendFile: Invio comando per control file '%s'\n", acCmd ));

	if (writen(iLpdSock, acCmd, strlen(acCmd)) != strlen(acCmd))
	{
		Debug(( DBG_L_DUMP_V,
				"LpSendFile: errore %d in query per stampante '%s'\n",
				errno, pstrPrintItem->acPrinterName ));
		Debug(( DBG_L_TRACE,
				"LpSendFile: stampante '%s' disabilitata\n",
				pstrPrintItem->acPrinterName ));
		pstrPrintItem->lUnixLpStatus = 0L;
		close(iLpdSock);
		return(1);
	}

	/*
	 * Lettura dell'acknoledge.
	 */
	siginterrupt(SIGALRM, 1);
	alarm(5);
	if (read(iLpdSock, acAnswer, 1) != 1)
	{
		Debug(( DBG_L_DUMP_V,
				"LpSendFile: errore %d in read acknoledge\n", errno ));
		close(iLpdSock);
		siginterrupt(SIGALRM, 0);
		alarm(0);
		return(1);
	}
	siginterrupt(SIGALRM, 0);
	alarm(0);

	if (acAnswer[0] != '\0')
	{
		Debug(( DBG_L_DUMP_V,
				"LpSendFile: errore 0x%02X in ricezione ACK da server\n",
				acAnswer[0] ));
		close(iLpdSock);
		return(1);
	}

	/*
	 * Invio file di controllo
	 */
	if (LpSendData(pcControlFile, "", 0) != 0)
	{
		Debug(( DBG_L_DUMP_V,
				"LpSendFile: errore in fase di spedizione di '%s'\n",
				errno, pcControlFile ));
		close(iLpdSock);
		return(1);
	}

	strcpy(pstrReportItem->acUnixId, pcControlFile);

	Debug(( DBG_L_DUMP_V,
			"LpSendFile: Update valore di UNIX_ID a %s per file %s\n",
			pcControlFile, pstrReportItem->acCfgFilePath ));

	if ((pFCfg = fopen(pstrReportItem->acCfgFilePath, "r")) == NULL)
	{
		Debug(( DBG_L_TRACE,
				"LpSendFile: Impossibile cambiare UNIX_ID errore %d\n",
				errno ));
	}
	else
	{
		CfgGetFile(pFCfg);
		fclose(pFCfg);
		CfgUpdValue(NULL, "UNIX_ID", pcControlFile);
		CfgUpdFile(pstrReportItem->acCfgFilePath);
		Debug(( DBG_L_DUMP_V,
				"LpSendFile: UNIX_ID di %s sostituito con %s\n",
				pstrReportItem->acCfgFilePath, pcControlFile ));
	}


	memset(acAnswer, '\0', sizeof(acAnswer ));
	/*
	 * Richiesta di acknoledge da server
	 */
	if (writen(iLpdSock, "", 1) != 1)
	{
		Debug(( DBG_L_DUMP_V,
				"LpSendFile: errore %d in query per stampante '%s'\n",
				errno, pstrPrintItem->acPrinterName ));
		Debug(( DBG_L_TRACE,
				"LpSendFile: stampante '%s' disabilitata\n",
				pstrPrintItem->acPrinterName ));
		pstrPrintItem->lUnixLpStatus = 0L;
		close(iLpdSock);
		return(0);
	}

	iRcvdBytes = read(iLpdSock, acAnswer, 1);
	switch(iRcvdBytes)
	{
	case 0:
		Debug(( DBG_L_DUMP_V,
				"LpSendFile: Il partner ha chiuso il socket. Result OK\n" ));
		acAnswer[0] = '\0';
		break;

	case -1:
		Debug(( DBG_L_DUMP_V,
				"LpSendFile: errore %d in read acknoledge (ctrlfile)\n",
				errno ));
		close(iLpdSock);
		return(0);
		break;

	case 1:
		break;
	}

	if (acAnswer[0] != '\0')
	{
		Debug(( DBG_L_DUMP_V,
				"LpSendFile: errore 0x%02X in ricezione ACK da server\n",
				(int) acAnswer[0] ));
		close(iLpdSock);
		return(0);
	}
	close(iLpdSock);

	Debug(( DBG_L_DUMP_V, "LpSendFile: status %ld\n",
			pstrPrintItem->lUnixLpStatus ));
	Debug(( DBG_L_STOP_F, "LpSendFile: fine funzione\n" ));
	return(0);
}


/*
 *
 *	Function:		EnqueueReport
 *
 *	Description:	Verifica se vi sono report in attesa e ne accoda la stampa
 *
 *	Parameters:		nessuno
 *
 *	Returns:		nessuno
 *
 */
void	EnqueueReport()
{
	int		iSeqNum;
	char	cFlgInitPrt = 0;
	char	cFlgPrinterHasReport;
	char	acControlFile[64];
	char	acDataFile[64];
	char	acBSDCtrlFileName[128];
	char	*pcBSDDataFileName;
	FILE	*pFCtrl;

	Debug(( DBG_L_START_F, "EnqueueReport: Inizio funzione\n" ));

	RestartPrinterList();

	gcFlgReportQueued = FALSE;
	while((pstrPrintItem = GetNextPrinter()) != NULL)
	{
		pstrReportItem = NULL;
		/*
		 * Verifico se la stampante e' libera o meno.
		 */
		if (pstrPrintItem->lUnixLpStatus & LPD_STATUS_PRINTING_REPORT)
		{
			/*
			 * Verifico se nel frattempo si e' liberata.
			 */
			LpCheckStatus();

			if ((pstrPrintItem->lUnixLpStatus & LPD_STATUS_QUEUEING_ENABLED) &&
				 (pstrPrintItem->lUnixLpStatus & LPD_STATUS_PRINTING_ENABLED))
			{
				Debug(( DBG_L_TRACE, "EnqueueReport: Stampante %s occupata\n",
						pstrPrintItem->acPrinterName ));
				gcFlgReportQueued = TRUE;
			}
			continue;
		}

		Debug(( DBG_L_TRACE,
				"EnqueueReport: Verifico report per stampante '%s'\n",
				pstrPrintItem->acPrinterName ));

		/*
		 * Verifico se vi sono report da stampare e li accodo.
		 */
		ResetReportList(pstrPrintItem->acPrinterName);

		cFlgPrinterHasReport = FALSE;
		while((pstrReportItem = 
					GetNextReport(pstrPrintItem->acPrinterName)) != NULL)
		{
			cFlgInitPrt = 0;
			Debug(( DBG_L_TRACE,
					"EnqueueReport: report '%s' '%s' ....\n",
					pstrReportItem->acReportName,
					pstrReportItem->acReportPath ));

			if (strcmp(pstrReportItem->acUnixId, "0") != 0)
			{
				/*
				 * Il report e' gia' stato mandato in stampa 
				 */
				Debug(( DBG_L_TRACE,
				"EnqueueReport: report '%s' in stampa (id '%s') lo salto\n",
						pstrReportItem->acReportName,
						pstrReportItem->acUnixId ));
	
				pstrReportItem = pstrReportItem->pstrRepoNext;
				LpCheckStatus();
				gcFlgReportQueued = TRUE;
				break;
			}

			if (pstrReportItem->sDispoOpt == GCOS_DISPO_OPTION_HOLD)
			{
				/*
				 * Il report e' in stato HOLD
				 */
				Debug(( DBG_L_TRACE,
						"EnqueueReport: saltato report '%s' in stato HOLD\n",
						pstrReportItem->acReportName,
						pstrReportItem->sDispoOpt ));
				pstrReportItem = pstrReportItem->pstrRepoNext;
				continue;
			}

			cFlgPrinterHasReport = TRUE;

			/*
			 * Verifico se la stampante e' in condizioni di stampare.
			 */
			LpCheckStatus();
			Debug(( DBG_L_TRACE,
					"EnqueueReport: Stato della stampante %s %d\n",
					pstrPrintItem->acPrinterName,
					pstrPrintItem->lUnixLpStatus ));

			if (!(pstrPrintItem->lUnixLpStatus & LPD_STATUS_QUEUEING_ENABLED))
			{
				/*
				 * La stampante e' down
				 */
				Debug(( DBG_L_TRACE,
				"EnqueueReport: Impossibile stampare. Stampante %s down\n",
						pstrPrintItem->acPrinterName ));
				pstrReportItem = pstrReportItem->pstrRepoNext;
				break;
			}

			Debug(( DBG_L_TRACE,
					"EnqueueReport: La stampante %s e' pronta per la stampa\n",
					pstrPrintItem->acPrinterName ));

			if (strcmp(pstrPrintItem->acCurrentPaper,
						  pstrReportItem->acPaper) != 0)
			{
				Debug(( DBG_L_TRACE, "EnqueueReport: Carta non congruente\n" ));

				if (! pstrPrintItem->cMntPaperSent)
				{
					Debug(( DBG_L_TRACE,
							"EnqueueReport: Invio montacarta\n" ));
					/* 
					 * Mando il monta carta. Se l'operazione dovesse fallire,
					 * al prossimo giro riprovero'
					 */
					pstrPrintItem->cMntPaperSent = 
						 SendPaperMountRequest(pstrPrintItem->acMasterStation,
												pstrReportItem->acPaper,
												pstrPrintItem->acPrinterName);
				}
				/*
				 * Dobbiamo attendere che dal client venga inviato il monta
				 * carta
				 */
				Debug(( DBG_L_TRACE, "EnqueueReport: Attendo montacarta\n" ));
				gcFlgReportQueued = TRUE;
				break;
			}
			else if (pstrPrintItem->cMntPaperSent)
			{
				pstrPrintItem->cMntPaperSent = 0;
				cFlgInitPrt = 1;
			}

			Debug(( DBG_L_DUMP_V,
					"EnqueueReport: Report '%s' pronto per la stampa\n",
					pstrReportItem->acReportPath ));

			strcpy(acBSDCtrlFileName, pstrPrintItem->acPrinterName);

			iSeqNum = LpGetSeqNo();

			if ((pcBSDDataFileName = 
						strrchr(acBSDCtrlFileName, '.')) != NULL)
			{
				pcBSDDataFileName = '\0';
			}

			sprintf(acControlFile, "%s/spool/cfA%03d%-8.8s",
					 gacGCUXHome, iSeqNum, acBSDCtrlFileName);
			sprintf(acDataFile, "%s/spool/dfA%03d%-8.8s",
					 gacGCUXHome, iSeqNum, acBSDCtrlFileName);

			while(acControlFile[strlen(acControlFile)- 1] == ' ')
				acControlFile[strlen(acControlFile)- 1] = '\0';

			while(acDataFile[strlen(acDataFile)- 1] == ' ')
				acDataFile[strlen(acDataFile)- 1] = '\0';

			pcBSDDataFileName = strrchr(acDataFile, '/')+ 1;
			if ((pFCtrl = fopen(acControlFile, "w")) == NULL)
			{
				Debug(( DBG_L_DUMP_V,
						"EnqueueReport: Errore in apertura control file "
						"per stampante '%s' (%s)\n",
						pstrPrintItem->acPrinterName,
						acControlFile ));
				pstrReportItem = pstrReportItem->pstrRepoNext;
				continue;
			}
			fprintf(pFCtrl,
					 "H%s\n"		// host che ha generato la richiesta
					 "P%s\n"		// nome dell'utente che invia il report
					 "C%s\n"		// classe del report
					 "L%s-%s-%s\n"	// nome del job in stampa
					 "J%s\n"		// nome dell'utente su banner
					 "f%s\n"		// acDataFile
					 "U%s\n"		// acDataFile
					 "N%s\n",		// nome del report
					 acHostName,
					 pstrReportItem->acUserName,
					 acHostName,
					 pstrReportItem->acReportName,
					 pstrReportItem->acJobNum,
					 pstrReportItem->acJobSon,
					 pstrReportItem->acUserName,
					 pcBSDDataFileName,
					 pcBSDDataFileName,
					 pstrReportItem->acReportPath);
			fclose(pFCtrl);

			if (LpSendFile(acControlFile, acDataFile, cFlgInitPrt) == 0)
			{
				pstrPrintItem->lUnixLpStatus |= LPD_STATUS_PRINTING_REPORT;
				strcpy(pstrPrintItem->acPrintingRpt, 
						pstrReportItem->acReportName);
				strcpy(pstrPrintItem->acPrintingRptJ,
						pstrReportItem->acJobNum);
				strcpy(pstrPrintItem->acPrintingRptS,
						pstrReportItem->acJobSon);
				gcFlgReportQueued = TRUE;
				break;
			}
			else
			{
				Debug(( DBG_L_TRACE,
						"EnqueueReport: Errore in invio file a  %s\n",
						pstrPrintItem->acPrinterName ));
				pstrReportItem->sDispoOpt = GCOS_DISPO_OPTION_HOLD;
				strcpy(pstrPrintItem->acCurrentPaper, "ZZZZ");
				remove(acControlFile);
			}
			break;
		}
		if (! cFlgPrinterHasReport)
		{
			Debug(( DBG_L_TRACE,
					"EnqueueReport: Stampante libera. Tolgo flag papermount\n"
					));
			pstrPrintItem->cMntPaperSent = 0;
		}

		Debug(( DBG_L_TRACE,
				"EnqueueReport: fine controllo stampante %s\n",
				pstrPrintItem->acPrinterName ));
	}
	Debug(( DBG_L_STOP_F, "EnqueueReport: fine funzione\n" ));
}
