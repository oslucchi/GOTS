/* Global includes */
#include	<stdio.h>
#include	<errno.h>
#include	<signal.h>
#include	<unistd.h>
#include	<string.h>
#include	<stdlib.h>
#include	<ctype.h>
#include	<sys/types.h>
#include	<sys/wait.h>

/* Local module includes */
#include	"cfgmng.h"
#include	"gotspool.h"
#include	"debug.h"

/* Costants */
#ifdef	DEBUG
#define	OPTIONS			"h:d:"
#else
#define	OPTIONS			"h:"
#endif

#define	MAX_VARS			12

#define	FLG_NO_PARMS		0
#define	FLG_HOMEDIR			1
#define	FLG_DEBUG			2

/* Macroes */               

/* Local struct & new type definition */

/* External Functions */
extern	int		Accept(void);
extern	void	OpenSocket(void);
extern	void	HandleIncomingConn(void);

/* External Variables */

/* Global Variables */
int		giDbgLevel;
long	glRetValue;
char	*gpcPrgName;
char	gcPrgFlags;
char	gacHomeDir[128];
char	gacGCUXHome[128];
char	gcFlgReportQueued;
_strRptMng	*pstrRptMngRoot, *pstrRptMngCurr;

/* Local Variables */

/*
 *
 *	Function:		ShutDownClient
 *
 *	Description:	invia lo shutdown ai figli spawnati
 *
 *	Parameters:		
 *
 *	Returns:		void
 *
 */
void	ShutDownClient()
{
	_strRptMng	*pstrRptMngApp, *pstrRptMngFree;

	Debug(( DBG_L_START_F, "ShutDownClient: Inizio funzione\n" ));
	pstrRptMngApp = pstrRptMngRoot;

	while(pstrRptMngApp != (_strRptMng *) NULL)
	{
		Debug(( DBG_L_TRACE,
				"ShutDownClient: Invio kill a %d\n", pstrRptMngApp->lPid ));
		pstrRptMngFree = pstrRptMngApp;
		pstrRptMngApp = pstrRptMngApp->pstrNext;
		kill(pstrRptMngFree->lPid, SIGTERM);
		free(pstrRptMngFree);
	}
	pstrRptMngRoot = pstrRptMngCurr = (_strRptMng *) NULL;

	Debug(( DBG_L_STOP_F, "ShutDownClient: Fine funzione\n" ));
}


/*
 *
 *	Function:		AddClient
 *
 *	Description:	mantiene traccia dei processi figli spawnati
 *
 *	Parameters:		process id del processo figlio
 *
 *	Returns:		void
 *
 */
void	AddClient(int iPid)
{
	Debug(( DBG_L_START_F, "AddClient: Inizio funzione\n" ));

	if (pstrRptMngRoot == (_strRptMng *) NULL)
	{
		pstrRptMngRoot = (_strRptMng *) malloc(sizeof (_strRptMng));
		if (pstrRptMngRoot == (_strRptMng *) NULL)
			return;
		pstrRptMngCurr = pstrRptMngRoot;
	}
	else
	{
		pstrRptMngCurr->pstrNext
					= (_strRptMng *) malloc(sizeof (_strRptMng));
		if (pstrRptMngCurr->pstrNext == (_strRptMng *) NULL)
			return;
		pstrRptMngCurr = pstrRptMngCurr->pstrNext;
	}
	pstrRptMngCurr->lPid = (long) iPid;
	pstrRptMngCurr->pstrNext = (_strRptMng *) NULL;
	Debug(( DBG_L_TRACE, "AddClient: aggiunto processo %d\n", iPid ));

	Debug(( DBG_L_STOP_F, "AddClient: Fine funzione\n" ));
	return;
}


/*
 *
 *	Function:		DropClient
 *
 *	Description:	elimina dalla coda dei figli i processi chiusi
 *
 *	Parameters:		process id del processo figlio
 *
 *	Returns:		void
 *
 */
void	DropClient()
{
	_strRptMng	*pstrRptMngApp, *pstrRptMngFree;
	int			iPid, iStatus;

	Debug(( DBG_L_START_F, "DropClient: Inizio funzione\n" ));
	iPid = wait(&iStatus);

	Debug(( DBG_L_TRACE, "DropClient: morto processo %d\n", iPid ));

	if ((pstrRptMngApp = pstrRptMngRoot ) == (_strRptMng *) NULL)
	{
		return;
	}

	/*
	 * Il processo morto e' la root della lista
	 */
	if (pstrRptMngRoot->lPid == iPid)
	{
		pstrRptMngApp = pstrRptMngRoot->pstrNext;
		free(pstrRptMngRoot);
		pstrRptMngRoot = pstrRptMngApp;
		if (pstrRptMngRoot == (_strRptMng *) NULL)
		{
			pstrRptMngCurr = (_strRptMng *) NULL;
		}
		Debug(( DBG_L_TRACE, "DropClient: elminata testa lista\n" ));
	}
	else
	{
		while(pstrRptMngApp->pstrNext != (_strRptMng *) NULL)
		{
			if (pstrRptMngApp->pstrNext->lPid == iPid)
				break;
			pstrRptMngApp = pstrRptMngApp->pstrNext;
		}

		if (pstrRptMngApp->pstrNext == (_strRptMng *) NULL)
		{
			/*
			 * Il processo morto non e' nella lista.
			 */
			return;
		}

		if (pstrRptMngApp->pstrNext == pstrRptMngCurr)
		{
			/*
			 * Il processo morto e' l'ultimo della lista
			 */
			free(pstrRptMngCurr);
			pstrRptMngCurr = pstrRptMngApp;
			pstrRptMngApp->pstrNext = (_strRptMng *) NULL;
			Debug(( DBG_L_TRACE, "DropClient: elminata coda lista\n" ));
		}
		else
		{
			/*
			 * Il processo morto e' in mezzo alla lista
			 */
			pstrRptMngFree = pstrRptMngApp->pstrNext;
			pstrRptMngApp->pstrNext = pstrRptMngFree->pstrNext;
			free(pstrRptMngFree);
			Debug(( DBG_L_TRACE, "DropClient: elminata item lista\n" ));
		}
	}
	Debug(( DBG_L_STOP_F, "DropClient: Fine funzione\n" ));
}



