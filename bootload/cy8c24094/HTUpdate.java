package com.htsoft.htupdate;
import	java.io.*;
import	java.awt.*;
import	java.awt.event.*;
import	javax.swing.ImageIcon;

//import	com.htsoft.util.Formatter;
//import	java.nio.*;
//import	java.nio.channels.*;
import	java.util.ArrayList;



class Result
{
	int		buflen;
	int		value = 0;
	int		status =0, sense = 0;
	int		cmd;
}



class FirmwareFileNameField extends TextField{
	
	public FirmwareFileNameField(){
		super("Load the application");
		setEditable(true);
		
	}
}

public class HTUpdate extends Frame implements ActionListener
{





	private static final String TITLE = "CPSOC Bootloader";

/* Bootloader stuff */

	private static final String	BL_VIDPID 	= "0x1725:0xFFFF";
	private static final int	BL_SIG 		=	0x4c425448;		// little endian
	private static final int	BL_RES 		=	0x52425448;		// little endian
	public	 static final int	BL_IDENT 	=	0x01;			// ident command - response is product number
	private static final int	BL_DNLD 	=	0x02;			// download data
	private static final int	BL_DONE		=	0x03;			// download complete
	private static final int	BL_QBASE 	=	0x04;			// query base for download	
	private static final int	BL_VERIFY	=	0x05;			// verifies memory with the given chunk of data
	private static final int	BL_INIT		=	0x06;			// initializes flash delay and clock

	private static final int	BL_OK 		= 	0x00;			// ok
	


/* JTAG programming stuff */

	private static final String	JT_GUID = "{37ABC9BB-51FE-47bd-BACD-A974BC4111E8}";
	private static final String	JT_DEV = "enumdev";
	private static final String	JT_MACDEV = "./HTUSB";


	private static final String	cmdNames[] = {
		"UNKNOWN",
		"BL_IDENT",
		"BL_DNLD",
		"BL_DONE",
		"BL_QBASE",
	};


	// device codes - passed as the len argument in a JT_CONNECT command.

	// tag in firmware file
	private static final int	FW_TAG =	0xBF224355;//0x554322BF;

	private InputStream	rdr;
	private OutputStream	wtr;
	private int			sequence;
	private int			productCode = 0x94040000;//494;	
	private Button  m_loadButton = new Button("Browse");
	private Button  ProgramButton = new Button("Program");	
	private TextArea	m_statusText = new TextArea("Select fimware to load",5, 60);
	private FirmwareFileNameField m_firmwareFileNameField = new FirmwareFileNameField();
	
	//error

	private static final int	BL_BADREQ		=	0x01;			// unknown request
	private static final int BL_PROGFAIL		=	0x02;			// programming failure
	private static final int BL_VERIFYERR 		=	0x03;
	private static final int BL_NOINIT			=	0x04;



