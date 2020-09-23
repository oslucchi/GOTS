/* Global includes */
#include	<stdio.h>
#include	<stdlib.h>
#include	<errno.h>
#include	<string.h>
#include	<sys/sem.h>
#include	<sys/shm.h>

/* Local module includes */
#include	"libshm.h"
#include	"debug.h"
#include	"cfgmng.h"

/* Costants */

/* Macroes */               

/* Local struct & new type definition */

/* External Functions */

/* External Variables */
extern	char gacGCUXHome[];

/* Global Variables */
char	acPrinterIdx[MAX_PRINTER_DEFINED];
char	acReportIdx[MAX_REPO_QUEUE];

char	*pcSHMPrinterBuf;
char	*pcSHMReportBuf;

/* Local Variables */
static	_strPrinterItem	*pstrPrintItem;
static	_strReportItem	*pstrReportItem;
static	_strReportList	apstrReportList[MAX_PRINTER_DEFINED];

static	char	*pcToShmBase;
static	char	*pcPrtIndex;
static	char	*pcPrtVector;
static	char	*pcRptMatrix;

static	int		iPrtIdx;

static	struct	sembuf	semOpLock[2] = 
{
	{0, 0, 0},
	{0, 1, SEM_UNDO}
};

static	struct	sembuf	semOpUnlock[1] = 
{
	{0, -1, (IPC_NOWAIT | SEM_UNDO)}
};

static	int		iSemid = -1;


/*
 *
 *	Function:		InitSHM
 *
 *	Description:	Inizializza la SHM. Termina l'applicazione se
 *					non riesce a fare create/attach alla SHM
 *
 *	Parameters:		flag richiesta creazione SHM (char 1/0)
 *
 *	Returns:		void
 *
 */
void	InitSHM(char cFlgCreate)
{
	key_t	iSHM_KEY;
	char	acPath[1024];
	int		iShmSize;
	int		iShmId;
	int		iMode;
	
	Debug(( DBG_L_START_F, "InitSHM: Inizio funzione (%d)\n", cFlgCreate ));
	Debug(( DBG_L_DUMP_V, "InitSHM: gacGCUXHome is null %s\n", 
							(gacGCUXHome == NULL ? "true" : "false") ));
	printf("InitSHM: gacGCUXHome is null %s\n", 
							(gacGCUXHome == NULL ? "true" : "false"));
	printf("InitSHM: gacGCUXHome '%s'\n", gacGCUXHome);

	sprintf(acPath, "%s", gacGCUXHome);
	iSHM_KEY = ftok(acPath, 'A');
	Debug(( DBG_L_DUMP_V,
			"InitSHM: path per chiave '%s' (%d)\n", gacGCUXHome, iSHM_KEY ));
	
/*
 * La struttura del blocco di memoria condivisa che viene richeisto e'
 * la seguente:
 * |--Indice stampanti--|
 *                      |--Array strutture stampanti--|
 *                                                    |--matrice report--|
 *
 * L'indice contiene ....
 * L'array strutture stampanti contiene la struttura con i dati della stampante
 * La matrice report e' organizzata per riga = stampante colonna = report
 */
	iShmSize = MAX_PRINTER_DEFINED +	// l'array indice delle stampanti
			   sizeof(_strPrinterItem) *
			   		MAX_PRINTER_DEFINED	+ // l'array delle stampanti definite
			   MAX_PRINTER_DEFINED *
			   		sizeof(_strReportItem) * 
					MAX_REPO_QUEUE;		// la matrice dei report

	iMode = 0666 | (cFlgCreate ? IPC_CREAT : 0);

	Debug(( DBG_L_DUMP_V,
			"InitSHM: iShmSize = %d | iMode = %04X | KEY = %08X\n",
			iShmSize, iMode, iSHM_KEY ));

	/*
	 * Allocazione dello spazio in memoria condivisa
	 */
	if ((iShmId = shmget(iSHM_KEY, iShmSize, iMode)) < 0)
	{
		Debug(( DBG_L_FATAL, "InitSHM: Errore %d in shmget\n", errno ));
		exit(EXIT_ALLOCATE_SHM_REPORTS);
	}
	Debug(( DBG_L_TRACE, "InitSHM: chiave per SHM recuperata\n" ));

	if ((pcToShmBase = shmat(iShmId, 0, 0)) == NULL)
	{
		Debug(( DBG_L_FATAL, "InitSHM: Errore %d in shmat\n", errno ));
		exit(EXIT_ATTACH_SHM_REPORTS);
	}
	Debug(( DBG_L_TRACE, "InitSHM: segmento attaccato\n" ));

	pcPrtIndex = pcToShmBase;
	pcPrtVector = pcToShmBase + MAX_PRINTER_DEFINED;
	pcRptMatrix = pcToShmBase + MAX_PRINTER_DEFINED +
				  sizeof(_strPrinterItem) * MAX_PRINTER_DEFINED;
	Debug(( DBG_L_TRACE, "InitSHM: puntatori impostati\n" ));
	
	for(iPrtIdx = 0; iPrtIdx < MAX_PRINTER_DEFINED; iPrtIdx++)
	{
		apstrReportList[iPrtIdx].pstrReportRoot =
			apstrReportList[iPrtIdx].pstrReportCurr =
							(_strReportItem *)
								((long) pcRptMatrix +
								 ((long) sizeof(_strReportItem)) * 
									MAX_REPO_QUEUE * iPrtIdx);
		apstrReportList[iPrtIdx].iCurrentIdx = 0;
	}
	Debug(( DBG_L_TRACE, "InitSHM: vettori puntatori a report impostati\n" ));
	iPrtIdx = 0;

	Debug(( DBG_L_TRACE, "InitSHM: fino a memset\n" ));
	if (cFlgCreate)
		memset(pcToShmBase, '\0', iShmSize);
}



