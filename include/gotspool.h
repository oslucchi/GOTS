#ifndef	__GCSPOOL_H
#define	__GCSPOOL_H

/* Other dependecy includes */

/* Prototype */

/* Costants */ 
#ifndef	TRUE
# define	TRUE	1
#endif

#ifndef FALSE
# define	FALSE	(!TRUE)
#endif

#define	EXIT_OK								 0
#define	EXIT_BAD_PARMS						 1
#define	EXIT_CANT_ACCESS					 2
#define	EXIT_BAD_OPENDIR_PRINTERS			 3
#define	EXIT_ALLOCATE_SHM_PRINTERS			 4
#define	EXIT_ATTACH_SHM_PRINTERS			 5
#define	EXIT_PRINTERFILE_CANT_OPEN			 6
#define	EXIT_BAD_PRINTER_FILE				 7
#define	EXIT_REPORTFILE_CANT_OPEN			 8
#define	EXIT_BAD_REPORT_FILE				 9
#define	EXIT_ALLOCATE_SHM_REPORTS			10
#define	EXIT_ATTACH_SHM_REPORTS				11
#define	EXIT_BAD_OPENDIR_REPORTS			12
#define	EXIT_ERROR_GETHOSTNAME				13
#define	EXIT_BAD_LPD_HOSTNAME				14
#define	EXIT_BAD_LPD_SERVNAME				15
#define	EXIT_ERROR_RRESVPORT				16
#define	EXIT_ERROR_LPD_CONNECT				17
#define	EXIT_CREATE_SEMAPHORE				18

#define	MAX_REPO_QUEUE					   300
#define	MAX_PRINTER_DEFINED					30
#define	MAX_PAPER_DEFINED				   100

#define	GCOS_DISPO_OPTION_HOLD			0x0001
#define	GCOS_DISPO_OPTION_KEEP			0x0002
#define	GCOS_DISPO_OPTION_DELE			0x0004

#define	LPD_SUBMIT_REPORT				'\002'
#define	LPD_SUBMIT_DATA_FILE			'\003'
#define	LPD_QUERY_STATUS				'\003'

#define	LPD_STATUS_QUEUEING_ENABLED		0x0001
#define	LPD_STATUS_PRINTING_ENABLED		0x0002
#define	LPD_STATUS_PRINTER_UP			0x0004
#define	LPD_STATUS_PRINTING_REPORT		0x0008

#define	SHM_KEY							  2048
#define	SHM_PRT_KEY     				  2049
#define	SHM_RPT_KEY     				  2050

/* Macroes */
#define	SPACE( c )	(c == '\0' ? ' ' : c)

/* External functions */
void	InitializePrintQueues();
void	LoadWaitingReports(char);

/* Esternal variables */

/* New datatype */
typedef struct  __strReportItem {
	char	acCfgFilePath[64];	// Path del file di configurazione
	char	acReportName[8];	// Nome report da programma
	char	acJobName[9];		// Nome del Job che ha generato il report
	char	acJobNum[9];		// Job che ha generato il report
	char	acJobSon[9];		// Son del job che ha generato il report
	char	acReportPath[62];	// Path del file dati da stampare
	char	acUserName[16];		// Nome dell'utente che ha generato il report
	char	acPaper[8];			// Tipo di carta da usare
	char	acDevice[7];		// Device richiesto per la stampa
	char	acUnixId[31];		// Identificativo dato alla stampa 
								// dallo spooler unix
	char	cPrio;				// Priorita' assegnata al report
	short	sSheetOpt;			// Opzioni sulla carta
	short	sSkipOpt;			// Opzioni sullo skip
	short	sDispoOpt;			// Disposizione del report H/K/D
	short	sNumOfCopies;		// Numero di copie
	long	lCreationTime;		// Data di creazione
	long	lLastPrint;			// Data ultima stampa
	int		iLines;				// Numero di linee nel report
	struct  __strReportItem	*pstrRepoNext;	// Prossimo report in coda
} _strReportItem;

typedef struct  __strPrinterItem {
	char	acPrinterName[8];	// Nome stampante
	char	acPrintingRpt[8];	// Nome report in corso di stampa
	char	acPrintingRptJ[8];	// JON report in corso di stampa
	char	acPrintingRptS[8];	// SON report in corso di stampa
	char	acCurrentPaper[8];	// Tipo di carta da usare
	char	acUnixDevice[32];	// Device UNIX corrispondente
	char	acDescription[64];	// Descrizione della stampante
	char	acUnixLpName[27];	// Nome device lp UNIX corrispondente
	char	cRptQueueLen;		// Numero di report accodati sulla stampante
	long	lUnixLpStatus;		// Stato della stampante
	long	lRepoRootOffset;	// Root della coda di stampa
	long	lIpAddress;			// IP per connessione LPD
	char	cMntPaperSent;
	char	acMasterStation[39];// Pc master per la stampante
	char	acLPI8[8];			// Sequenza per 8lpi
	char	acLPI6[8];			// Sequenza per 6lpi
	char	acLpdMaster[40];	// Stazione master per LPD 
} _strPrinterItem;

typedef struct  __strPaperItem {
	char	acPaperName[8];		// Nome stampante
	int		iNumOfLines;		// Numero di linee
	char	acLinesPerInch[8];	// Numero di linee per pollice
	int		iInchsPerPage;		// Numero di pollici per pagina
	char	acReset[8];			// Numero di linee per pollice
} _strPaperItem;

typedef	struct	__strRptMng	{
	struct  __strRptMng	*pstrNext;
	long	lPid;
} _strRptMng;

typedef struct  __strReportList {
	int				iCurrentIdx;
	_strReportItem  *pstrReportRoot;
	_strReportItem  *pstrReportCurr;
} _strReportList;

#endif