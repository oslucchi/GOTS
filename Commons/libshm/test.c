// vi: ts=4 sw=4
#include	<stdio.h>
#include	"gcspool.h"
#include	"libshm.h"

char gacGCUXHome[1024];

int main()
{
	int		iResult;
	_strPrinterItem	strPrintItem;
	_strPrinterItem	*pstrPrintItem;
	_strReportItem	strReportItem;
	_strReportItem	*pstrReportItem;

	InitSHM( 0 );

/*
	strcpy( strPrintItem.acPrinterName, "ERRPRT" );
	strcpy( strPrintItem.acPrintingRpt, "AGFLCO" );
	strcpy( strPrintItem.acCurrentPaper, "STND" );
	AddPrinter( &strPrintItem );

	strcpy( strPrintItem.acPrinterName, "STP04" );
	strcpy( strPrintItem.acPrintingRpt, "" );
	strcpy( strPrintItem.acCurrentPaper, "STND" );
	AddPrinter( &strPrintItem );

	strcpy( strPrintItem.acPrinterName, "STP16" );
	strcpy( strPrintItem.acPrintingRpt, "" );
	strcpy( strPrintItem.acCurrentPaper, "CONT" );
	AddPrinter( &strPrintItem );

	strcpy( strPrintItem.acPrinterName, "epson" );
	strcpy( strPrintItem.acPrintingRpt, "AGFLCO" );
	strcpy( strPrintItem.acCurrentPaper, "STND" );
	AddPrinter( &strPrintItem );

	strcpy( strPrintItem.acPrinterName, "oki10ex" );
	strcpy( strPrintItem.acPrintingRpt, "AGFLCO" );
	strcpy( strPrintItem.acCurrentPaper, "STND" );
	AddPrinter( &strPrintItem );
*/

	RestartPrinterList();

	while(( pstrPrintItem = GetNextPrinter() ) != NULL )
	{
		printf( "Stampante '%s' carta '%s' report '%s'\n",
				pstrPrintItem->acPrinterName,
				pstrPrintItem->acCurrentPaper,
				pstrPrintItem->acPrintingRpt );
	}

/*
	strcpy( strReportItem.acCfgFilePath, "cfg-path-1" );
	strcpy( strReportItem.acReportName, "r-name-1" );
	strcpy( strReportItem.acReportPath, "r-path-1" );
	strcpy( strReportItem.acUserName, "ur-name-1" );
	strcpy( strReportItem.acPaper, "paper-1" );
	strcpy( strReportItem.acDevice, "device-1" );
	strcpy( strReportItem.acUnixId, "unix-id-1" );

	iResult = AddReport( &strReportItem, "epson" );
	printf( "AddReport ha ritornato %d\n", iResult );

	strcpy( strReportItem.acCfgFilePath, "cfg-path-2" );
	strcpy( strReportItem.acReportName, "r-name-2" );
	strcpy( strReportItem.acReportPath, "r-path-2" );
	strcpy( strReportItem.acUserName, "ur-name-2" );
	strcpy( strReportItem.acPaper, "paper-2" );
	strcpy( strReportItem.acDevice, "device-2" );
	strcpy( strReportItem.acUnixId, "unix-id-2" );

	iResult = AddReport( &strReportItem, "epson" );
	printf( "AddReport ha ritornato %d\n", iResult );

	strcpy( strReportItem.acCfgFilePath, "cfg-path-3" );
	strcpy( strReportItem.acReportName, "r-name-3" );
	strcpy( strReportItem.acReportPath, "r-path-3" );
	strcpy( strReportItem.acUserName, "ur-name-3" );
	strcpy( strReportItem.acPaper, "paper-3" );
	strcpy( strReportItem.acDevice, "device-3" );
	strcpy( strReportItem.acUnixId, "unix-id-3" );

	iResult = AddReport( &strReportItem, "oki10ex" );
	printf( "AddReport ha ritornato %d\n", iResult );

	strcpy( strReportItem.acCfgFilePath, "cfg-path-4" );
	strcpy( strReportItem.acReportName, "r-name-4" );
	strcpy( strReportItem.acReportPath, "r-path-4" );
	strcpy( strReportItem.acUserName, "ur-name-4" );
	strcpy( strReportItem.acPaper, "paper-4" );
	strcpy( strReportItem.acDevice, "device-4" );
	strcpy( strReportItem.acUnixId, "unix-id-4" );

	iResult = AddReport( &strReportItem, "epson" );
	printf( "AddReport ha ritornato %d\n", iResult );

	strcpy( strReportItem.acCfgFilePath, "cfg-path-5" );
	strcpy( strReportItem.acReportName, "r-name-5" );
	strcpy( strReportItem.acReportPath, "r-path-5" );
	strcpy( strReportItem.acUserName, "ur-name-5" );
	strcpy( strReportItem.acPaper, "paper-5" );
	strcpy( strReportItem.acDevice, "device-5" );
	strcpy( strReportItem.acUnixId, "unix-id-5" );

	iResult = AddReport( &strReportItem, "oki10ex" );
	printf( "AddReport ha ritornato %d\n", iResult );
*/

	ResetReportList( "epson" );
	printf( "Printer EPSON:\n" );
	while(( pstrReportItem = GetNextReport( "epson" )) != NULL )
	{
		printf( "\tpath '%s' - name '%s' - user '%s' - id '%s'\n",
				pstrReportItem->acCfgFilePath,
				pstrReportItem->acReportName,
				pstrReportItem->acUserName,
				pstrReportItem->acUnixId );
	}

	ResetReportList( "oki10ex" );
	printf( "\n\nPrinter OKI10EX:\n" );
	while(( pstrReportItem = GetNextReport( "oki10ex" )) != NULL )
	{
		printf( "\tpath '%s' - name '%s' - user '%s' - id '%s'\n",
				pstrReportItem->acCfgFilePath,
				pstrReportItem->acReportName,
				pstrReportItem->acUserName,
				pstrReportItem->acUnixId );
	}
}