	public HTUpdate() throws Exception
	{
		
		super(TITLE);
		Process	enumdevProc;
		BufferedReader enudevIStream;
		String enumdevOutput;
		ArrayList	deviceList  = new ArrayList();
		
		ImageIcon icon = new ImageIcon("hitech.jpg");
			
		this.setIconImage(icon.getImage());		
		this.setLayout(new GridBagLayout());	
		
		GridBagConstraints gbc = new GridBagConstraints ();
		
		gbc.gridx = 0;
		gbc.gridy = 0;		
		gbc.gridwidth = 22;
		gbc.gridheight = 1;
		gbc.anchor = GridBagConstraints.WEST;
		((GridBagLayout)this.getLayout()).setConstraints(m_firmwareFileNameField, gbc);
		
		gbc.gridx = 23;
		gbc.gridy = 0;
		gbc.gridwidth = 1;
		gbc.gridheight = 1;
		gbc.fill = 10;
		gbc.anchor = GridBagConstraints.WEST;
		((GridBagLayout)this.getLayout()).setConstraints(m_loadButton, gbc);
		
		
		
		
		
		gbc.gridx = 24;
		gbc.gridy = 0;		
		gbc.gridwidth = 1;
		gbc.gridheight = 1;
		gbc.fill = 10;
		gbc.anchor = GridBagConstraints.WEST;
		((GridBagLayout)this.getLayout()).setConstraints(ProgramButton, gbc);
		
		
		
		gbc.gridx = 0;
		gbc.gridy = 3;		
		gbc.gridwidth = 25;
		gbc.gridheight = 4;		
		gbc.anchor = GridBagConstraints.WEST;
		((GridBagLayout)this.getLayout()).setConstraints(m_statusText, gbc);
		
		
		this.setSize(600, 200);
		this.setMenuBar(new MenuBar());
		this.add(m_loadButton);
		this.add(m_firmwareFileNameField);
		this.add(ProgramButton);
		this.add(m_statusText);
		
		
		m_loadButton.addActionListener(this);
		ProgramButton.addActionListener(this);
		this.enableEvents(AWTEvent.WINDOW_EVENT_MASK);
				
		this.setVisible(true);
		//ll.setVisible(true);
		
		enumdevProc = Runtime.getRuntime().exec("./enumdev " + JT_GUID, null, null);
		m_statusText.setText(enumdevProc.toString());
		enudevIStream = new BufferedReader(new InputStreamReader(enumdevProc.getInputStream()));
		while((enumdevOutput = enudevIStream.readLine()) != null) {
			//System.err.println("read " + s);
			deviceList.add(enumdevOutput);
		}
		if(deviceList.size() == 0) {
			m_statusText.setText("No devices found");
			
			return;
			
		}
		enumdevOutput = (String)deviceList.get(0);
		
		m_statusText.setText("Using jtag device " +BL_VIDPID );
		
		rdr = new FileInputStream(enumdevOutput + "\\1#2000");
		wtr = new FileOutputStream(enumdevOutput + "\\0#2000");
		
		sequence = 0xFFFFFFFF;
		
	}
	
	public void actionPerformed(ActionEvent e){
		FileDialog fileDia = new FileDialog(this);
		String command  = e.getActionCommand();
		if(command.equals("Browse")){
			fileDia.setVisible(true);			
			m_firmwareFileNameField.setText(fileDia.getDirectory()+fileDia.getFile());
			m_firmwareFileNameField.setEditable(true);
		}
		if(command.equals("Program")){
			m_statusText.setText("Program starting");
			try{
				program(m_firmwareFileNameField.getText());
			}catch(IOException ioe){
				
				m_statusText.setForeground(Color.RED);
				m_statusText.setText(ioe.getMessage());				
				
			}			
		}		
	}
	protected void processEvent(AWTEvent e){
		super.processEvent(e);
		if(e.getID() == 201){
			this.dispose();
		}
		System.out.println(e.getID());
	}


	// write a 4 byte value, little endian, into a buffer
	private void put4(byte[] buf, int offs, int value)
	{
		buf[offs+3] = (byte)(value & 0xFF);
		buf[offs+2] = (byte)((value >> 8) & 0xFF);
		buf[offs+1] = (byte)((value >> 16) & 0xFF);
		buf[offs+0] = (byte)((value >> 24) & 0xFF);
	}

	// write a 4 byte value, little endian, into a buffer
	private void put4(byte[] buf, int offs, long value)
	{
		buf[offs+3] = (byte)(value & 0xFF);			
		buf[offs+2] = (byte)((value >> 8) & 0xFF);			
		buf[offs+1] = (byte)((value >> 16) & 0xFF);			
		buf[offs+0] = (byte)((value >> 24) & 0xFF);
	}
		
	// write a 2 byte value, little endian, into a buffer
	private void put2(byte[] buf, int offs, int value)
	{
		buf[offs+1] = (byte)(value & 0xFF);
		buf[offs+0] = (byte)((value >> 8) & 0xFF);
	}

	// get a 4 byte value from a buffer
	private int get4(byte[] buf, int offs)
	{
		int value;

		value = (int)buf[offs+3] & 0xFF;
		value += ((int)buf[offs+2] & 0xFF) << 8;
		value += ((int)buf[offs+1] & 0xFF) << 16;
		value += ((int)buf[offs+0] & 0xFF) << 24;
		return value;
	}
	
	private int get4li(byte[] buf, int offs)
	{
		int value;

		value = (int)buf[offs+0] & 0xFF;
		value += ((int)buf[offs+1] & 0xFF) << 8;
		value += ((int)buf[offs+2] & 0xFF) << 16;
		value += ((int)buf[offs+3] & 0xFF) << 24;
		return value;
	}

	
	
