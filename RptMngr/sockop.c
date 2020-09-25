/* Global includes */
#include	<stdio.h>
#include	<errno.h>
#include	<signal.h>
#include	<string.h>
#include	<unistd.h>
#include	<sys/types.h>
#include	<sys/socket.h>
#include	<netinet/in.h>
#include	<netdb.h>
#include	<stdlib.h>

/* Local module includes */
#include	"gotspool.h"
#include	"debug.h"

/* Costants */
#define	SKTSERV		"rptmng"

/* Macroes */               

/* Local struct & new type definition */

/* External Functions */
extern	char	*AddressConvertNTA(long longAddr);

/* External Variables */
extern	long	glRetValue;
extern	char	*gpcPrgName;
extern	char	gacHomeDir[];
extern	char	gacGCUXHome[];

/* Global Variables */

/* Local Variables */
static	int		iLstSkt;
static	int		iRcvSkt;
static	int		iAddrLen;
static	struct	sockaddr_in strAddr;


/*
 *
 *	Function:		CloseReceiveSocket
 *
 *	Description:	Chiude il socket di ricezione
 *
 *	Parameters:		nessuno
 *
 *	Returns:		void
 *
 */
void	CloseReceiveSocket()
{
	close(iRcvSkt);
}


/*
 *
 *	Function:		RecvSock
 *
 *	Description:	riceve un messaggio completo dal socket
 *
 *	Parameters:		buffer in cui mettere il messaggio, size del buffer
 *
 *	Returns:		0 per OK / codice d'errore
 *
 */
int		RecvSock(char *pcBuf, int iBufSize)
{
	char	iRcvdBytes;
	char	iLenToReceive = 5;
	char	*pcHdrPtr = pcBuf;
	char	*pcBodyPtr = pcBuf + 5;

	Debug(( DBG_L_START_F, "RecvSock: Inizio funzione\n" ));

	memset(pcBuf, '\0', iBufSize);

	/*
	 * Ricezione dell'header.
	 */
	while(strlen(pcBuf ) < 5)
	{
		iRcvdBytes = recv(iRcvSkt, pcHdrPtr, 5 - strlen(pcBuf), 0);
		Debug(( DBG_L_DUMP_V,
				"RecvSock: ricezione hdr: '%s' (%d-%d)\n",
				pcBuf, iRcvdBytes, errno ));
		switch(iRcvdBytes)
		{
		case -1:
			if (errno != EINTR)
				return(errno);
			break;

		case 0:
			/*
			 * Socket chiuso.
			 */
			return(-1);
			break;
		}
		pcHdrPtr += iRcvdBytes;
	}
	iLenToReceive = atoi(pcBuf);

	while(strlen(pcBuf ) < iLenToReceive)
	{
		iRcvdBytes = recv(iRcvSkt, pcBodyPtr, 
						   iLenToReceive - strlen(pcBuf), 0);
		Debug(( DBG_L_DUMP_V,
				"RecvSock: ricezione body: '%s' (%d-%d)\n",
				pcBuf, iRcvdBytes, errno ));
		switch(iRcvdBytes)
		{
		case -1:
			return(errno);
			break;

		case 0:
			/*
			 * Socket chiuso.
			 */
			return(-1);
			break;
		}
		pcBodyPtr += iRcvdBytes;
	}
	Debug(( DBG_L_DUMP_V, "RecvSock: ricevuto messaggio '%s'\n", pcBuf ));

	return(0);
	Debug(( DBG_L_STOP_F, "RecvSock: Fine funzione\n" ));
}


/*
 *
 *	Function:		SendSock
 *
 *	Description:	Spedisce un messaggio via socket
 *
 *	Parameters:		messaggio da spedire (solo ASCII)
 *
 *	Returns:		0 per OK / codice d'errore
 *
 */
void	SendSock(char *pcMsgToSnd)
{
	char	acLenBuf[6];

	Debug(( DBG_L_START_F, "SendSock: Inizio funzione\n" ));

	sprintf(acLenBuf, "%5.5ld", strlen(pcMsgToSnd ) + 5);

	send(iRcvSkt, acLenBuf, 5, 0);
	send(iRcvSkt, pcMsgToSnd, strlen(pcMsgToSnd), 0);
	Debug(( DBG_L_START_F, "SendSock: Spedito '%s%s'\n",
			acLenBuf, pcMsgToSnd ));

	Debug(( DBG_L_STOP_F, "SendSock: Fine funzione\n" ));
}


