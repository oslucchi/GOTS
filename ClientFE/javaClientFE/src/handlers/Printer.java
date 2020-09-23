package handlers;

public class Printer {

	private String name = "";
	private String description = "";
	private String paper = "";
	private String report = "";
	private String reportJ = "";
	private String reportS = "";
	
	public Printer(String name, String report, String reportJ, String reportS, String paper, String description)
	{
		this.name = name;
		this.description = description;
		this.paper = paper;
		this.report = report;
		this.reportJ = reportJ;
		this.reportS = reportS;
	}

	public String getName() {
		return name;
	}

	public void setName(String name) {
		this.name = name;
	}

	public String getDescription() {
		return description;
	}

	public void setDescription(String description) {
		this.description = description;
	}

	public String getPaper() {
		return paper;
	}

	public void setPaper(String paper) {
		this.paper = paper;
	}

	public String getReport() {
		return report;
	}

	public void setReport(String report) {
		this.report = report;
	}

	public String getReportJ() {
		return reportJ;
	}

	public void setReportJ(String reportJ) {
		this.reportJ = reportJ;
	}

	public String getReportS() {
		return reportS;
	}

	public void setReportS(String reportS) {
		this.reportS = reportS;
	}
}