	private void frameCommandHeader(byte[] comBuffer, int sequence, long address, 
			int outlen, int inlen, int cmd ){
		put4(comBuffer, 0, BL_SIG);
		put4(comBuffer, 4, sequence);
		put4(comBuffer, 8, address);
		
		if(outlen != 0){
			comBuffer[13] = (byte)(0xFF & outlen);
			comBuffer[12] = (byte)(0xFF & outlen >> 8);
			//put2(comBuffer, 12, outlen);
		}else
			put2(comBuffer, 12, inlen);
		comBuffer[14] = (byte)cmd;		
		comBuffer[15] = (byte)0x00;
		
		
	}

	

	/**
	 * 	Function: transact
	 * 	Send a command to the device and receive a reply.
	 */

	public Result transact(int cmd, int addr, byte[] buffer, int outlen, int inlen) throws IOException
	{
		Result	res = new Result();
		byte jCmd[] = new byte[16];
		byte usbPacket [] = new byte[jCmd.length+Math.max(outlen, inlen)] ;
		sequence = 1;

		res.cmd = cmd;
		//System.err.println("Transact: " + cmd + "/" + sequence);
		/*put4(jCmd, 0, BL_SIG);
		put4(jCmd, 4, sequence);
		put4(jCmd, 8, addr);
		jCmd[12] = (byte)cmd;
		jCmd[13] = (byte)len;
		jCmd[14] = (byte)0;
		jCmd[15] = (byte)0;*/
		
		frameCommandHeader(jCmd, sequence, addr, outlen, inlen, cmd);
		System.arraycopy(jCmd, 0, usbPacket,0,jCmd.length);
		
		wtr.write(jCmd);
		wtr.flush();
		try {
			Thread.sleep(10);
		} catch(Exception e)
		{
		}
		if(outlen != 0) {
			wtr.write(buffer, 0, outlen);
			wtr.flush();
		}
		/*try {
			Thread.sleep(100);
		} catch(Exception e)
		{
		}*/
		
		
		res.buflen = rdr.read(usbPacket, 0, inlen+16);
		/*res.buflen = rdr.read(usbPacket, 0, 64);
		res.buflen = rdr.read(usbPacket, 0, 20);*/
		if(res.buflen == -1){
			try{
				Thread.sleep(100);				
				res.buflen = rdr.read(usbPacket, 0, inlen+jCmd.length);
			}catch(Exception e){
				throw new IOException("Internal Error");
				
			}
			
			
		}
		
		if(res.buflen != inlen+16){
			res.buflen = rdr.read(usbPacket, 0, inlen+jCmd.length);
			
			throw new IOException("Expected "+inlen+16+"Got "+res.buflen);					
		}
		
		if(inlen == 0){
			
			System.arraycopy(usbPacket, 0 , jCmd, 0, jCmd.length);			
			
		}else{
			System.arraycopy(usbPacket, 0 , buffer, 0, inlen);	
			System.arraycopy(usbPacket, inlen , jCmd, 0, jCmd.length);	
			
		}
	
		
		//System.err.println("Reading response");
		/*if(rdr.read(jCmd, 0, jCmd.length) != jCmd.length)
			throw new IOException("Short read");*/
		if(get4(jCmd, 0) != BL_RES)
			throw new IOException("Bad signature");
		if(get4(jCmd, 4) != sequence)
			throw new IOException("Sequence error - expected " + sequence + " but saw " + get4(jCmd, 4));
		res.value = get4(jCmd, 8);
		res.status = jCmd[12];
		res.sense = jCmd[13];		
		return res;
	}

	static void fail(Result r)
	{
		String	s;

		if(r.cmd < cmdNames.length)
			s = cmdNames[r.cmd];
		else
			s = Integer.toString(r.cmd);
		System.err.println(s + " failed: sense = " + r.sense + " buflen = " + r.buflen);
	}


	Result transact(int cmd) throws IOException
	{
		return transact(cmd, 0, null, 0, 0);
	}


	boolean disconnect() throws IOException
	{
		Result	r;

		r = transact(BL_DONE);
		rdr.close();
		wtr.close();
		if(r.status != 0) {
			fail(r);
			return false;
		}
		return true;
	}
	