/*
 *
 *	Function:		ResetPrinterList
 *
 *	Description:	Risetta il puntatore all'array delle stampanti al primo
 *					elemento
 *
 *	Parameters:		
 *
 *	Returns:		void
 *
 */
void	RestartPrinterList()
{
	Debug(( DBG_L_START_F, "RestartPrinterList: Inizio funzione\n" ));
	iPrtIdx = 0;
	Debug(( DBG_L_STOP_F, "RestartPrinterList: Inizio funzione\n" ));
}



/*
 *
 *	Function:		SearchPrinter
 *
 *	Description:	Cerca la stampante passata per parametro
 *
 *	Parameters:		nome stampante, puntatore all'indice (se trovata valore
 *					intero dell'indice, NULL viceversa)
 *
 *	Returns:		puntatore alla struttura printer in memoria (NULL se
 *					non trovata)
 *
 */
_strPrinterItem	*SearchPrinter(char *pcPrinterName, int *iIdx)
{
	static	int		iPrtIdx;

	Debug(( DBG_L_START_F, "SearchPrinter: Inizio funzione\n" ));

	Debug(( DBG_L_DUMP_V, "SearchPrinter: cerco printer '%s'\n",
			pcPrinterName ));
	iPrtIdx = 0;
	while(pcPrtIndex[iPrtIdx])
	{
		pstrPrintItem = (_strPrinterItem *)
						((long) pcPrtVector + 
								iPrtIdx * sizeof(_strPrinterItem ));
		Debug(( DBG_L_DUMP_V, "SearchPrinter: printer corrente '%s'\n",
				pstrPrintItem->acPrinterName ));
		if (strcmp(pcPrinterName, pstrPrintItem->acPrinterName) == 0)
		{
			if (iIdx != NULL)
				*iIdx = iPrtIdx;
			Debug(( DBG_L_TRACE, "SearchPrinter: printer '%s' trovata\n",
					pstrPrintItem->acPrinterName ));
			Debug(( DBG_L_STOP_F, "SearchPrinter: Fine funzione\n" ));
			return(pstrPrintItem);
		}
		iPrtIdx++;
	}
	if (iIdx != NULL)
		*iIdx = -1;
	Debug(( DBG_L_TRACE, "SearchPrinter: printer '%s' NON trovata\n",
			pcPrinterName ));
	Debug(( DBG_L_STOP_F, "SearchPrinter: Fine funzione\n" ));
	return(NULL);
}


/*
 *
 *	Function:		GetNextPrinter
 *
 *	Description:	Ritorna il puntatore al prossimo item printer
 *
 *	Parameters:		
 *
 *	Returns:		puntatore alla prossima struttura printer in memoria 
 *					(NULL se non trovata)
 *
 */
_strPrinterItem	*GetNextPrinter()
{
	Debug(( DBG_L_START_F, "GetNextPrinter: Inizio funzione\n" ));
	if (pcPrtIndex[iPrtIdx])
	{
		pstrPrintItem = (_strPrinterItem *)
						((long) pcPrtVector +
								iPrtIdx * sizeof(_strPrinterItem ));
		Debug(( DBG_L_DUMP_V, "SearchPrinter: trovata printer '%s'\n",
					pstrPrintItem->acPrinterName ));
		iPrtIdx++;
	}
	else
	{
		pstrPrintItem = NULL;
	}
	Debug(( DBG_L_STOP_F, "GetNextPrinter: Inizio funzione\n" ));
	return(pstrPrintItem);
}

