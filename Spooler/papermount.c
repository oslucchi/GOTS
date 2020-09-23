/* Global includes */
#include	<stdio.h>
#include	<errno.h>
#include	<sys/types.h>
#include	<sys/socket.h>
#include	<netinet/in.h>
#include	<netdb.h>
#include	<signal.h>
#include	<unistd.h>
#include	<string.h>

/* Local module includes */
#include	"debug.h"

/* Costants */

/* Macroes */               

/* Local struct & new type definition */

/* External Functions */
void	SigTrap();

/* External Variables */
extern	struct	servent	gstrPMServent;

/* Global Variables */

/* Local Variables */
static	struct	sockaddr_in	strSockAddr;


/*
 *
 *	Function:		SendPaperMountRequest
 *
 *	Description:	Invia richiesta di monta carta all'host responsabile
 *					per la stampante richiesta
 *
 *	Parameters:		nome dell'host,
 *					tipco carta,
 *					nome della stampante
 *
 *	Returns:		0 per errore, 1 per OK.
 *
 */
int		SendPaperMountRequest(char *pcMasterStation, 
							  char *pcPaper,
							  char *pcPrinterName)
{
	int		iRetValue;
	int		iSock;
	char	acBuf[40];
	struct	hostent	*pstrHE;

	Debug(( DBG_L_START_F, "SendPaperMountRequest: inizio funzione\n" ));

	Debug(( DBG_L_DUMP_V,
			"SendPaperMountRequest: monta carta %s a %s di %s\n",
			pcPaper, pcPrinterName, pcMasterStation ));

	if ((iSock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		Debug(( DBG_L_FATAL,
				"SendPaperMountRequest: Errore %d in allocazione socket\n",
				errno ));
		Debug(( DBG_L_STOP_F, "SendPaperMountRequest: fine funzione\n" ));
		return(0);
	}

	Debug(( DBG_L_TRACE,
			"SendPaperMountRequest: connetto host %s su porta %d\n",
			pcMasterStation, ntohs( gstrPMServent.s_port ) ));

	strSockAddr.sin_family = AF_INET;
	strSockAddr.sin_port = (unsigned short) gstrPMServent.s_port;

	if ((pstrHE = gethostbyname(pcMasterStation)) == NULL)
	{
		Debug(( DBG_L_FATAL, 
				"SendPaperMountRequest: gethostbyname (%s) ritorna errore %d\n",
				pcMasterStation, errno ));
		Debug(( DBG_L_STOP_F, "SendPaperMountRequest: fine funzione\n" ));
		return(0);
	}

	memcpy(&(strSockAddr.sin_addr.s_addr),
		   &(pstrHE->h_addr[0]), sizeof(long));

	signal(SIGALRM, SigTrap);
	siginterrupt(SIGALRM, 1);
	alarm(3);
	if (connect(iSock, (const struct sockaddr *) &strSockAddr, sizeof(strSockAddr)) < 0)
	{
		siginterrupt(SIGALRM, 0);
		alarm(0);
		Debug(( DBG_L_FATAL,
				"SendPaperMountRequest: Impossibile connettere station (%d)\n",
				errno ));
		Debug(( DBG_L_STOP_F, "SendPaperMountRequest: fine funzione\n" ));
		return(0);
	}
	siginterrupt(SIGALRM, 0);
	alarm(0);

	Debug(( DBG_L_TRACE, "SendPaperMountRequest: connessione effettuata\n" ));

	memset(acBuf, '\0', sizeof(acBuf));
	sprintf(acBuf, "0002301%-8.8s%-8.8s", pcPrinterName, pcPaper);
	iRetValue = send(iSock, acBuf, (int) strlen(acBuf), 0);
	Debug(( DBG_L_DUMP_V, "SendPaperMountRequest: Spedito '%s'/%d/%d\n",
			acBuf, iRetValue, errno ));
	close(iSock);

	Debug(( DBG_L_STOP_F, "SendPaperMountRequest: fine funzione\n" ));
	return(1);
}
