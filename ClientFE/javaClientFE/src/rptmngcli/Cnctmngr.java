package rptmngcli;

import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.Socket;
import java.net.UnknownHostException;
import java.nio.ByteBuffer;
import java.util.Arrays;

public class Cnctmngr {
	private Socket socket = null;
	private InputStreamReader reader = null;
	private InputStream input = null;
	private OutputStream output = null;
	byte[] hdrBuffer = new byte[8];
	byte[] msgBuffer = new byte[1024];
	int offset = 0;
	int readLen = 0;
	int targetLen = 0;
	
	public Cnctmngr()
	{
		
	}
	
	public void openSocket(String hostname, int port) throws Exception
	{
		socket = new Socket(hostname, port);
        input = socket.getInputStream();
        output = socket.getOutputStream();
	}
	
	public String readSocket() throws Exception 
	{
        // read the 5 char header with the total message len
        if ((readLen = input.read(hdrBuffer, 0, 5)) < 5)
        {
        	// incomplete read. Throw exception
        	throw new Exception("Errore sul canale di lettura, ricevuto header incompleto (" + readLen + " bytes)");
        }
        targetLen = Integer.parseInt(new String(hdrBuffer).trim()) - 5;
        System.out.println("Received header " + new String(hdrBuffer) + ". Going to read other " + targetLen + " bytes");
        if ((readLen = input.read(msgBuffer, 0, targetLen)) < targetLen)
        {
        	// incomplete read. Throw exception
        	throw new Exception("Errore sul canale di lettura, ricevuto messaggio incompleto (" + targetLen + " bytes)");
        }
        byte[] outBuf = Arrays.copyOfRange(msgBuffer, 0, targetLen);
		return new String(outBuf);
	}

	
	public void writeSocket(String message) throws Exception 
	{
		targetLen = message.length() + 5;
		byte[] writeBuffer = String.format("%05d%s", targetLen, message).getBytes();	
		output.write(writeBuffer);
	}
	
	public void closeSocket() throws IOException
	{
		socket.close();
	}
	
	public boolean hasData() throws IOException
	{
		return input.available() > 0;
	}
}