/*
 *
 *	Function:		AddPrinter
 *
 *	Description:	Aggiunge una stampante alla SHM
 *
 *	Parameters:		pointer alla struttura printer da aggiungere
 *
 *	Returns:		0 per OK, -1 in caso di errore
 *
 */
int		AddPrinter(_strPrinterItem *pstrPrintToAdd)
{
	int		iPrtIdx = 0;

	while(pcPrtIndex[iPrtIdx]  && (iPrtIdx < MAX_PRINTER_DEFINED))
	{
		iPrtIdx++;
	}

	if (iPrtIdx == MAX_PRINTER_DEFINED)
	{
		return(-1);
	}

	pstrPrintItem = (_strPrinterItem *)
					((long) pcPrtVector + iPrtIdx * sizeof(_strPrinterItem ));
	
	memcpy(pstrPrintItem, pstrPrintToAdd, sizeof(_strPrinterItem ));

	pstrPrintItem->lRepoRootOffset = (long) (sizeof(_strReportItem) * 
											 MAX_REPO_QUEUE * iPrtIdx);

	pcPrtIndex[iPrtIdx] = 1;
	pstrPrintItem->cRptQueueLen = 0;
	return(0);
}

/*
 *
 *	Function:		AddReport
 *
 *	Description:	Aggiunge un report alla stampante indicata
 *
 *	Parameters:		pointer alla struttura report da aggiungere,
 *					id della stampante
 *
 *	Returns:		0 per OK, -1 in caso di errore
 *
 */
int		AddReport(_strReportItem *pstrReportAdd, char *pcPrinterName)
{
	int		iRptQueue = 0;
	int		iRPrtIdx;
	_strReportItem	*pstrReportLocal;
	static 	_strPrinterItem	*pstrPrintLocal;

	Debug(( DBG_L_START_F, "AddReport: Inizio funzione\n" ));

	if ((pstrPrintLocal = SearchPrinter(pcPrinterName, &iPrtIdx)) == NULL)
	{
		Debug(( DBG_L_STOP_F, "AddReport: fine funzione\n" ));
		return(-1);
	}
	Debug(( DBG_L_DUMP_V, 
			"AddReport: indice trovato per %s %d\n",
			pcPrinterName, iPrtIdx ));


	/*
	 * Per il momento non tengo conto della priorita' 
	 */
	if (pstrPrintLocal->cRptQueueLen >= (char) MAX_REPO_QUEUE)
	{
		/*
		 * Non c'e' piu' spazio in coda.
		 */
		Debug(( DBG_L_STOP_F, 
				"AddReport: non c'e' piu' spazio in coda. Fine funzione\n" ));
		return(-1);
	}

	Debug(( DBG_L_DUMP_V,
			"AddReport: pstrReportRoot %ld - offset %ld\n",
			apstrReportList[iPrtIdx].pstrReportRoot,
			((int) sizeof(_strReportItem)) * 
				(int) pstrPrintLocal->cRptQueueLen ));

	pstrReportLocal = (_strReportItem  *)
					  ((long) apstrReportList[iPrtIdx].pstrReportRoot +
					   		((int) sizeof(_strReportItem)) * 
							   (int) pstrPrintLocal->cRptQueueLen);

	Debug(( DBG_L_DUMP_V,
			"AddReport: report pointer settato a %ld (offset %ld)\n",
			pstrReportLocal, (long) pstrReportLocal - (long) pcRptMatrix ));
	memcpy(pstrReportLocal, pstrReportAdd, sizeof(_strReportItem ));
	pstrPrintLocal->cRptQueueLen++;

	Debug(( DBG_L_STOP_F, "AddReport: Fine funzione\n" ));
	return(pstrPrintLocal->cRptQueueLen);
}

/*
 *
 *	Function:		ResetReportList
 *
 *	Description:	Risetta il puntatore all'array dei report per la stampante
 *					richiesta
 *
 *	Parameters:		nome della stampante
 *
 *	Returns:		void
 *
 */
