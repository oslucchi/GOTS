#ifndef LIBCFG_H
#define LIBCFG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Other dependecy includes */

/* Prototype */
void	CfgUngetFile();
long	CfgGetFile(FILE *cfgFP);
int		CfgUpdValue(char *moduleName, char *varName, char *varValue);
char	*CfgGetValue(char *moduleName, char *varName);
char	*CfgGetAllValueLike(char *moduleName, char *varNameReq, char *varName);
long	CfgUpdFile(char *pcFileName);

/* Costants */ 
#define CFG_MALLOC_ERROR 			-1
#define CFG_BAD_ENTRY_NO_VALUE		-2
#define CFG_BAD_ENTRY_EQUAL_SIGN	-3
#define CFG_BAD_ENTRY_NO_EQUAL		-4
#define CFG_MALFORMED_MODULE		-5

/* Macroes */

/* Local struct & new type definition */
struct	_varList	{
	char	*module;
	char	*name;
	char	*value;
	struct	_varList	*next;
};

/* External functions */

/* External variables */

/* Global Variables */

/* Local Variables */

#ifdef __cplusplus
}
#endif

#endif
