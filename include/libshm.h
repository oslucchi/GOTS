#ifndef LIBSHM_H
#define LIBSHM_H

#ifdef __cplusplus
extern "C" {
#endif

/* Other dependecy includes */
#include	"gotspool.h"

/* Prototype */
void			InitSHM(char cFlgCreate);
void			RestartPrinterList();
_strPrinterItem	*SearchPrinter(char *pcPrinterName, int *iIdx);
_strPrinterItem	*GetNextPrinter();
int				AddPrinter(_strPrinterItem *pstrPrintToAdd);
int				AddReport(_strReportItem *pstrReportAdd, char *pcPrinterName);
void			ResetReportList(char *pcPrinterName);
_strReportItem	*SearchReport(char *pcReportId, char *pcPrinterName);
int				DeleteReportFromQueue(char *pcReportId, char *pcPrinterName);
int				MoveReportBetweenQueue(char *pcReportId, char *pcOldPrinterName, char *pcNewPrinterName);
_strReportItem	*GetNextReport(char *pcPrinterName);

/* Costants */ 

/* Macroes */

/* Local struct & new type definition */

/* External functions */

/* External variables */

/* Global Variables */

/* Local Variables */

#ifdef __cplusplus
}
#endif

#endif