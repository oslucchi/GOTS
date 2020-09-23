/* System includes */
#include	<stdio.h>        
#include	<string.h>
#include	<stdlib.h>        
#include	<errno.h>
#include	<unistd.h>

/* Local module includes */
#include	"cfgmng.h"
#include	"debug.h"

/* Costants */
#ifndef	TRUE
#define	TRUE	1
#define	FALSE	!TRUE
#endif

#define		BLANK			((char) 32)
#define		CNULL			((char) 0)
#define		COMMENT			'#'
#define		STARTMODULE		'['
#define		ENDMODULE		']'

/* Macroes */               

/* Local struct & new type definition */

/* External Functions */

/* External Variables */

/* Global Variables */

/* Local Variables */
static	struct _varList	*root = NULL;



/*
 * Function:	CfgUngetFile
 *
 * Description:	Libera la memoria precedentemente occupata dalla GetCgfFile.
 *
 * Parameters:	nessuno
 *
 * Returs:		0 PER ok, il codice d'errore viceversa.
 */
void	CfgUngetFile()
{
	struct	_varList	*current;
	struct	_varList	*temp;
	
	current = root;
	while(current != NULL)
	{
		temp = current->next;
		free(current);
		current = temp;
	}
	root = (struct _varList *) NULL;
	
	return;
}

/*
 * Function:	CfgGetFile
 *
 * Description:	Crea un lista in formato:
 *				MODULO,VARIABILE, VALORE
 *				leggendone i campi dal file il cui file pointer e' passato 
 *				per parametro.  Vengono ignorate tutte le entry che hanno come 
 *				primo carattere '#'.
 *				Il formato delle righe e' VARIABILE = VALORE. I separatori 
 *				ammessi sono BLANK e TAB.
 *				L'inizio di un modulo e' determinato dalla sintassi [modulo]
 *				ad inizio riga.
 *				Le definizioni prima del primo modulo sono globali.
 *
 * Parameters:	file pointer del file di configurazione (gia' aperto)
 *
 * Returs:		0 PER ok, il codice d'errore viceversa.
 */