	private String getError(int errNum){
		

		
		switch (errNum){
		case BL_BADREQ:
			return "Bootloader could not understand the command";
		
		case BL_PROGFAIL:
			return "Failed to write the flash. May be a the device protection does not allow writing";
		
		case BL_VERIFYERR:
			return "Verification falied";
			
		case BL_NOINIT:
			return "Device delay variables are not initialised";
			
			
		}
		return "unknown";
	}

/*
 * 	The firmware file header

	typedef struct
	{
		unsigned long	tag;
		unsigned long	product;
		unsigned long	version;
		unsigned long	addr;
		unsigned long	size;
	}	firmware;
 */


	boolean program(String filename) throws IOException
	{
		FileInputStream infile = new FileInputStream(filename);
		byte	buffer[] = new byte[20];
		byte	tempbuf[];
		int		addr, len, tlen, offs, i;
		Result	r;
		r = transact(BL_IDENT, 0, null, 0, 0);
		
		r = transact(BL_QBASE, 0, null, 0, 0);
		/*r = transact(BL_IDENT, 0, null, 0, 0);
		r = transact(BL_IDENT, 0, null, 0, 0);
		r = transact(BL_IDENT, 0, null, 0, 0);
		r = transact(BL_IDENT, 0, null, 0, 0);*/
		
		m_statusText.setText("Identified device 0x"+Integer.toHexString(r.value)+"\n");
		
		
		//r = transact(BL_INIT, 0, null, 0, 0);
		

		if(infile.read(buffer) != 20 || get4(buffer, 0) != FW_TAG) 
			throw new IOException("Bad firmware header");
		i = get4(buffer, 4);
		/*if(i != productCode)
			throw new IOException("Product code " + i + " should be "+ productCode);*/
		
		addr = get4li(buffer, 12);
		/*if(addr != dlAddr)
			throw new IOException("Download addr " + Formatter.toHexStringPrefixed(addr) + 
					" should be " + Formatter.toHexStringPrefixed(dlAddr));*/
		len = get4li(buffer, 16);
		buffer = new byte[len];
		if(infile.read(buffer) != len)
			throw new IOException("Short read on firmware file");
		infile.close();
		tempbuf = new byte[64];
		offs = 0;
		
		this.m_statusText.setText("Loading the firmware");
		
		while(offs != len) {
				tlen = len-offs;
			if(tlen > tempbuf.length)
				tlen = tempbuf.length;
			System.arraycopy(buffer, offs, tempbuf, 0, tlen);
			r = transact(BL_DNLD, addr+offs, tempbuf, tlen, 0);
			if(r.status != BL_OK) {
				String error = new String("Bootloader error while writing record at "+ Long.toString(addr+offs) +" "+ 
										 getError(r.sense));
				throw (new IOException (error));
				//return false;
			}
			this.m_statusText.append(".");
			if(m_statusText.getText().length() % 120 == 0){
				this.m_statusText.append("\n");
				
			}
			offs += tlen;
		}
		this.m_statusText.setText("Finished writing");
		this.m_statusText.append("\nVerifying");
		
		offs = 0;
		while(offs != len) {
			tlen = len-offs;
			if(tlen > tempbuf.length)
				tlen = tempbuf.length;
			System.arraycopy(buffer, offs, tempbuf, 0, tlen);
			r = transact(BL_VERIFY, addr+offs, tempbuf, tlen, 0);
			if(r.status != BL_OK) {
				String error = new String("Bootloader error while verifying record at "+ Long.toHexString(addr+offs) +" "+ 
						 getError(r.sense));
				throw (new IOException (error));

			}
			this.m_statusText.append(".");
			if(m_statusText.getText().length() % 120 == 0){
				this.m_statusText.append("\n");
				
			}
			offs += tlen;
		}	
			
		
		this.m_statusText.append("\nStating user application");
		
		r = transact(BL_DONE, 0, null, 0, 0);
		return true;
		
	}

	static public void main(String args[])
	{
		
		
		
		try {
			
			new HTUpdate();
		}			
		catch(Exception e)
		{
			System.out.println("Exception: " + e.getMessage());
			System.err.println("Exception: " + e.getMessage());
			e.printStackTrace();
			System.exit(1);
		}
		
	}
}


