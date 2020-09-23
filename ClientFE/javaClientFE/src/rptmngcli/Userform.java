package rptmngcli;

//Java program to implement 
//a Simple Registration Form 
//using Java Swing 

import javax.swing.*;

import handlers.Paper;
import handlers.Printer;
import handlers.Report;

import java.awt.*; 
import java.awt.event.*;
import java.util.HashMap; 

public class Userform 
	extends JFrame 
	implements ActionListener { 

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;
	
	// Components of the Form 
	private Cnctmngr channel = null;
	private JButton btnConnection; 
	private Container c; 
	private JLabel title; 
	private JTextField user; 
	private JLabel userLbl; 
	private JLabel printerLbl;
	protected JComboBox printer; 
	protected JComboBox paper; 
	protected HashMap<String, Printer> printers = new HashMap<>();
	protected String[] printersList = new String[0];
	protected HashMap<String, Paper> papers = new HashMap<>();
	protected String[] papersList = new String[0];
	protected HashMap<String, Report> reports = new HashMap<>();
	protected String[] reportsList = new String[0];
	private JLabel printerDescriptionLbl; 
	private JLabel printerPaperLbl; 
	private JLabel printerReportLbl; 
	private JLabel printerReportJLbl; 
	private JLabel printerReportSLbl; 
	private JLabel printerDescriptionValueLbl; 
	private JLabel printerReportValueLbl; 
	private JLabel printerReportJValueLbl; 
	private JLabel printerReportSValueLbl; 
	private JTextArea msgReceived; 

	private MsgHandler msgHandler = null;
	
	// constructor, to initialize the components 
	// with default values. 
	public Userform() 
	{
		channel = new Cnctmngr();
		
		setTitle("Report client"); 
		setBounds(300, 90, 900, 600); 
		setDefaultCloseOperation(EXIT_ON_CLOSE); 
		setResizable(false); 

		c = getContentPane(); 
		c.setLayout(null); 

		title = new JLabel("Report manager client"); 
		title.setFont(new Font("Arial", Font.PLAIN, 30)); 
		title.setSize(353, 40); 
		title.setLocation(276, 0); 
		c.add(title);
		
		userLbl = new JLabel("User"); 
		userLbl.setFont(new Font("Arial", Font.PLAIN, 15)); 
		userLbl.setSize(50, 20); 
		userLbl.setLocation(50, 75); 
		c.add(userLbl); 

		user = new JTextField(); 
		user.setFont(new Font("Arial", Font.PLAIN, 15)); 
		user.setSize(150, 20); 
		user.setLocation(105, 75); 
		c.add(user); 

		btnConnection = new JButton("Logon"); 
		btnConnection.setFont(new Font("Arial", Font.PLAIN, 15)); 
		btnConnection.setSize(150, 20); 
		btnConnection.setLocation(260, 75); 
		btnConnection.addActionListener(this); 
		c.add(btnConnection); 		

		printerLbl = new JLabel("Stampanti");
		printerLbl.setFont(new Font("Arial", Font.PLAIN, 15)); 
		printerLbl.setSize(100, 20); 
		printerLbl.setLocation(50, 120); 
		c.add(printerLbl); 
		printer = new JComboBox(printersList); 
		printer.setFont(new Font("Arial", Font.PLAIN, 15)); 
		printer.setSize(200, 20); 
		printer.setLocation(160, 120);
		printer.addActionListener(this);
		c.add(printer);
		
		printerDescriptionLbl = new JLabel("Descr:");
		printerDescriptionLbl.setFont(new Font("Arial", Font.PLAIN, 14)); 
		printerDescriptionLbl.setSize(80, 20); 
		printerDescriptionLbl.setLocation(398, 120); 
		c.add(printerDescriptionLbl); 
		printerDescriptionValueLbl = new JLabel("");
		printerDescriptionValueLbl.setFont(new Font("Arial", Font.PLAIN, 14)); 
		printerDescriptionValueLbl.setSize(388, 20); 
		printerDescriptionValueLbl.setLocation(478, 120); 
		c.add(printerDescriptionValueLbl);
		
		printerPaperLbl = new JLabel("Carta:");
		printerPaperLbl.setFont(new Font("Arial", Font.PLAIN, 14)); 
		printerPaperLbl.setSize(80, 20); 
		printerPaperLbl.setLocation(398, 152); 
		c.add(printerPaperLbl);
		
		printerReportLbl = new JLabel("Report:");
		printerReportLbl.setFont(new Font("Arial", Font.PLAIN, 14)); 
		printerReportLbl.setSize(80, 20); 
		printerReportLbl.setLocation(398, 184); 
		c.add(printerReportLbl);
		printerReportValueLbl = new JLabel("");
		printerReportValueLbl.setFont(new Font("Arial", Font.PLAIN, 14)); 
		printerReportValueLbl.setSize(150, 20); 
		printerReportValueLbl.setLocation(478, 184); 
		c.add(printerReportValueLbl);
		
		printerReportJLbl = new JLabel("Job:");
		printerReportJLbl.setFont(new Font("Arial", Font.PLAIN, 14)); 
		printerReportJLbl.setSize(80, 20); 
		printerReportJLbl.setLocation(398, 215); 
		c.add(printerReportJLbl);
		printerReportJValueLbl = new JLabel("");
		printerReportJValueLbl.setFont(new Font("Arial", Font.PLAIN, 14)); 
		printerReportJValueLbl.setSize(150, 20); 
		printerReportJValueLbl.setLocation(479, 215); 
		c.add(printerReportJValueLbl);

		printerReportSLbl = new JLabel("Son:");
		printerReportSLbl.setFont(new Font("Arial", Font.PLAIN, 14)); 
		printerReportSLbl.setSize(80, 20); 
		printerReportSLbl.setLocation(398, 247); 
		c.add(printerReportSLbl);
		printerReportSValueLbl = new JLabel("");
		printerReportSValueLbl.setFont(new Font("Arial", Font.PLAIN, 14)); 
		printerReportSValueLbl.setSize(150, 20); 
		printerReportSValueLbl.setLocation(478, 247); 
		c.add(printerReportSValueLbl);
		paper = new JComboBox(papersList); 
		paper.setFont(new Font("Arial", Font.PLAIN, 15)); 
		paper.setSize(200, 20); 
		paper.setLocation(478, 152);
		paper.addActionListener(this);
		c.add(paper);

		msgReceived = new JTextArea(); 
		msgReceived.setFont(new Font("Arial", Font.PLAIN, 15)); 
		msgReceived.setSize(315, 75); 
		msgReceived.setLocation(50, 166); 
		msgReceived.setLineWrap(true); 
		c.add(msgReceived); 
		
		JButton btnMontaCarta = new JButton("Monta carta");
		btnMontaCarta.setFont(new Font("Dialog", Font.PLAIN, 15));
		btnMontaCarta.setBounds(50, 245, 150, 20);
		getContentPane().add(btnMontaCarta);
		
		setVisible(true); 
		msgHandler = new MsgHandler(this);
	}
	
	// method actionPerformed() 
	// to get the action performed 
	// by the user and act accordingly 
	public void actionPerformed(ActionEvent e) 
	{ 
		try 
		{
			if (e.getSource() == btnConnection)
			{
				if (btnConnection.getText().compareTo("Logon") == 0)
				{
					try 
					{
						channel.openSocket("localhost", 9001);
					} 
					catch (Exception e1) {
						System.out.println("Eccezione nel collegamento al server");
						e1.printStackTrace();
						System.exit(-1);
					}
					channel.writeSocket(user.getText());
					user.setEditable(false);
					btnConnection.setEnabled(false);
					Thread.sleep(2000);
					btnConnection.setText("Disconnetti");
					while(channel.hasData())
					{
						String message = channel.readSocket();
//						msgReceived.append( message + "\n");
						msgHandler.decodeMsg(message);
					}
					btnConnection.setEnabled(true);
				}
				else
				{
					channel.closeSocket();
					btnConnection.setText("Logon");
				}	
			}
			if (e.getSource() == printer)
			{
				Printer p = printers.get(printer.getSelectedItem());
				printerDescriptionValueLbl.setText(p.getDescription());
				printerReportValueLbl.setText(p.getReport());
				printerReportJValueLbl.setText(p.getReportJ());
				printerReportSValueLbl.setText(p.getReportS());
			}
		} 
		catch (Exception e1) 
		{
			// TODO Auto-generated catch block
			e1.printStackTrace();
		}
	} 
} 