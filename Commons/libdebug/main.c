/* Global includes */
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include <sys/times.h>
#include <unistd.h>

/* Local module includes */
#include "debug.h"

/* Costants */

/* Macroes */               

/* Local struct & new type definition */

/* External Functions */
char	*getenv();

/* External Variables */

/* Global Variables */

/* Local Variables */
static	int		iDbgLevel = 0;
static	long	lStartTime;
static	FILE	*pFDebug;
static	struct	tms	tmb;


/*
 *
 *	Function:		_DEBUGChgLevel
 *
 *	Description:	Cambia il valore del livello di debug secondo il parametro 
 *					ricevuto
 *
 *	Parameters:		Valore da sommare algebricamente al livello di debug
 *
 *	Returns:		void
 *
 */
void	_DEBUGChgLevel(int iAdd)
{
	iDbgLevel = (iDbgLevel + iAdd > 0 ? iDbgLevel + iAdd : 0);
}


/*
 *
 *	Function:		_DEBUGInit
 *
 *	Description:	Inizializza il debug aprendo il file specificato e impo-
 *					stando le variabili di livello secondo i parametri ricevuti
 *
 *	Parameters:		Massimo livello di Debug (iLevel),
 *					path del file per il debug
 *
 *	Returns:		0 on error, 1 on successfully opened log
 *
 */
int		_DEBUGInit(int iLevel, char *pcFilePath)
{
	char	*ptrToSlash;
	char	acFilePath[120];

	iDbgLevel = iLevel;

	if (pcFilePath == NULL)
	{
		pFDebug = stderr;
	}
	else
	{
		int	dfd;

		if (getenv("DBGPATH") != NULL)
		{
			if ((ptrToSlash = strrchr(pcFilePath, '/')) != NULL)
			{
				ptrToSlash++;
				sprintf(acFilePath, "%s/%s", getenv("DBGPATH"), ptrToSlash);
			}
			else
			{
				sprintf(acFilePath, "%s/%s", getenv("DBGPATH"), pcFilePath);
			}
		}
		else
		{
			strcpy(acFilePath, pcFilePath);
		}

		if ((dfd = open(acFilePath,
						  O_WRONLY|O_CREAT|O_TRUNC|O_APPEND|O_NDELAY,
						  0666)) == -1)
		{
			return(0);
		}

		pFDebug = fdopen(dfd, "a");
# ifdef UNBUF
		setbuf(pFDebug, NULL);
# endif
	}

	lStartTime = times(&tmb);
	return(1);
}

/*
 *
 *	Function:		_DEBUGEnd()
 *
 *	Description:	Chiude il file di log
 *
 *	Parameters:		void
 *
 *	Returns:		void
 *
 */
void	_DEBUGEnd()
{
	if (pFDebug != stderr)
		fclose(pFDebug);
}

/*
 *
 *	Function:		_DEBUG
 *
 *	Description:	Effettua fisicamente la scrittura sul file di lof
 *					se il livello di debug specificato nel comando e'
 *					<= di quello attualmente imposta per la libreria 
 *
 *	Parameters:		varargs, primo parametro livello di debug richiesto
 *					per il logging
 *
 *	Returns:		void
 *
 */
void	_DEBUG(int r_level, ...)
{
	char	*fmt;
	struct tm	*ptm;
	va_list	dargs;

	va_start(dargs, r_level);
	fmt = va_arg(dargs, char *);

	if ((r_level & ~DBG_FLAGS) > iDbgLevel)
	{
		va_end(dargs);
		return;
	}

	if (pFDebug == NULL)
	{
		va_end(dargs);
		return;
	}
	
	if (!(r_level & DBG_SIL))
	{
		time_t	lTime;
		char	*ctime();

		fprintf(pFDebug, "%02d - ", r_level & ~DBG_FLAGS);
		lTime = time((time_t *) 0);
		ptm = localtime(&lTime);

		if (r_level & DBG_DATE)
		{
			fprintf(pFDebug,
					 "%02d/%02d/%02d %02d:%02d:%02d - ", 
					 ptm->tm_mday, ptm->tm_mon + 1, ptm->tm_year, 
					 ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
		}
			
		if (r_level & DBG_DATEONLY)
		{
			fprintf(pFDebug,
					 "%02d/%02d/%02d - ",
					 ptm->tm_mday, ptm->tm_mon + 1, ptm->tm_year);
		}
			
		if (r_level & DBG_TIMEONLY)
		{
			fprintf(pFDebug,
					 "%02d:%02d:%02d - ",
					 ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
		}
			
		if (r_level & DBG_TIME)
		{
			fprintf(pFDebug, "%ld - ", times(&tmb) - lStartTime);
		}
	}
	vfprintf(pFDebug, fmt, dargs);

	fflush(pFDebug);
	va_end(dargs);
}

/*
 *
 *	Function:		DEBUGConsole
 *
 *	Description:	logga il messaggio su /dev/console se esiste 
 *
 *	Parameters:		varargs, primo parametro formato di stampa
 *
 *	Returns:		void
 *
 */
void	DEBUGConsole(char * fmt, ...)
{
	char	acFileName[256];
	char	acTmpBuf[256];
	char	acOutBuf[256];
	va_list	dargs;
	int		iTtyOut;
	int		iFileOut;
	time_t	lTime;
	struct	tm	*pTm;

	va_start(dargs, fmt);
	(void) vsprintf(acTmpBuf, fmt, dargs);
	va_end(dargs);

	time(&lTime);
	pTm = localtime(&lTime);

	sprintf(acOutBuf, 
			 "%02d/%02d/%02d %02d:%02d:%02d - %s", 
			 pTm->tm_year, pTm->tm_mon + 1, pTm->tm_mday,
			 pTm->tm_hour, pTm->tm_min, pTm->tm_sec, acTmpBuf);

	if ((iTtyOut = open("/dev/console", 
						  O_WRONLY|O_CREAT|O_APPEND|O_NDELAY)) == -1)
	{
		iTtyOut = 3;
	}
	write(iTtyOut, acOutBuf, strlen(acOutBuf));

	if (iTtyOut != 3)
	{
		close(iTtyOut);
	}

	if (getenv("DBGPATH") != NULL)
	{
		sprintf(acFileName, "%s/mms.console", getenv("DBGPATH"));
	}
	else
	{
		sprintf(acFileName, "/tmp/mms.console");
	}
	if ((iFileOut = open(acFileName,
						   O_WRONLY|O_CREAT|O_APPEND|O_NDELAY,
						   0666)) == -1)
	{
		return;
	}

	write(iFileOut, acOutBuf, strlen(acOutBuf));
	close(iFileOut);
}