/*
 *
 *	Function:		OpenSocket
 *
 *	Description:	Crea e apre il socket per la comunicazione CLI/SER
 *
 *	Parameters:		nessuno
 *
 *	Returns:		void
 *
 */
void	OpenSocket()
{
	int		iSktOptVal;
	int		iSktOptLen;
	struct	servent *pstrServent;

	Debug(( DBG_L_START_F, "OpenSocket: Inizio funzione\n" ));
	/*
	 * Crea il socket
	 */
	if ((iLstSkt = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
	{
		DEBUGConsole("%s: Errore %d in allocazione socket\n",
					  gpcPrgName, errno);
		exit(errno);
	}

	Debug(( DBG_L_TRACE, "OpenSocket: Socket creato\n" ));


	/*
	 * Setta SO_KEEPALIVE e SO_REUSEADDR sul socket
	 */
	glRetValue = getsockopt(iLstSkt, SOL_SOCKET, SO_KEEPALIVE,
						   (void *) &iSktOptVal, &iSktOptLen);
	Debug(( DBG_L_DUMP_V,
			"OpenSocket: sktOptVal %d sktOptLen %d.\n",
			iSktOptVal, iSktOptLen ));
	iSktOptVal = 1;
	iSktOptLen = 4;
	glRetValue = setsockopt(iLstSkt, SOL_SOCKET, SO_KEEPALIVE,
						   (void *) &iSktOptVal, iSktOptLen);
	Debug(( DBG_L_DUMP_V,
			"OpenSocket: setsockopt SO_KEEPALIVE ha ritornato %d-%d\n",
			glRetValue, errno ));

	glRetValue = setsockopt(iLstSkt, SOL_SOCKET, SO_REUSEADDR,
						   (void *) &iSktOptVal, iSktOptLen);
	Debug(( DBG_L_DUMP_V,
			"OpenSocket: setsockopt SO_REUSEADDR ha ritornato %d-%d\n",
			glRetValue, errno ));

	/*
	 * Prendi il numero di porta
	 */
	Debug(( DBG_L_DUMP_V, "OpenSocket: cerco servizio %s.\n", SKTSERV ));
	pstrServent = getservbyname(SKTSERV, NULL);
	if (pstrServent == NULL)
	{
		Debug(( DBG_L_FATAL,
				"OpenSocket: Servizio %s non presente in /etc/services\n",
				SKTSERV ));
		DEBUGConsole("%s: Servizio %s non presente in /etc/services\n",
					  gpcPrgName, SKTSERV);
		exit(1);
	}

	Debug(( DBG_L_DUMP_V, "OpenSocket: servizio %s numero porta %d.\n", 
			pstrServent->s_name,
			ntohs((unsigned short) pstrServent->s_port) ));

	strAddr.sin_family = AF_INET;
	strAddr.sin_port = (unsigned short) pstrServent->s_port;
	// strAddr.sin_port = (unsigned short) htons(9050);
	strAddr.sin_addr.s_addr = INADDR_ANY;
	if (bind(iLstSkt, (const struct sockaddr *) &strAddr, sizeof(strAddr ) ) != 0)
	{
		DEBUGConsole("%s: Errore %d in bind socket\n", gpcPrgName, errno);
		close(iLstSkt);
		exit(2);
	}

	Debug(( DBG_L_TRACE, "OpenSocket: bind effettuata.\n" ));

	listen(iLstSkt, 5);

	Debug(( DBG_L_STOP_F, "OpenSocket: Fine funzione\n" ));
}


/*
 *
 *	Function:		Accept
 *
 *	Description:	Gestisce la richiesta di connessione del client
 *
 *	Parameters:		nessuno
 *
 *	Returns:		0 per OK / errore
 *
 */
int		Accept(void)
{
	struct	hostent		*pstrHe;
	long	*lAddrLong;
	int		iPid;
	int		iRetValue;

	Debug(( DBG_L_START_F, "Accept: Inizio funzione\n" ));

	iAddrLen = sizeof(strAddr);
	if ((iRcvSkt = accept(iLstSkt, (struct sockaddr *) &strAddr, &iAddrLen)) < 0)
	{
		DEBUGConsole("%s: Errore %d in accept\n", gpcPrgName, errno);
		return(-1);
	}
	else
	{
		pstrHe = gethostbyname(AddressConvertNTA(strAddr.sin_addr.s_addr));
		Debug(( DBG_L_DUMP_V, "Accept: si e' collegato il client '%s'\n",
				pstrHe->h_name ));
		return(0);
	}
}
