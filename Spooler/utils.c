/*
 * vi: ai ts=4 sw=4
 */

/*
 *	@(#)$RCSfile: utils.c,v $	$Revision: 1.1 $	(C) 1998 L-Soft sas
 *
 *	Release $Revision: 1.1 $ del $Date: 2001/08/21 10:37:11 $
 *	Origine: $Source: /usr/prj/gcspool/migraspool/RCS/utils.c,v $
 *
 *	$Log: utils.c,v $
 *	Revision 1.1  2001/08/21 10:37:11  root
 *	Initial revision
 *
 *
 */

#define SCCSID "$Id: utils.c,v 1.1 2001/08/21 10:37:11 root Exp $"



/* Include globali */
#include	<stdio.h>
#include	<errno.h>
#include	"gcspool.h"
#include	"debug.h"

/* Include locali al modulo */
#include	<string.h>
#include	<netdb.h>
#include	<netinet/in.h>
#include	<sys/stat.h>
#include	<sys/file.h>

/* Costanti del modulo */

/* Macroes */

/* Funzioni esterne */

/* Variabili esterne */
extern	char	*pcSHMPrinterBuf;
extern	char	*pcSHMReportBuf;
extern	char	acPrinterIdx[];
extern	char	acReportIdx[];
extern	char	gacGCUXHome[];

/* Variabili locali al modulo */ 
static	_strPrinterItem	*pstrPrintItem;
static	_strReportItem	*pstrReportItem;

/* Variabili visibili del modulo */ 


/*
 *
 * Nome procedura:  DumpSHM
 * 
 * Descrizione:     Dumpa il contenuto della SHM
 * 
 * Parametri:       void
 * 
 * Valore ritorno:  void
 *
 */
void	DumpSHM( void )
{
	int		iSeqNum;
	int		iCount = 1;

	Debug(( DBG_L_START_F, "DumpRecords: Inizio funzione\n" ));
	while( acPrinterIdx[iCount] )
	{
		pstrPrintItem = (_strPrinterItem *)
						(pcSHMPrinterBuf + sizeof( _strPrinterItem ) * iCount);

		printf( "Stampante:\n\tname: %s %s\n\tdev: %s - lp: %s - stat: %d\n",
				pstrPrintItem->acPrinterName,
				pstrPrintItem->acDescription,
				pstrPrintItem->acUnixDevice,
				pstrPrintItem->acUnixLpName,
				pstrPrintItem->lUnixLpStatus );

		printf( "\tReport accodati:\n" );
		/*
		 * Verifico se vi sono report da stampare e li accodo.
		 */
		pstrReportItem = pstrPrintItem->pstrRepoRoot;
		while( pstrReportItem != NULL )
		{
			printf( "\tname: %s - job %s/%s\n\tfile: %s\n"
					"\tusr:  %s - dev %s - uid %s\n",
					pstrReportItem->acReportName,
					pstrReportItem->acJobNum,
					pstrReportItem->acJobSon,
					pstrReportItem->acReportPath,
					pstrReportItem->acUserName,
					pstrReportItem->acDevice,
					pstrReportItem->acUnixId );

			pstrReportItem = pstrReportItem->pstrRepoNext;
		}
		printf( "\n\n" );
		iCount++;
	}
	Debug(( DBG_L_STOP_F, "DumpRecords: fine funzione\n" ));
}
