/* Global includes */
#include	<stdio.h> 
#include	<stdlib.h>        
#include	<errno.h>
#include	<signal.h>
#include	<unistd.h>
#include	<string.h>
#include	<ctype.h>
#include	<netdb.h>

/* Local module includes */
#include	"spooler.h"
#include	"libshm.h"
#include	"cfgmng.h"
#include	"debug.h"
#include	"gotspool.h"

/* Costants */
#ifdef	DEBUG
#define	OPTIONS			"h:d:"
#else
#define	OPTIONS			"h:"
#endif

#define			MPSERV		"gcspool_mp"

#define	MAX_VARS			12

#define	FLG_NO_PARMS		0
#define	FLG_HOMEDIR			1
#define	FLG_DEBUG			2

/* Macroes */               

/* Local struct & new type definition */

/* External Functions */

/* External Variables */

/* Global Variables */
struct	servent	gstrLPServEnt;
struct	servent	gstrPMServent;
int		giDbgLevel;
long	glRetValue;
char	*gpcPrgName;
char	gcPrgFlags;
char	gacHomeDir[1024];
char	gacGCUXHome[1024];
char	gcFlgReportQueued;

/* Local Variables */


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
		signal(SIGUSR1, SigTrap);
		giDbgLevel = (giDbgLevel - 1 >= 0 ? giDbgLevel - 1 : 0);
		DebugChgLevel(( giDbgLevel ));
		break;
	
	case SIGUSR2:
		signal(SIGUSR2, SigTrap);
		giDbgLevel++;
		DebugChgLevel(( giDbgLevel ));
		break;

	case SIGHUP:
		gcFlgReportQueued = TRUE;
		signal(SIGHUP, SigTrap);
		break;

	case SIGALRM:
		gcFlgReportQueued = TRUE;
		signal(SIGALRM, SigTrap);
		break;
	}
	Debug(( DBG_L_STOP_F, "SigTrap: Fine funzione\n" ));
}


/*
 *
 * Nome procedura:	GetOptions
 *
 * Descrizione:		valorizza i parametri passati da shell nelle rispettive
 *					variabili
 *
 * Parametri:		argc, argv
 *
 * Valore ritorno:	nessuno.
 *
 */
