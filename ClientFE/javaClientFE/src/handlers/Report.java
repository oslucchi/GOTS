package handlers;

import java.util.Date;

public class Report {
	private String printerName = "";
	private String printerPath = "";
	private String reportName = "";
	private String jobNumber = "";
	private String jobSon = "";
	private String userName = "";
	private String paper = "";
	private String unixID = "";
	private char priority = ' ';
	private short sheetOpt = 0;
	private short skipOpt = 9;
	private short dispoOpt = 0;
	private short nomOfCopies = 0;
	private Date timestamp = null;
	private int lines = 0;
	private String jobName = "";
	
	
	public Report(String printerName, String printerPath, String reportName, String jobNumber, String jobSon,
			String userName, String paper, String unixID, char priority, short sheetOpt, short skipOpt, short dispoOpt,
			short nomOfCopies, Date timestamp, int lines, String jobName)
	{
		super();
		this.printerName = printerName;
		this.printerPath = printerPath;
		this.reportName = reportName;
		this.jobNumber = jobNumber;
		this.jobSon = jobSon;
		this.userName = userName;
		this.paper = paper;
		this.unixID = unixID;
		this.priority = priority;
		this.sheetOpt = sheetOpt;
		this.skipOpt = skipOpt;
		this.dispoOpt = dispoOpt;
		this.nomOfCopies = nomOfCopies;
		this.timestamp = timestamp;
		this.lines = lines;
		this.jobName = jobName;
	}


	public String getPrinterName() {
		return printerName;
	}


	public void setPrinterName(String printerName) {
		this.printerName = printerName;
	}


	public String getPrinterPath() {
		return printerPath;
	}


	public void setPrinterPath(String printerPath) {
		this.printerPath = printerPath;
	}


	public String getReportName() {
		return reportName;
	}


	public void setReportName(String reportName) {
		this.reportName = reportName;
	}


	public String getJobNumber() {
		return jobNumber;
	}


	public void setJobNumber(String jobNumber) {
		this.jobNumber = jobNumber;
	}


	public String getJobSon() {
		return jobSon;
	}


	public void setJobSon(String jobSon) {
		this.jobSon = jobSon;
	}


	public String getUserName() {
		return userName;
	}


	public void setUserName(String userName) {
		this.userName = userName;
	}


	public String getPaper() {
		return paper;
	}


	public void setPaper(String paper) {
		this.paper = paper;
	}


	public String getUnixID() {
		return unixID;
	}


	public void setUnixID(String unixID) {
		this.unixID = unixID;
	}


	public char getPriority() {
		return priority;
	}


	public void setPriority(char priority) {
		this.priority = priority;
	}


	public short getSheetOpt() {
		return sheetOpt;
	}


	public void setSheetOpt(short sheetOpt) {
		this.sheetOpt = sheetOpt;
	}


	public short getSkipOpt() {
		return skipOpt;
	}


	public void setSkipOpt(short skipOpt) {
		this.skipOpt = skipOpt;
	}


	public short getDispoOpt() {
		return dispoOpt;
	}


	public void setDispoOpt(short dispoOpt) {
		this.dispoOpt = dispoOpt;
	}


	public short getNomOfCopies() {
		return nomOfCopies;
	}


	public void setNomOfCopies(short nomOfCopies) {
		this.nomOfCopies = nomOfCopies;
	}


	public Date getTimestamp() {
		return timestamp;
	}


	public void setTimestamp(Date timestamp) {
		this.timestamp = timestamp;
	}


	public int getLines() {
		return lines;
	}


	public void setLines(int lines) {
		this.lines = lines;
	}


	public String getJobName() {
		return jobName;
	}


	public void setJobName(String jobName) {
		this.jobName = jobName;
	}
}