/*
 *
 *	Function:		SigTrap
 *
 *	Description:	Signal Handler
 *
 *	Parameters:		codice del segnale
 *
 *	Returns:		void
 *
 */
void	SigTrap(int iSigCode)
{
	Debug(( DBG_L_START_F, "SigTrap: Inizio funzione\n" ));
	Debug(( DBG_L_TRACE,
			"SigTrap: ricevuta signal %d\n", iSigCode ));
	switch(iSigCode)
	{
	case SIGUSR1:
		ShutDownClient();
		exit(0);
		break;
	
	case SIGUSR2:
		signal(SIGUSR2, SigTrap);
		giDbgLevel++;
		DebugChgLevel((giDbgLevel ));
		break;

	case SIGCHLD:
		DropClient();
		signal(SIGCHLD, SigTrap);
		break;

	case SIGTERM:
		exit(0);
		break;

	case SIGHUP:
		signal(SIGHUP, SigTrap);
		break;

	case SIGALRM:
		signal(SIGALRM, SigTrap);
		break;
	}
	Debug(( DBG_L_STOP_F, "SigTrap: Fine funzione\n" ));
}



/*
 *
 *	Function:		GetOptions
 *
 *	Description:	valorizza i parametri passati da shell nelle rispettive
 *					variabili
 *
 *	Parameters:		argc, argv
 *
 *	Returns:		void
 *
 */
void	GetOptions(int iArgc, char **apcArgv)
{
	int		iOptionIndex;

	gcPrgFlags = FLG_NO_PARMS;
	giDbgLevel = 20;

	while((iOptionIndex = getopt(iArgc, apcArgv, OPTIONS)) != -1)
	{
		switch(iOptionIndex)
		{
		case 'h':
			gcPrgFlags |= FLG_HOMEDIR;
			strcpy(gacHomeDir, optarg);
			break;

		case 'd':
			gcPrgFlags |= FLG_DEBUG;
			giDbgLevel = atoi(optarg);
			break;
		default:
			exit(EXIT_BAD_PARMS);
		}
	}

	if (gcPrgFlags == FLG_NO_PARMS)
	{
		fprintf(stderr, "usage: %s -h home_dir\n", gpcPrgName);
		exit(EXIT_BAD_PARMS);
	}

	if (access(gacHomeDir, X_OK | R_OK ) == -1)
	{
		fprintf(stderr, "%s: home dir non accessibile\n", gpcPrgName);
		exit(EXIT_CANT_ACCESS);
	}
}




/*
 *
 *	Function:		main
 *
 *	Description:	
 *
 *	Parameters:		argc, argv
 *
 *	Returns:		o per OK
 *
 */
int		main(int argc, char **argv)
{
	int		iRet;
	int		iCount = 0;
	char	acBufDebugFile[256];
	char	*pcToDot;
	FILE	*pFPid;

	gpcPrgName = argv[0];

	GetOptions(argc, argv);

	/*
	 * Attivo il debugging (se selezionato)
	 */
	if ((pcToDot = strchr(gpcPrgName, '.')) != NULL)
	{
		*pcToDot = '\0';
	}

	signal(SIGINT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	signal(SIGHUP, SIG_IGN);
	signal(SIGUSR1, SIG_IGN);
	signal(SIGUSR2, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGCHLD, SigTrap);

	memset(acBufDebugFile, '\0', sizeof(acBufDebugFile ));
	sprintf(acBufDebugFile, "%s.%05d", gpcPrgName, getpid());
	while(acBufDebugFile[iCount] != '\0')
	{
		acBufDebugFile[iCount] = toupper(acBufDebugFile[iCount]);
		iCount++;
	}
	DebugInit(( giDbgLevel, acBufDebugFile ));

	Debug(( DBG_L_START_F | DBG_TIMEONLY, "main: inizio funzione.\n" ));

	strcpy(gacGCUXHome, gacHomeDir);

	sprintf(acBufDebugFile, "%s/lock/gcrptmng", gacGCUXHome);

	if ((pFPid = fopen(acBufDebugFile, "w")) != NULL)
	{
		fprintf(pFPid, "%05d", getpid());
		fclose(pFPid);
	}


	/*
	 * Inizializzazione ed apertura del servizio.
	 */
	OpenSocket();
	pstrRptMngRoot = (_strRptMng *) NULL;

	/*
	 * Il server si mette in attesa di chiamate entranti.
	 */
	signal(SIGUSR1, SigTrap);
	while(1)
	{
		iRet = Accept();

		if (iRet == 0)
		{
			/*
			 * Ricevuta una chiamata entrante.
			 */
			HandleIncomingConn();
		}
	}
	exit(0);
}
