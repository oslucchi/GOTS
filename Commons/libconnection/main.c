/* Global includes */
#include	<stdio.h>        
#include	<string.h>
#include	<stdlib.h>        
#include	<errno.h>

/* Local module includes */
#include "connection.h"

/* Costants */

/* Macroes */

/* Local struct & new type definition */
struct	nw_a_addr	{
	unsigned char	c1;
	unsigned char	c2;
	unsigned char	c3;
	unsigned char	c4;
};

union	nw_addr	{
	struct	nw_a_addr	nw_c_addr;
	long	nw_l_addr;
};

/* External Functions */

/* External Variables */

/* Global Variables */

/* Local Variables */



/*
 *
 *	Function:		AddressConvertATN
 *
 *	Description:	Conversione da carattere a long dell'indirizzo di rete
 *					del mittente
 *
 *	Parameters:		indirizzo ascii
 *
 *	Returns:		indirizzo in formato long
 *
 */
long	AddressConvertATN(char *txtAddr)
{
	char	*ptrToSep;
	char	*ptrToAddr;
	char	localBuf[16];
	static	union	nw_addr	retValue;

	memset(localBuf, '\0', sizeof(localBuf));
	memcpy(localBuf, txtAddr, strlen(txtAddr));

	ptrToAddr = localBuf;
	if ((ptrToSep = strchr(ptrToAddr, '.')) == NULL)
	{
		return((long) 0);
	}
	*ptrToSep = '\0';
	retValue.nw_c_addr.c1 = atoi(ptrToAddr);

	ptrToAddr = ptrToSep + 1;
	if ((ptrToSep = strchr(ptrToAddr, '.')) == NULL)
	{
		return((long) 0);
	}
	*ptrToSep = '\0';
	retValue.nw_c_addr.c2 = atoi(ptrToAddr);

	ptrToAddr = ptrToSep + 1;
	if ((ptrToSep = strchr(ptrToAddr, '.')) == NULL)
	{
		return((long) 0);
	}
	*ptrToSep = '\0';
	retValue.nw_c_addr.c3 = atoi(ptrToAddr);

	ptrToAddr = ptrToSep + 1;
	retValue.nw_c_addr.c4 = atoi(ptrToAddr);


	return(retValue.nw_l_addr);
}



/*
 *
 *	Function:		AddressConvertNTA
 *
 *	Description:	Conversione da long a stringa di carattere dell'indirizzo 
 *					di rete del mittente
 *
 *	Parameters:		indirizzo IP in formato long
 *
 *	Returns:		indirizzo in formato stringa
 *
 */
char	*AddressConvertNTA(long longAddr)
{
	union	nw_addr	retValue;
	static	char	txtAddr[16];

	retValue.nw_l_addr = longAddr;
	sprintf(txtAddr,
			 "%d.%d.%d.%d",
			 retValue.nw_c_addr.c1,
			 retValue.nw_c_addr.c2,
			 retValue.nw_c_addr.c3,
			 retValue.nw_c_addr.c4);

	return(txtAddr);
}