void	ResetReportList(char *pcPrinterName)
{
	_strPrinterItem	*pstrPrintLocal;
	int		iPrtIdx;
	
	if ((pstrPrintLocal = SearchPrinter(pcPrinterName, &iPrtIdx)) == NULL)
	{
		return;
	}

	apstrReportList[iPrtIdx].pstrReportCurr =
								apstrReportList[iPrtIdx].pstrReportRoot;
	apstrReportList[iPrtIdx].iCurrentIdx = 0;
}


/*
 *
 *	Function:		SearchReport
 *
 *	Description:	Cerca il report sulla stampante passati per parametro
 *
 *	Parameters:		report id, nome stampante
 *
 *	Returns:		puntatore al report se esiste / NULL viceversa
 *
 */
_strReportItem	*SearchReport(char *pcReportId, char *pcPrinterName)
{
	char	*ptrToSlash;
	int		iPrtIdx;
	_strPrinterItem	*pstrPrintLocal;
	static	_strReportItem	*pstrReportLocal;
	
	if ((pstrPrintLocal = SearchPrinter(pcPrinterName, &iPrtIdx)) == NULL)
	{
		return(NULL);
	}

	/*
	 * Risetto il pointer alla lista dei report per la stampante richiesta.
	 */
	ResetReportList(pcPrinterName);

	for(; 
		apstrReportList[iPrtIdx].iCurrentIdx < pstrPrintLocal->cRptQueueLen;
		apstrReportList[iPrtIdx].iCurrentIdx++)
	{
		apstrReportList[iPrtIdx].pstrReportCurr =
						(_strReportItem *)
						((long) apstrReportList[iPrtIdx].pstrReportRoot + 
							((int) sizeof(_strReportItem)) * 
							apstrReportList[iPrtIdx].iCurrentIdx);

		Debug(( DBG_L_TRACE,
				"SearchReport: report path '%s'\n",
				apstrReportList[iPrtIdx].pstrReportCurr->acReportPath ));

		ptrToSlash = 
				strrchr(apstrReportList[iPrtIdx].pstrReportCurr->acReportPath,
						 '/');

		if (ptrToSlash != NULL)
		{
			ptrToSlash++;
			if (strcmp(ptrToSlash, pcReportId) == 0)
				break;
		}
	}

	if (apstrReportList[iPrtIdx].iCurrentIdx == 
										(int) pstrPrintLocal->cRptQueueLen)
	{
		return(NULL);
	}

	return(apstrReportList[iPrtIdx].pstrReportCurr);
}


/*
 *
 *	Function:		DeleteReportFromQueue
 *
 *	Description:	Cancella il report dalla stampante passata per parametro
 *
 *	Parameters:		report id, nome stampante
 *
 *	Returns:		0 per OK, -1 per errore 
 *
 */
int	DeleteReportFromQueue(char *pcReportId, char *pcPrinterName)
{
	int		iPrtIdx;
	int		iRptIdx = 0;
	char	*ptrToSlash;
	_strPrinterItem	*pstrPrintLocal;
	_strReportItem	*pstrRepo;

	if ((pstrPrintLocal = SearchPrinter(pcPrinterName, &iPrtIdx)) == NULL)
	{
		return(-1);
	}

	/*
	 * Risetto il pointer alla lista dei report per la stampante richiesta.
	 */
	ResetReportList(pcPrinterName);

	for(; 
		apstrReportList[iPrtIdx].iCurrentIdx < pstrPrintLocal->cRptQueueLen;
		apstrReportList[iPrtIdx].iCurrentIdx++)
	{
		apstrReportList[iPrtIdx].pstrReportCurr =
						(_strReportItem *)
						((long) apstrReportList[iPrtIdx].pstrReportRoot + 
							((int) (sizeof(_strReportItem)) * 
									apstrReportList[iPrtIdx].iCurrentIdx ));

		ptrToSlash = 
				strrchr(apstrReportList[iPrtIdx].pstrReportCurr->acReportPath,
						 '/');

		if (ptrToSlash != NULL)
		{
			ptrToSlash++;
		}
		else
		{
			ptrToSlash = apstrReportList[iPrtIdx].pstrReportCurr->acReportPath;
			continue;
		}

		Debug(( DBG_L_TRACE,
				"DeleteReportFromQueue: report cercato '%s' trovato '%s'\n",
				pcReportId, ptrToSlash ));

		if (strcmp(ptrToSlash, pcReportId) != 0)
			continue;

		iRptIdx = apstrReportList[iPrtIdx].iCurrentIdx;
		pstrRepo = apstrReportList[iPrtIdx].pstrReportCurr;
		Debug(( DBG_L_TRACE,
				"DeleteReportFromQueue: offset da cancellare %ld (size %ld)\n",
				(long) pstrRepo, sizeof(_strReportItem) ));
		Debug(( DBG_L_TRACE,
				"DeleteReportFromQueue: report su coda %d - corrente %d\n",
				pstrPrintLocal->cRptQueueLen, iRptIdx ));
		for(; iRptIdx < pstrPrintLocal->cRptQueueLen - 1; iRptIdx++) 
		{
			Debug(( DBG_L_TRACE,
					"DeleteReportFromQueue: offset in copia %ld\n",
					(long) pstrRepo ));
			memcpy(pstrRepo, 
					(char  *) ((long) pstrRepo + sizeof(_strReportItem)), 
					sizeof(_strReportItem ));
			pstrRepo = (_strReportItem *)
					   ((long) pstrRepo + sizeof(_strReportItem ));
		}
		memset(pstrRepo, '\0', sizeof(_strReportItem ));
		pstrPrintLocal->cRptQueueLen--;
		return(0);
	}

	return(-1);
}