long	CfgGetFile(FILE *cfgFP)
{
	char	cfgEntry[256];
	char	*token; 
	char	module[256]; 
	char	*ptrToChr;
	char	*ptrToEqual;
	char	equalIsPresent;
	struct	_varList	*current;
	struct	_varList	*temp;
	
	/*
	 * Il modulo iniziale e' ""
	 */
	strcpy(module, "");

	current = root;
	while(current != NULL)
	{
		temp = current->next;
		free(current);
		current = temp;
	}
	root = (struct _varList *) NULL;
	
	while(1)
	{
		/*
		 * Cerco la prossima entry.
		 */
		memset(cfgEntry, '\0', sizeof(cfgEntry));
		
		if (fgets(cfgEntry, sizeof(cfgEntry), cfgFP) == NULL)
		{         
			/*
			 * Verifico se raggiunta o meno la fine del file.
			 */
			if (feof(cfgFP))
			{
				return(0);
			}
			else
			{
				return(ferror(cfgFP));
			}
		}
		
		if ((ptrToChr = strchr(cfgEntry, '\n')) != NULL)
			*ptrToChr = '\0';

		Debug((DBG_L_DUMP_V, "CfgGetFile: entry: '%s'\n", cfgEntry));
		
		/*
		 * Verifico se la linea contiene qualcosa.
		 */
		if ((token = strtok(cfgEntry, "	 ")) == NULL)
			continue;
		
		/*
		 * Verifico se la linea e' un commento.
		 */
		if (*token == COMMENT)
			continue;

		/*
		 * Verifico se la linea e' l'inizio di un modulo
		 */
		if (*token == STARTMODULE)
		{
			token++;

			/*
			 * C'e' la chiusura del modulo ?
			 */
			if ((ptrToChr = strchr(token, ENDMODULE)) != NULL)
			{
				*ptrToChr = '\0';
			}
			else
			{
				return(CFG_MALFORMED_MODULE);
			}

			strcpy(module, token);

			continue;
		}

		/*
		 * Allocazione del nuovo elemento sulla lista.
		 */
		if (root == NULL)
		{
			if ((root = (struct _varList *)
						 malloc(sizeof(struct _varList))) == NULL)
			{
				return(CFG_MALLOC_ERROR);
			}
			memset(root, '\0', sizeof(struct _varList));
			root->next = NULL;
			current = root;
		}
		else
		{
			if ((temp = (struct _varList *)
						 malloc(sizeof(struct _varList))) == NULL)
			{
				return(CFG_MALLOC_ERROR);
			}
			memset(temp, '\0', sizeof(struct _varList));
			current->next = temp;
			temp->next = NULL;
			current = temp;
		}


		/*
		 * La linea puo' avere i seguenti formati:
		 * 1) NOME=VALORE
		 * 2) NOME= VALORE
		 * 3) NOME =VALORE
		 * 4) NOME = VALORE
		 * dove VALORE puo' essere una singola parola oppure una sequenza:
		 * 'VALORE VALORE VALORE ....' racchiussa tra apici
		 */

		/*
		 * Controllo se l'uguale e' compreso nella linea appena letta e setto
		 * una variabile opportuna (casi 1 e 2).
		 * Se c'e' l'uguale setto a NULL il carattere cosi' la successiva 
		 * allocazione e' precisa
		 */
		if ((ptrToEqual = strchr(token, '=')) != NULL)
		{
			Debug((DBG_L_TRACE, "CfgGetFile: C'e' gia' l'uguale"));
			*ptrToEqual = '\0';
			equalIsPresent = TRUE;
		}
		else
		{
			equalIsPresent = FALSE;
		}

		if (strcmp(module, "") != 0)
		{
			/*
			 * Nel file di configurazione e' gia' stato specificato il primo
			 * modulo. Tutte le righe di configurazione che seguono la prima
			 * definizione o sono modul-header (ridefiniscono il modulo) o
			 * sono appartenenti all'ultimo modulo definito.
			 */

			/*
			 * Allocazione del campo modulo.
			 */
			if ((current->module = 
							malloc((int) strlen(module) + 1)) == NULL)
			{
				return(CFG_MALLOC_ERROR);
			}
			strcpy(current->module, module);
		}
		else
		{
			current->module = NULL;
		}
	    
	    /*
	     * Allocazione del campo nome. 
		 * Se la riga non e' un module-header deve esistere per forza.
	     */
	    if ((current->name = malloc((int) strlen(token) + 1)) == NULL)
	    {
	    	return(CFG_MALLOC_ERROR);
	    }
	    strcpy(current->name, token);
		Debug((DBG_L_DUMP_V, "CfgGetFile: il nome e' '%s'\n", token));
	    

		/*
		 * Controllo che il campo successivo sia un uguale.
		 */
	    if (equalIsPresent)
		{
			/*
			 * Eravamo nei casi 1 e 2, l'uguale e' gia' letto
			 */
			token = ptrToEqual + 1;
		}
		else
		{
			Debug((DBG_L_TRACE, "CfgGetFile: cerco l'uguale\n"));
			if ((token = strtok(NULL, " 	")) == NULL)
			{
				return(CFG_BAD_ENTRY_EQUAL_SIGN);
			}                         
			if (*token != '=')
			{
				return(CFG_BAD_ENTRY_EQUAL_SIGN);
			}                         
			Debug((DBG_L_TRACE, "CfgGetFile: trovato (%s)\n", token));

			if (strlen(token) == 1)
			{
				token++;
			}
			token++;
		}

		while(*token == ' ')
		{
			token++;
		}

		ptrToChr = token + strlen(token) - 1;
		while(*ptrToChr == ' ')
		{
			*ptrToChr = '\0';
			ptrToChr--;
		}


		if (*token == '\'')
		{
			token++;
			if ((ptrToChr = strchr(token, '\'')) == NULL)
			{
				return(CFG_BAD_ENTRY_EQUAL_SIGN);
			}                         
			*ptrToChr = '\0';
		}
			
		Debug((DBG_L_DUMP_V, "CfgGetFile: il valore e' '%s'\n", token));


		if (strlen(token) == 0)
		{
			/*
			 * Manca il campo valore.
			 */
			return(CFG_BAD_ENTRY_EQUAL_SIGN);
		}

	    /*
	     * Allocazione del campo valore.
	     */
	    if ((current->value = malloc(strlen(token) + 1)) == NULL)
	    {
	    	return(CFG_MALLOC_ERROR);
	    }
		memset(current->value, '\0', sizeof(current->value));
	    strcpy(current->value, token);
	}                                    
	
	return(0);
}



/*
 * Function:	CfgUpdValue
 *
 * Description:	Aggiorna il valore del campo specificato nella chiamata
 *				al valore passato per parametro.
 *
 * Parameters:	Modulo da cercare. Se "" cerca solo nel modulo main.
 *				Variabile da modificare.
 *				Valore per la modifica.
 *
 * Return:		0 per OK, -1 viceversa
 */
int		CfgUpdValue(char *moduleName, char *varName, char *varValue)
{
	static struct	_varList	*current;

	/*
	 * La funzione di inizializzazione non � stata eseguita.
	 */
	if (root == NULL)
	{
		return(-1);
	}
	current = root;
	
	while(current != NULL)
	{
		/*
		 * Se sto facendo una ricerca per modulo e non siamo nel modulo giusto
		 * salta al successivo
		 */
		if ((moduleName != NULL) && 
			(strcmp(current->module, moduleName) != 0))
		{
			current = current->next;
			continue;
		}

		if (strcmp(current->name, varName) == 0)
		{
			free(current->value);
			if ((current->value = malloc(strlen(varValue) + 1)) == NULL)
			{
				return(-1);
			}
			memset(current->value, '\0', sizeof(current->value));
			strcpy(current->value, varValue);
			return(0);
		}
		
		current = current->next;
	}
	
	return(-1);
}	




