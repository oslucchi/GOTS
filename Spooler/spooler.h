/* Other dependecy includes */

/* Prototype */
void	LoadPapers();
void	LpGetStartSequence(char *pcStartSequence, char *pcPrinter, char *pcPaper, int *piSeqLen);
void	LoadReport(char *pcReportPath);
void	LoadPrinter(char *pcPrinterPath);
void	InitializePrintQueues();
void	RenameReports();
void	LoadWaitingReports(char cFlgJustStarted);
int		LpGetSeqNo();
int		GetOneLine(int	iSock, char *pcBuf, int iBufSize);
void	GetLpdStatus(char *acAnswer);
int		LpdOpenConnection();
void	ReportPrinted(char *pcPrinterName);
void	LpCheckStatus();
void	EnqueueReport();
int		LpSendData(char *pcFileName, char *pcStartSequence, int iSeqLen);
int		LpSendFile(char *pcControlFile, char *pcDataFile, char cFlgInitPrt);
int		SendPaperMountRequest(char *pcMasterStation, char *pcPaper, char *pcPrinterName);

/* Costants */ 

/* Macroes */

/* Local struct & new type definition */

/* External functions */

/* External variables */

/* Global Variables */

/* Local Variables */