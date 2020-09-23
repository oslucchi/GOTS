/*
 *	Project:	Libutil
 *
 *	@(#)$RCSfile: $	$Revision: $	(C) 1998 L-Soft sas
 *
 *	Release $Revision: $ del $Date: $
 *	Origine: $Source: $
 *
 *	$Log: $
 *
 */
static char rcsid[] = "$Id$";


/* 
 * Include globali
 */
#include <sys/types.h>
#include <sys/times.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

/*
 * Include locali al modulo
 */
#include <ctype.h>

/*
 * Costanti del modulo 
 */

/*
 * Macroes
 */

/*
 * Funzioni esterne
 */

/*
 * Variabili esterne
 */

/*
 * Variabili visibili del modulo
 */

/*
 * Variabili locali al modulo
 */



/*
 * Nome procedura:	StringToUpper
 *
 * Descrizione:		Converte la stringa passata per parametro in caratteri
 *					maiuscoli. CONVERTE LA STRINGA PASSATA PER PARAMETRO.
 *
 * Parametri:		stringa da convertire
 *					
 *
 * Ritorna:			stringa convertita
 */
char	*StringToUpper( char *pcString )
{
	char	*pcLocal = pcString;

	while( *pcLocal != '\0' )
	{
		*pcLocal = toupper( *pcLocal );
		pcLocal++;
	}
	return( pcString );
}



/*
 * Nome procedura:	StringToLower
 *
 * Descrizione:		Converte la stringa passata per parametro in caratteri
 *					minuscoli. CONVERTE LA STRINGA PASSATA PER PARAMETRO.
 *
 * Parametri:		stringa da convertire
 *					
 *
 * Ritorna:			stringa convertita
 */
char	*StringToLower( char *pcString )
{
	char	*pcLocal = pcString;

	while( *pcLocal != '\0' )
	{
		*pcLocal = tolower( *pcLocal );
		pcLocal++;
	}
	return( pcString );
}



/*
 * Nome procedura:	StringTrimm
 *
 * Descrizione:		Elimina i trailing ed i leading blank da una stringa
 *					CONVERTE LA STRINGA PASSATA PER PARAMETRO.
 *
 * Parametri:		stringa da convertire
 *					
 *
 * Ritorna:			stringa convertita
 */
char	*StringTrimm( char *pcString )
{
	int		iCount, iOffset;
	char	*pcLocal = pcString;

	while( *pcLocal == ' ' )
	{
		pcLocal++;
	}

	if ( pcLocal != pcString )
	{
		iOffset = (int) (pcLocal - pcString);
		for( iCount = iOffset;  iCount < strlen( pcString ); iCount++ )
		{
			pcString[iCount - iOffset] = pcString[iCount];
		}
		memset( pcLocal + iCount, '\0', iOffset );
	}

	pcLocal = pcString + strlen( pcString ) - 1;
	while( *pcLocal == ' ' )
	{
		*pcLocal = '\0';
		pcLocal--;
	}

	return( pcString );
}