/*
 *
 *	Function:		MoveReportBetweenQueue
 *
 *	Description:	Muove un report da una coda stampante ad un'altra
 *
 *	Parameters:		report id, stampante vecchia, stampante nuova
 *
 *	Returns:		0 per OK, -1 per errore 
 *
 */
int		MoveReportBetweenQueue(char *pcReportId, char *pcOldPrinterName,
							   char *pcNewPrinterName)
{
	int		iPrtIdx;
	int		iRptIdx = 0;
	_strPrinterItem	*pstrPrintLocal;
	_strReportItem	*pstrReportLocal;
	
	if ((pstrPrintLocal = SearchPrinter(pcNewPrinterName, 
										  &iPrtIdx)) == NULL)
	{
		return(-1);
	}
	
	if (pstrPrintLocal->cRptQueueLen >= (char) MAX_REPO_QUEUE)
	{
		return(-1);
	}

	if ((pstrReportLocal = SearchReport(pcReportId,
										  pcOldPrinterName)) == NULL)
	{
		return(-1);
	}

	if (AddReport(pstrReportLocal, pcNewPrinterName) < 0)
	{
		return(-1);
	}

	DeleteReportFromQueue(pcReportId, pcOldPrinterName);
	return(0);
}


/*
 *
 *	Function:		GetNextReport
 *
 *	Description:	Ritorna il puntatore al prossimo report per una printer
 *
 *	Parameters:		nome stampante su cui cercare report
 *
 *	Returns:		pointer al report se esiste, NULL viceversa 
 *
 */
_strReportItem	*GetNextReport(char *pcPrinterName)
{
	int		iPrtIdx;
	int		iRptIdx = 0;
	_strPrinterItem	*pstrPrintLocal;
	static	_strReportItem	*pstrReportLocal;
	
	if ((pstrPrintLocal = SearchPrinter(pcPrinterName, &iPrtIdx)) == NULL)
	{
		return(NULL);
	}

	if (apstrReportList[iPrtIdx].iCurrentIdx == 
									(int) pstrPrintLocal->cRptQueueLen)
	{
		/*
		 * Fine della lista
		 */
		ResetReportList(pcPrinterName);
		return(NULL);
	}

	pstrReportLocal = apstrReportList[iPrtIdx].pstrReportCurr;
	apstrReportList[iPrtIdx].pstrReportCurr =
							(_strReportItem *)
							   ((long) apstrReportList[iPrtIdx].pstrReportCurr +
							    ((int) sizeof(_strReportItem) ));
	apstrReportList[iPrtIdx].iCurrentIdx++;
	return(pstrReportLocal);
}


/*
 *
 *	Function:		MemoryUnlock
 *
 *	Description:	Setta il semaforo per memory free
 *
 *	Parameters:		
 *
 *	Returns:		0 per OK / codice d'errore 
 *
 */
int		MemoryUnlock()
{
	if (semop(iSemid, &semOpUnlock[0], 1) < 0)
	{
		return(errno);
	}
	return(0);
	
}

/*
 *
 * Nome procedura:	MemoryLock
 *
 * Descrizione:		Setta il semaforo per memory lock
 *
 * Parametri:		void
 *
 * Valore ritorno:	0 per OK / codice d'errore
 *
 */
int		MemoryLock()
{
	if (semop(iSemid, &semOpLock[0], 2) < 0)
	{
		return(errno);
	}
	return(0);
}