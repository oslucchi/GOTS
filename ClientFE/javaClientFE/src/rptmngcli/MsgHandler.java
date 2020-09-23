package rptmngcli;

import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Date;

import handlers.Paper;
import handlers.Printer;
import handlers.Report;

public class MsgHandler {
	static final String MSGTYPE_ALIGNMENT_START = "91";
	static final String MSGTYPE_ALIGNMENT_END = "92";
	static final String MSGTYPE_PRINTER = "94";
	static final String MSGTYPE_PAPER = "95";
	static final String MSGTYPE_REPORT = "96";
			
	Userform ufLocal = null;
	
	public MsgHandler(Userform userform)
	{
		this.ufLocal = userform;
	}
	
	public void decodeMsg(String message)
	{
		switch(message.substring(0, 2))
		{
		case MSGTYPE_ALIGNMENT_START:
			System.out.println("Alignment starts");
			break;
			
		case MSGTYPE_ALIGNMENT_END:
			System.out.println("Alignment over");
			break;
		
		case MSGTYPE_PRINTER:
			handlePrinter(message.substring(2));
			break;

		case MSGTYPE_PAPER:
			handlePaper(message.substring(2));
			break;

		case MSGTYPE_REPORT:
			handleReport(message.substring(2));
			break;

		}
	}
	
	@SuppressWarnings("unchecked")
	private void handlePrinter(String message)
	{
		Printer printer = new Printer(message.substring(0, 8),
									  message.substring(8, 16),
									  message.substring(16, 24),
									  message.substring(24, 32),
									  message.substring(32, 40),
									  message.substring(40));
		ufLocal.printers.put(printer.getName(), printer);
		ufLocal.printer.addItem(printer.getName());
	}
	@SuppressWarnings("unchecked")
	private void handlePaper(String message)
	{
		Paper paper = new Paper(message.substring(0, 8));
		ufLocal.papers.put(paper.getName(), paper);
		ufLocal.paper.addItem(paper.getName());
	}

	private void handleReport(String message)
	{
		SimpleDateFormat sdf = new SimpleDateFormat("dd/MM/yyyy HH:mm:ss");
		Date date = null;
		try {
			date = sdf.parse(message.substring(140, 159));
		} catch (ParseException e) {
			// TODO Auto-generated catch block
			date = new Date();
		}
		Report report = new Report(message.substring(0, 8),
								   message.substring(8, 72),
								   message.substring(72, 80),
								   message.substring(80, 88),
								   message.substring(88, 96),
								   message.substring(96, 112),
								   message.substring(112, 120),
								   message.substring(120, 127),
								   message.charAt(127),
								   Short.parseShort(message.substring(128, 131)),
								   Short.parseShort(message.substring(131, 134)),
								   Short.parseShort(message.substring(134, 137)),
								   Short.parseShort(message.substring(137, 140)),
								   date,
								   Integer.parseInt(message.substring(159, 165)),
								   message.substring(165, 173));
		ufLocal.reports.put(report.getReportName(), report);
//		ufLocal.report.addItem(report.getReportName());
	}
	
}