/*
 * Function:	CfgGetValue
 *
 * Description:	Cerca sulla lista dei valori quello corrispondente al
 *				parametro e ne ritorna il valore in formato stringa.
 *
 * Parameters:	Modulo da cercare. Se NULL creca in tutti i moduli.
 *								   Se "" cerca nel modulo iniziale.
 *				Variabile da cercare.
 *
 * Return:		Valore trovato, NULL viceversa.
 */
char	*CfgGetValue(char *moduleName, char *varName)
{
	static struct	_varList	*current;

	/*
	 * La funzione di inizializzazione non � stata eseguita.
	 */
	if (root == NULL)
	{
		return(NULL);
	}
	current = root;
	
	while(current != NULL)
	{
		/*
		 * Se sto facendo una ricerca per modulo e non siamo nel modulo giusto
		 * salta al successivo
		 */
		if ((moduleName != NULL) && 
			(strcmp(current->module, moduleName) != 0))
		{
			current = current->next;
			continue;
		}

		if (strcmp(current->name, varName) == 0)
		{
			return(current->value);
		}
		
		current = current->next;
	}
	
	return("");
}	



/*
 * Function:	CfgGetAllValueLike
 *
 * Description:	Cerca sulla lista dei valori tutti quelli uguali alla parte
 *				di variabile passata per parametro e ne ritorna il valore 
 *				in formato stringa. La prima chiamata specifica il valore 
 *				della variabile, successivamente va specifiato NULL
 *
 * Parameters:	Modulo 
 *				Variabile da cercare.
 *				Puntatore a buffer gia' allocato per metter il nome della 
 *				variabile trovata
 *
 * Return:		Valore trovato, NULL viceversa.
 */
char	*CfgGetAllValueLike(char *moduleName, char *varNameReq, char *varName)
{
	static char		*localVarName;
	static struct	_varList	*current;

	/*
	 * La funzione di inizializzazione non � stata eseguita.
	 */
	if (root == NULL)
	{
		return(NULL);
	}

	if (varNameReq != NULL) 
	{
		/*
		 * Prima chiamata non eseguita
		 */
		localVarName = varNameReq;
		current = root;
	}
	else
	{
		if (localVarName == NULL)
			return(NULL);
		current = current->next;
	}

	while(current != NULL)
	{
		/*
		 * Se sto facendo una ricerca per modulo e non siamo nel modulo giusto
		 * salta al successivo item
		 */
		if ((moduleName != NULL) && 
			(strcmp(current->module, moduleName) != 0))
		{
			current = current->next;
			continue;
		}

		if (strncmp(current->name, 
					  localVarName, strlen(localVarName)) == 0)
		{
			strcpy(varName, current->name);
			return(current->value);
		}
		
		current = current->next;
	}
	
	varName = NULL;
	localVarName = NULL;
	return(NULL);
}	



/*
 * Function:	CfgUpdFile
 *
 * Description:	Dumpa su un file la lista in formato:
 *				MODULO,VARIABILE, VALORE
 *				Vengono persi i commenti
 *
 * Parameters:	nome del file da scrivere. Se esiste viene riscritto.
 *
 * Return:		0 PER ok, il codice d'errore viceversa.
 */
long	CfgUpdFile(char *pcFileName)
{
	char	acTempFile[256];
	char	moduleName[32]; 
	struct	_varList	*current;
	struct	_varList	*temp;
	FILE	*pFOut;
	
	/*
	 * Il modulo iniziale e' ""
	 */
	strcpy(moduleName, "");

	strcpy(acTempFile, "/tmp/cfgXXXXXX");
	mkstemp(acTempFile);
	if ((pFOut = fopen(acTempFile, "w")) == NULL)
	{
		return(errno);
	}
	
	current = root;
	while(current != NULL)
	{
		if ((current->module != NULL) &&
			(strcmp(current->module, moduleName) != 0))
		{
			fprintf(pFOut, "\n\n[%s]\n", current->module);
			strcpy(moduleName, current->module);
		}
		fprintf(pFOut, "%s = %s\n", current->name, current->value);
		current = current->next;
	}
	fclose(pFOut);
	if (access(pcFileName, F_OK) == 0)
	{
		if (remove(pcFileName) != 0)
		{
			return(errno);
		}
	}
	if (rename(acTempFile, pcFileName) != 0)
	{
		return(errno);
	}
	return(0);
}