void	GetOptions(int iArgc, char **apcArgv)
{
	int		iOptionIndex;

	gcPrgFlags = FLG_NO_PARMS;

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
 * Nome procedura:	main
 *
 * Descrizione:		configura e lancia una istanza dell'applicazione 
 *
 * Parametri:		argc, argv
 *
 * Valore ritorno:	segnale di terminazione
 *
 */
int		main(int argc, char **argv)
{
	int		iCount = 0;
	int		iPid;
	char	acBufDebugFile[256];
	char	*pcToDot;
	char	cFlgJustStarted = TRUE;
	FILE	*pFLock;
	struct	servent	*pstrServEnt;

	gpcPrgName = argv[0];

	/*
	 * Verifica della correttezza dei parametri passati
	 */
	GetOptions(argc, argv);

	/*
	 * Attivo il debugging (se selezionato)
	 */
	if ((pcToDot = strchr(gpcPrgName, '.')) != NULL)
	{
		*pcToDot = '\0';
	}

	signal(SIGINT, SIG_IGN);
	signal(SIGHUP, SIG_IGN);
	signal(SIGUSR1, SIG_IGN);
	signal(SIGUSR2, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);

	memset(acBufDebugFile, '\0', sizeof(acBufDebugFile ));
	sprintf(acBufDebugFile, "%s.%05d", gpcPrgName, getpid());
	while(acBufDebugFile[iCount] != '\0')
	{
		acBufDebugFile[iCount] = toupper(acBufDebugFile[iCount]);
		iCount++;
	}
	DebugInit(( giDbgLevel, acBufDebugFile ));
	// DebugInit(( giDbgLevel, "GCSPOOL.dbg" ));

	Debug(( DBG_L_START_F | DBG_TIMEONLY, "main: inizio funzione.\n" ));

	strcpy(gacGCUXHome, gacHomeDir);

	sprintf(acBufDebugFile, "%s/lock/gcspool", gacGCUXHome);
	if ((pFLock = fopen(acBufDebugFile, "r+")) != NULL)
	{
		fscanf(pFLock, "%d", &iPid);
		Debug(( DBG_L_DUMP_V, "main: Pid = %d\n", iPid ));
		if ((kill(iPid, 0 ) == 0 ) && (iPid != getpid()))
		{
			Debug(( DBG_L_FATAL, 
					"main: Una istanza di GCSpool e' gia' in esecuzione\n" ));
			exit(0);
		}
		fseek(pFLock, 0, SEEK_SET);
		fprintf(pFLock, "%05d", getpid());
		fclose(pFLock);
	}
	else
	{
		Debug(( DBG_L_FATAL, "main: Impossibile aprire lock file '%s'\n",
				acBufDebugFile ));
		exit(errno);
	}


	Debug(( DBG_L_TRACE, "main: inizio gestione SHM\n" ));
	/*
	 * Inizializzazione stampanti e report.
	 * Viene tutto caricato in memoria condivisa.
	 */
	InitSHM((char) 1);
	Debug(( DBG_L_TRACE, "main: inizializzazione stampanti\n" ));
	InitializePrintQueues();
	Debug(( DBG_L_TRACE, "main: inizializzazione completata\n" ));
	Debug(( DBG_L_TRACE, "main: inizializzazione carte\n" ));
	LoadPapers();
	Debug(( DBG_L_TRACE, "main: inizializzazione completata\n" ));


	/*
	 * Servizio di stampa (LPD).
	 */
	if ((pstrServEnt = getservbyname("printer", "tcp")) == NULL)
	{
		Debug(( DBG_L_FATAL, "main: servizio printer non definito\n" ));
		exit(EXIT_BAD_LPD_SERVNAME);
	}
	memcpy(&gstrLPServEnt, pstrServEnt, sizeof (struct servent));


	/*
	 * Servizio di monta carta.
	 */
	if ((pstrServEnt = getservbyname(MPSERV, NULL)) == NULL)
	{
		Debug(( DBG_L_FATAL,
				"main: servizio %s non definito in services\n",
				MPSERV ));
		Debug(( DBG_L_STOP_F, "main: fine funzione\n" ));
		return(0);
	}
	memcpy(&gstrPMServent, pstrServEnt, sizeof (struct servent));


	/*
	 * Fase di spool.
	 */
	signal(SIGALRM, SigTrap);
	signal(SIGUSR1, SigTrap);
	signal(SIGUSR2, SigTrap);
	signal(SIGHUP, SigTrap);
	RenameReports();
	while(1)
	{
		/*
		 * Verifico se vi sono nuovi report da stampare.
		 */
		Debug(( DBG_L_TRACE, "main: caricamento report\n" ));
		/*
		 * La variabile cFlgJustStarted indica se gcspool e' in fase di
		 * start-up oppure gia' operativo.
		 * Nel primo caso, dovra' mettere lo stato HOLD a tutti i report
		 * caricati e lo UNIX-ID a 0
		 */
		LoadWaitingReports(cFlgJustStarted);

		/*
		 * Da questo momento, carica i report con lo stato segnato nel file
		 */
		cFlgJustStarted = FALSE;

		Debug(( DBG_L_TRACE, "main: inizializzazione completata\n" ));

		/*
		 * Verifico lo stato dello spooler di Unix per ciascuna delle
		 * stampanti caricate.
		LpCheckStatus();
		 */

		/*
		 * Verifico se per una stampante libera c'e' qualcosa da stampare e
		 * lo accodo.
		 */
		EnqueueReport();
		
		if (gcFlgReportQueued)
		{
			Debug(( DBG_L_TRACE, "main: ci sono report in coda. sleep 10\n" ));
			alarm(3);
		}
		/*
		else
		{
			alarm(30);
		}
		*/
		
		/*
		 * Sospendo il programma.
		 * Verra' risvegliato da una alarm (se ci sono report in coda) o
		 * da una signal esterna per l'accodamento di un nuovo report.
		 */
		siginterrupt(SIGHUP, 1);
		pause();
		siginterrupt(SIGHUP, 0);

		alarm(0);
	}
	exit(0);
}
