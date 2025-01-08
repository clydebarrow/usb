import	java.io.*;
import	java.nio.*;
import	java.net.*;
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

public class JProgram
{

/*
typedef struct
{
	unsigned long	sig;			// Boot load signature - HTJT
	unsigned long	tag;			// tag for response packet
	unsigned long	addr;			// address to download data
	unsigned short	len;			// length of data
	unsigned char	cmd;			// command
	unsigned char	mem;			// Memory qualifier
}	JCMD_t;


typedef struct
{
	unsigned long	sig;			// response signature - HTJR
	unsigned long	tag;			// matching tag
	unsigned long	value;			// value result
	unsigned short	resid;			// residual length not transferred
	unsigned char	status;			// status - 0 ok, 1 not ok
	unsigned char	sense;			// more info about what is wrong.
}	JRES_t;

*/
 	private static final String     JT_VIDPID = "0x1725:0x0002";
	private static final String     JT_MACDEV = "./HTUSB";
	private static final String	JT_GUID = "{37ABC9BB-51FE-47bd-BACD-A974BC4111E8}";
	private static final int	JT_SIG =	0x544a5448;		// little endian version of signature
	private static final int	JT_RES =	0x524a5448;		// response sig
	private static final int	JT_IDENT =		0x01;		// ident command - response is product number
	private static final int	JT_READMEM =	0x03;		// read memory
	private static final int	JT_WRITEMEM =	0x04;		// write memoryi
	private static final int	JT_READREG =	0x05;		// read reg
	private static final int	JT_WRITEREG =	0x06;		// write reg
	private static final int	JT_SETBRK =		0x07;		// set code breakpoint
	private static final int	JT_CLRBRK =		0x08;		// clear breakpoint
	private static final int	JT_STEP =		0x09;		// step one instruction
	private static final int	JT_CONNECT =	0x0A;		// connect to target - response is chip ID
	private static final int	JT_FIRMWARE =	0x0B;		// go into download mode
	private static final int	JT_PINWRITE =	0x0C;		// write I/O pins and set modes
	private static final int	JT_PINREAD =	0x0D;		// read I/O pins
	private static final int	JT_LIST =		0x0E;		// list supported devices
	private static final int	JT_DEVERASE =	0x0F;		// erase entire device memory
	private static final int	JT_DISCONN =	0x10;		// Disconnect target device
	private static final int	JT_VERIFY =		0x11;		// verify flash memory
	private static final int	JT_GO	=		0x12;		// verify flash memory
	private static final int	JT_STOP	=		0x13;		// verify flash memory
	private static final int	JT_DEBUG=		0x14;		// verify flash memory
	private static final int	JT_ISSTOP=		0x15;		// verify flash memory
	private static final int	JT_EXECINSTR=		0x18;
	private static final int	JT_STARTFROM=		0x19;
	private static final int	JT_READPC=		0x1A;
	private static final int	JT_FDATA=		0x1B;
	private static final int	JT_VERSION=		0x1D;


	private static final int	JT_OK =			0x00;		// 0 means ok
	private static final int	JT_FAIL =		0x01;		// 1 means failed
	// sense codes

	private static final int	JT_BADREQ =		0x01;		// unknown request
	private static final int	JT_BADADDR =	0x02;		// bad address
	private static final int	JT_PROGFAIL =	0x03;		// programming failure
	// memory space codes - mapping between these and particular architectures can
	// vary

	private static final int	JT_PROGMEM =	0x00;		// program memory
	private static final int	JT_DATAMEM =	0x01;		// data memory (internal usually)
	private static final int	JT_EXTMEM =		0x02;		// external data memory
	private static final int	JT_SFR =		0x03;		// special function regs
	private static final int	JT_REG =		0x04;		// device registers
	private static		JProgram	jtag;

	private static final String	cmdNames[] = {
		"UNKNOWN",
		"JT_IDENT",
		"unknown",
		"JT_READMEM",
		"JT_WRITEMEM",
		"JT_READREG",
		"JT_WRITEREG",
		"JT_SETBRK",
		"JT_CLRBRK",
		"JT_STEP",
		"JT_CONNECT",
		"JT_FIRMWARE",
		"JT_PINWRITE",
		"JT_PINREAD",
		"JT_LIST",
		"JT_DEVERASE",
		"JT_DISCONN",
		"JT_VERIFY",
		"JT_GO",
	};


	// device codes - passed as the len argument in a JT_CONNECT command.

	private static final int	JT_SILABSC2 =	0x01;			// Silicon Labs/Cygnal 8051 series with C2 interface
	private static final int	JT_SILABSJT =	0x02;			// Silicon Labs/Cygnal 8051 with JTAG interface

	// tag in firmware file
	private static final int	FW_TAG =	0x554322BF;

	private FileInputStream	rdr;
	private FileOutputStream	wtr;
	private int			sequence;
	private boolean		waiting;
	private Thread		timer, client;
	private long		endTime;
	private int			productCode = 0;
	private Result		lastResult;


	public JProgram(String filename) throws IOException
	{
		Result	r;

		// 2000 millisecond second timeout on both streams
		// indicated by #2000 on end of device space filename

		/*rdr = new FileInputStream(filename + "\\1#2000");
		wtr = new FileOutputStream(filename + "\\0#2000");*/

		if(System.getProperty("os.name").equals("Linux")){
			rdr = new FileInputStream(filename );
			wtr = new FileOutputStream(filename);
		}else{
			rdr = new FileInputStream(filename + "\\1#2000");
			wtr = new FileOutputStream(filename + "\\0#2000");
			/*rdr = new FileInputStream(filename + "\\0#2000");
			wtr = new FileOutputStream(filename + "\\1#2000");*/

		}
		sequence = 1;
		/*r = transact(JT_IDENT);
		if(r.status == JT_OK)
			productCode = r.value;*/
	}

	// write a 4 byte value, little endian, into a buffer
	private void put4(byte[] buf, int offs, int value)
	{
		buf[offs+0] = (byte)(value & 0xFF);
		buf[offs+1] = (byte)((value >> 8) & 0xFF);
		buf[offs+2] = (byte)((value >> 16) & 0xFF);
		buf[offs+3] = (byte)((value >> 24) & 0xFF);
	}

	// write a 4 byte value, little endian, into a buffer
		private void put4(byte[] buf, int offs, long value)
		{
			buf[offs+0] = (byte)(value & 0xFF);
			//System.out.println(Formatter.toHexStringPrefixed(buf[offs+0]& 0xFF));
			buf[offs+1] = (byte)((value >> 8) & 0xFF);
			//System.out.println(Formatter.toHexStringPrefixed(buf[offs+1]& 0xFF));
			buf[offs+2] = (byte)((value >> 16) & 0xFF);
			//System.out.println(Formatter.toHexStringPrefixed(buf[offs+2]& 0xFF));
			buf[offs+3] = (byte)((value >> 24) & 0xFF);
			//System.out.println(Formatter.toHexStringPrefixed(buf[offs+3]& 0xFF));


	}

	// write a 2 byte value, little endian, into a buffer
	private void put2(byte[] buf, int offs, int value)
	{
		buf[offs+0] = (byte)(value & 0xFF);
		buf[offs+1] = (byte)((value >> 8) & 0xFF);
	}

	// get a 4 byte value from a buffer
	private int get4(byte[] buf, int offs)
	{
		int value;

		value = (int)buf[offs+0] & 0xFF;
		value += ((int)buf[offs+1] & 0xFF) << 8;
		value += ((int)buf[offs+2] & 0xFF) << 16;
		value += ((int)buf[offs+3] & 0xFF) << 24;
		return value;
	}

	// get a 2 byte value from a buffer
	private int get2(byte[] buf, int offs)
	{
		int value;

		value = (int)buf[offs+0] & 0xFF;
		value += ((int)buf[offs+1] & 0xFF) << 8;
		return value;
	}

	private static int getInt(String s)
	{
		return Integer.decode(s).intValue();
	}

	private static long getLong(String s)
		{
			return Long.decode(s).intValue();
	}


	private void frameCommandHeader(byte[] comBuffer, int sequence, long address,
			int outlen, int inlen, int cmd , int mem ){
		put4(comBuffer, 0, JT_SIG);
		put4(comBuffer, 4, sequence);
		put4(comBuffer, 8, address);
		if(outlen != 0)
			put2(comBuffer, 12, outlen);
		else
			put2(comBuffer, 12, inlen);
		comBuffer[14] = (byte)cmd;
		comBuffer[15] = (byte)mem;

	}

	Result transact(int cmd, long addr, int mem, byte[] buffer, int outlen, int inlen) throws IOException
	{
		Result	res = new Result();
		byte usbPacket [] = new byte[16+Math.max(outlen, inlen)] ;

		byte jCmd[] = new byte[16];
		res.cmd = cmd;
		sequence++;


		frameCommandHeader(jCmd, sequence, addr, outlen, inlen, cmd, mem);
		System.arraycopy(jCmd, 0, usbPacket,0,16);
		/*if(outlen != 0){
			System.arraycopy(buffer, 0, usbPacket,16,outlen);

		}*/
		wtr.write(jCmd);
		//wtr.write(usbPacket,0 , 16+outlen);
		if(outlen != 0)
			wtr.write(buffer, 0, outlen);


		res.buflen = rdr.read(usbPacket, 0, inlen+16);
		if(res.buflen == -1){
			try{
				Thread.sleep(100);
				res.buflen = rdr.read(usbPacket, 0, inlen+16);
			}catch(Exception e){
				throw new IOException("Internal Error");

			}


		}

		if(res.buflen != inlen+16){
			throw new IOException("Expected "+inlen+16+"Got "+res.buflen);
		}

		if(inlen == 0){

			System.arraycopy(usbPacket, 0 , jCmd, 0, 16);

		}else{
			System.arraycopy(usbPacket, 0 , buffer, 0, inlen);
			System.arraycopy(usbPacket, inlen , jCmd, 0, 16);

		}



			if(get4(jCmd, 0) != JT_RES)
				throw new IOException("Bad signature");
			if(get4(jCmd, 4) != sequence)
				throw new IOException("Sequence error - expected " + sequence + " but saw " + get4(jCmd, 4));
			res.value = get4(jCmd, 8);
			if(inlen != 0)
				res.buflen -= get2(jCmd, 12);
			res.status = jCmd[14];
			res.sense = jCmd[15];
			return res;



	}


	/**
	 * 	Function: transact
	 * 	Send a command to the device and receive a reply.
	 */
/*	Result transact(int cmd, long addr, int mem, byte[] buffer, int outlen, int inlen) throws IOException
	{
		Result	res = new Result();
		byte jCmd[] = new byte[16];
		res.cmd = cmd;
		sequence++;
		put4(jCmd, 0, JT_SIG);
		put4(jCmd, 4, sequence);
		put4(jCmd, 8, addr);
		if(outlen != 0)
			put2(jCmd, 12, outlen);
		else
			put2(jCmd, 12, inlen);
		jCmd[14] = (byte)cmd;

		jCmd[15] = (byte)mem;
		wtr.write(jCmd);
		if(outlen != 0)
			wtr.write(buffer, 0, outlen);
		if(inlen != 0)
			res.buflen = rdr.read(buffer, 0, inlen);
		int read = rdr.read(jCmd, 0, jCmd.length);
		if(read != jCmd.length)
		{
			System.out.println("Asked for " +jCmd.length + " but read " +read);
			throw new IOException("Short read");
		}
		if(get4(jCmd, 0) != JT_RES)
			throw new IOException("Bad signature");
		if(get4(jCmd, 4) != sequence)
			throw new IOException("Sequence error - expected " + sequence + " but saw " + get4(jCmd, 4));
		res.value = get4(jCmd, 8);
		if(inlen != 0)
			res.buflen -= get2(jCmd, 12);
		res.status = jCmd[14];
		res.sense = jCmd[15];
		lastResult = res;
		return res;
	}
*/
	/**
	 * 	Function: transact
	 * 	Send a command to the device and receive a reply.
	 */

	Result transact(int cmd, int addr, int mem, byte[] buffer, int outlen, int inlen) throws IOException
	{
		Result	res = new Result();
		byte jCmd[] = new byte[16];

		res.cmd = cmd;
		sequence++;
		put4(jCmd, 0, JT_SIG);
		put4(jCmd, 4, sequence);
		put4(jCmd, 8, addr);
		if(outlen != 0)
			put2(jCmd, 12, outlen);
		else
			put2(jCmd, 12, inlen);
		jCmd[14] = (byte)cmd;
		/*if(inlen != 0)
			mem |= 0x80;*/
		jCmd[15] = (byte)mem;
		wtr.write(jCmd);
		if(outlen != 0)
			wtr.write(buffer, 0, outlen);
		if(inlen != 0)
			res.buflen = rdr.read(buffer, 0, inlen);
		int read = rdr.read(jCmd, 0, jCmd.length);
		if(read != jCmd.length)
		{
			System.out.println("Asked for " +jCmd.length + " but read " +read);
			throw new IOException("Short read");
		}
		if(get4(jCmd, 0) != JT_RES)
			throw new IOException("Bad signature");
		if(get4(jCmd, 4) != sequence)
			throw new IOException("Sequence error - expected " + sequence + " but saw " + get4(jCmd, 4));
		res.value = get4(jCmd, 8);
		if(inlen != 0)
			res.buflen -= get2(jCmd, 12);
		res.status = jCmd[14];
		res.sense = jCmd[15];
		lastResult = res;
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

	Result transact(int cmd, int addr, int mem) throws IOException
	{
		return transact(cmd, addr, mem, null, 0, 0);
	}

	Result transact(int cmd) throws IOException
	{
		return transact(cmd, 0, 0, null, 0, 0);
	}

	boolean connect(int which) throws IOException
	{
		Result	r;
		int		i;
		byte[]	buffer = new byte[256];


		r = transact(JT_LIST, 0, 0, buffer, 0, buffer.length);
		if(r.status != 0) {
			fail(r);
			return false;
		}
		for(i = 0 ; i != r.buflen/2 ; i++)
			if(which == get2(buffer, i*2))
				break;
		if(i == r.buflen/2) {
			System.err.println("Not supported by this firmware: " + which);
			return false;
		}
		r = transact(JT_CONNECT, which, 0, null, 0, 0);	// connect and read chip ID
		if(r.status != 0) {
			fail(r);
			return false;
		}
		return true;
	}

	boolean erase() throws IOException
	{
		Result	r;
		int		i;
		long	start = System.nanoTime();

		r = transact(JT_DEVERASE);
		if(r.status != 0) {
			fail(r);
			return false;
		}
		start = System.nanoTime() - start;
		System.err.println("Erase succeeded in " + (start/1000000) + "ms");
		return true;
	}

	boolean disconnect() throws IOException
	{
		Result	r;

		r = transact(JT_DISCONN);
		if(r.status != 0) {
			fail(r);
			return false;
		}
		return true;
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

	static final byte[] serialPattern =
	{
 		26, 3,  'S', 0, 'E', 0, 'R', 0, 'I', 0, 'A', 0, 'L', 0,
		'N', 0, 'U', 0, 'M', 0, 'B', 0, 'E', 0, 'R', 0
	};

	static void replaceSerial(byte[] buffer, String serial)
	{
		int	i, j, k;

		for(i = 0 ; i < buffer.length-serialPattern.length ; i++) {
			for(j = 0 ; j != serialPattern.length ; j++)
				if(serialPattern[j] != buffer[i+j])
					break;
			if(j == serialPattern.length) {
				j = serial.length();
				if(j*2+2 > serialPattern.length)
					j = serialPattern.length/2-1;
				i += 2;
				// ensure serial number is at least 12 digits
				if(j*2+2 < serialPattern.length) {
					k = serialPattern.length/2-1-j;
					do {
						buffer[i] = '0';
						i += 2;
					} while(--k != 0);
				}
				for(k = 0 ; k != j ; k++) {
					buffer[i] = (byte)serial.charAt(k);
					i += 2;
				}
				return;
			}
		}
	}

	boolean program(String filename, String serial) throws IOException
	{
		FileInputStream infile = new FileInputStream(filename);
		byte	buffer[] = new byte[20];
		byte	tempbuf[];
		int		addr, len, version, tlen, offs, i;
		Result	r;

		if(infile.read(buffer) != 20 || get4(buffer, 0) != FW_TAG)
			throw new IOException("Bad firmware header");
		//if(get4(buffer, 4) != productCode)
			//throw new IOException("Product code does not match");
		version = get4(buffer, 8);
		addr = get4(buffer, 12);
		len = get4(buffer, 16);
		buffer = new byte[len];
		if(infile.read(buffer) != len)
			throw new IOException("Short read on firmware file");
		infile.close();
		if(serial != null)
			replaceSerial(buffer, serial);
		if(!erase())
			return false;
		tempbuf = new byte[16384];
		offs = 0;
		while(offs != len) {
				tlen = len-offs;
			if(tlen > tempbuf.length)
				tlen = tempbuf.length;
			System.arraycopy(buffer, offs, tempbuf, 0, tlen);
			r = transact(JT_WRITEMEM, addr+offs, JT_PROGMEM, tempbuf, tlen, 0);
			if(r.status != JT_OK) {
				fail(r);
				return false;
			}
			offs += tlen;
		}
		tempbuf = new byte[16384];
		offs = 0;
		while(offs != len) {
				tlen = len-offs;
			if(tlen > tempbuf.length)
				tlen = tempbuf.length;
			r = transact(JT_READMEM, addr+offs, JT_PROGMEM, tempbuf, 0, tlen);
			if(r.status != JT_OK) {
				fail(r);
				return false;
			}
			for(i = 0 ; i != tlen ; i++)
				if(tempbuf[i] != buffer[i+offs]) {
					System.err.println("Verify failure at address " + Formatter.toHexStringPrefixed(addr+offs+i));
					return false;
				}
			offs += tlen;
		}
		return true;
	}

	static public void main(String args[])
	{
		Process	p;
		BufferedReader	br;
		String	s;
		ArrayList	devs;
		Result		r;
		String 			m_usbdevice;


		if(args.length < 1) {
			System.err.println("Usage: JProgram {bootload|firmware|erase|program|readregs|readrom}");
			System.exit(1);
		}
		devs = new ArrayList();
		try {

			if(System.getProperty("os.name").equals("Linux")){


				/*m_usbdriverprocess		= Runtime.getRuntime().exec("/mnt/nimrod/home/vkumar/workspace/enum ", null, null);
				m_usbinputstream		= new BufferedReader(
										new InputStreamReader(m_usbdriverprocess.getInputStream())
										);*/
				m_usbdevice = "/dev/htjtag";
			/*while(( m_usbdevice = m_usbinputstream.readLine()) != null)*/
				devs.add(m_usbdevice);

			if(devs.size() == 0) {
				System.err.println("No devices found");
				System.exit(1);
			}
			m_usbdevice = (String)devs.get(0);
			jtag = new JProgram (m_usbdevice);
			}else if(System.getProperty("os.name").equals("Mac OS X")){
				p = Runtime.getRuntime().exec(JT_MACDEV + " enum " + JT_VIDPID, null, null);
	                        br = new BufferedReader(new InputStreamReader(p.getInputStream()));
	                        while((s = br.readLine()) != null) {
	                                //System.err.println("read " + s);
				        devs.add(s);
				 }
				if(devs.size() == 0) {
				         System.err.println("No devices found");
				          System.exit(1);
				 }

				s = (String)devs.get(0);
                                                                                                                                                                                                                     System.err.println("Using jtag device " +JT_VIDPID + "/" + s);
                                                                                                                                                                                                                     jtag = new JProgram(s);

			}else{
			p = Runtime.getRuntime().exec("enumdev " + JT_GUID, null, null);
			br = new BufferedReader(new InputStreamReader(p.getInputStream()));
			while((s = br.readLine()) != null)
				devs.add(s);
			if(devs.size() == 0) {
				System.err.println("No devices found");
				System.exit(1);
			}
			s = (String)devs.get(0);
			jtag = new JProgram(s);
			}
			if(args[0].equals("bootload")) {
				r = jtag.transact(JT_FIRMWARE);
				System.exit(0);
			} else if(args[0].equals("connect")) {
				if(!jtag.connect(Integer.parseInt(args[1]))){
					fail(jtag.lastResult);
					System.out.println("Chip id = " + Formatter.toHexStringPrefixed(jtag.lastResult.value));
				}else{
					/*r = jtag.transact(JT_STOP, 0, 0, null, 0, 0);
                                        if(r.status != JT_OK) {
						fail(r);
                                                System.exit(0);
                                        }
					System.out.println("Chip id = " + Formatter.toHexStringPrefixed(jtag.lastResult.value));
					System.out.println("Target is stopped");*/
				}
			} else if(args[0].equals("erase")) {
				if(jtag.connect(Integer.parseInt(args[1])))
					jtag.erase();
			}else if(args[0].equals("writemem")) {

				/*if(jtag.connect(Integer.parseInt(args[1]))) {*/
					System.out.println("Sending data");
					long addr = getLong(args[2]);
					long wdata = getLong(args[3]);
					long mem   = getLong(args[4]);
					byte dataout[] = new byte[4];
					dataout[0] = (byte)(wdata & 0xFF);
					dataout[1] = (byte)((wdata>>8) & 0xFF);
					dataout[2] = (byte)((wdata>>16) & 0xFF);
					dataout[3] = (byte)((wdata>>24) & 0xFF);
					System.out.println(mem);

					r = jtag.transact(JT_WRITEMEM, addr, (byte)(mem&0xFF), dataout, 4, 0);
					if(r.status != JT_OK) {
						fail(r);
						System.exit(0);
					}
				//}

			} else if(args[0].equals("readrom")) {
				if(args.length < 3)
					System.err.println("Usage: readrom <device> <addr> <length>");
				else /*if(jtag.connect(Integer.parseInt(args[1])))*/ {
					byte[] buff = new byte[getInt(args[3])];
					long addr = getLong(args[2]);
					r = jtag.transact(JT_READMEM, addr, JT_PROGMEM, buff, 0, buff.length);
					if(r.status != JT_OK) {
						fail(r);
						System.exit(0);
					}
					System.err.println("readrom succeeded: " + Formatter.toHexStringPrefixed(addr) +
							"-" + Formatter.toHexStringPrefixed(addr+buff.length-1));
					FileOutputStream outf = new FileOutputStream("romdata.bin");
					outf.write(buff);
					/*for(int i = 0; i< getInt(args[3]);i+=4){
						outf.write(buff[i+3]);
						outf.write(buff[i+2]);
						outf.write(buff[i+1]);
						outf.write(buff[i]);
					}*/

					outf.close();
					System.err.println("Data written to romdata.bin");
				}
			} else if(args[0].equals("readregs__")) {
				if(args.length < 3)
					System.err.println("Usage: readregs <device> <addr> <length>");
				else if(jtag.connect(Integer.parseInt(args[1]))) {
					byte[] buff = new byte[getInt(args[3])];
					int addr = getInt(args[2]);
					r = jtag.transact(JT_READMEM, addr, JT_SFR, buff, 0, buff.length);
					if(r.status != JT_OK) {
						fail(r);
						System.exit(0);
					}
					System.err.println("readregs succeeded: " + Formatter.toHexStringPrefixed(addr) +
							"-" + Formatter.toHexStringPrefixed(addr+buff.length-1));
					FileOutputStream outf = new FileOutputStream("regdata.bin");
					outf.write(buff);
					outf.close();
					System.err.println("Data written to regdata.bin");
				}
			} else if(args[0].equals("readregs")) {
				if(args.length < 3)
					System.err.println("Usage: readregs <device> <starting reg> <number of regs>");
				else /*if(jtag.connect(Integer.parseInt(args[1])))*/ {
					byte[] buff = new byte[4];
					int reg = getInt(args[2]);
					//r = jtag.transact(JT_READREG, reg, JT_SFR, buff, 0, buff.length);
					long regVal = readCPURegister((long)reg);

					/*if(r.status != JT_OK) {
						fail(r);
						System.exit(0);
					}*/
					System.err.println("readregs succeeded: " + Formatter.toHexStringPrefixed(reg) +
							"-" + Formatter.toHexStringPrefixed(reg+buff.length-1));
					/*System.out.println("0x"+ Formatter.toHexString(buff[3] & 0xFF, 2 , false));
					System.out.println("0x"+ Formatter.toHexString(buff[2] & 0xFF, 2 , false));
					System.out.println("0x"+ Formatter.toHexString(buff[1] & 0xFF, 2 , false));
					System.out.println("0x"+ Formatter.toHexString(buff[0] & 0xFF, 2 , false));*/
					System.out.println("0x"+ Formatter.toHexString(regVal, 8 , false));

					/*System.out.println(Formatter.toHexString(buff[2]& 0xFF));
					System.out.println(Formatter.toHexString(buff[1]& 0xFF));
					System.out.println(Formatter.toHexString(buff[0]& 0xFF));*/

				}
			}else if(args[0].equals("readpc")) {
				if(args.length < 1)
					System.err.println("Usage: readregs <device> ");
				else /*if(jtag.connect(Integer.parseInt(args[1])))*/ {
					byte[] buff = new byte[4];

					r = jtag.transact(JT_READPC, 0, 0, buff, 0, buff.length);
					if(r.status != JT_OK) {
						fail(r);
						System.exit(0);
					}
					System.err.println("readPC succeeded: ");
					System.out.println("0x"+ Formatter.toHexString(buff[3] & 0xFF, 2 , false));
					System.out.println("0x"+ Formatter.toHexString(buff[2] & 0xFF, 2 , false));
					System.out.println("0x"+ Formatter.toHexString(buff[1] & 0xFF, 2 , false));
					System.out.println("0x"+ Formatter.toHexString(buff[0] & 0xFF, 2 , false));

					/*System.out.println(Formatter.toHexString(buff[2]& 0xFF));
					System.out.println(Formatter.toHexString(buff[1]& 0xFF));
					System.out.println(Formatter.toHexString(buff[0]& 0xFF));*/

				}
			} else if(args[0].equals("startfrom")) {
							if(args.length < 3)
								System.err.println("Usage: readregs <device> <starting reg> <number of regs>");
							else /*if(jtag.connect(Integer.parseInt(args[1])))*/ {
								byte[] buff = new byte[4];
								long reg = getLong(args[2]);
								r = jtag.transact(JT_STARTFROM, reg, JT_SFR, null, 0, 0);
								if(r.status != JT_OK) {
									fail(r);
									System.exit(0);
								}

				}

			}else if(args[0].equals("debug")) {
				if(args.length < 1)
					System.err.println("Usage: debug <device>");
				else /*if(jtag.connect(Integer.parseInt(args[1])))*/ {
					byte[] buff = new byte[4];

					r = jtag.transact(JT_DEBUG, 0, JT_SFR, buff, 0, buff.length);
					if(r.status != JT_OK) {
						fail(r);
						System.exit(0);
					}
					System.err.println("debug succeeded: " );
					System.out.println("0x"+ Formatter.toHexString(buff[3] & 0xFF, 2 , false));
					System.out.println("0x"+ Formatter.toHexString(buff[2] & 0xFF, 2 , false));
					System.out.println("0x"+ Formatter.toHexString(buff[1] & 0xFF, 2 , false));
					System.out.println("0x"+ Formatter.toHexString(buff[0] & 0xFF, 2 , false));
				}

			}else if(args[0].equals("v")) {
				if(args.length < 1)
					System.err.println("Usage: debug <device>");
				else /*if(jtag.connect(Integer.parseInt(args[1])))*/ {
					byte[] buff = new byte[4];

					r = jtag.transact(JT_VERSION, 0, JT_SFR, buff, 0, 1);
					if(r.status != JT_OK) {
						fail(r);
						System.exit(0);
					}
					System.err.println("debug succeeded: " );
					System.out.println("0x"+ Formatter.toHexString(buff[3] & 0xFF, 2 , false));
					System.out.println("0x"+ Formatter.toHexString(buff[2] & 0xFF, 2 , false));
					System.out.println("0x"+ Formatter.toHexString(buff[1] & 0xFF, 2 , false));
					System.out.println("0x"+ Formatter.toHexString(buff[0] & 0xFF, 2 , false));
				}

			}
			else if(args[0].equals("writereg")) {


					long reg = getLong(args[2]);
					long wdata = getLong(args[3]);
					byte dataout[] = new byte[4];
					dataout[0] = (byte)(wdata & 0xFF);
				       	System.out.println( Formatter.toHexStringPrefixed (dataout[0]));

					dataout[1] = (byte)((wdata>>8) & 0xFF);
				       	System.out.println( Formatter.toHexStringPrefixed (dataout[1]));

					dataout[2] = (byte)((wdata>>16) & 0xFF);
				       	System.out.println( Formatter.toHexStringPrefixed (dataout[2]));

					dataout[3] = (byte)((wdata>>24) & 0xFF);
				       	System.out.println( Formatter.toHexStringPrefixed (dataout[3]));


					r = jtag.transact(JT_WRITEREG, reg, JT_PROGMEM, dataout, 4, 0);
					if(r.status != JT_OK) {
						fail(r);
						System.exit(0);
					}
					System.err.println("Result: " );
					System.out.println("0x"+ Formatter.toHexString(dataout[3] & 0xFF, 2 , false));
					System.out.println("0x"+ Formatter.toHexString(dataout[2] & 0xFF, 2 , false));
					System.out.println("0x"+ Formatter.toHexString(dataout[1] & 0xFF, 2 , false));
					System.out.println("0x"+ Formatter.toHexString(dataout[0] & 0xFF, 2 , false));
				//}
				}else if(args[0].equals("exec")) {

					long wdata = getLong(args[2]);
					byte dataout[] = new byte[4];
					dataout[0] = (byte)(wdata & 0xFF);
				       	System.out.println( Formatter.toHexStringPrefixed (dataout[0]));

					dataout[1] = (byte)((wdata>>8) & 0xFF);
				       	System.out.println( Formatter.toHexStringPrefixed (dataout[1]));

					dataout[2] = (byte)((wdata>>16) & 0xFF);
				       	System.out.println( Formatter.toHexStringPrefixed (dataout[2]));

					dataout[3] = (byte)((wdata>>24) & 0xFF);
				       	System.out.println( Formatter.toHexStringPrefixed (dataout[3]));


					r = jtag.transact(JT_EXECINSTR, 0, 0, dataout, 4, 4);
					if(r.status != JT_OK) {
						fail(r);
						System.exit(0);
					}
				//}
				}else if(args[0].equals("fdata")) {

					long wdata = getLong(args[2]);
					int direction = getInt(args[3]);
					byte dataout[] = new byte[4];
					/*dataout[0] = (byte)(wdata & 0xFF);
				       	System.out.println( Formatter.toHexStringPrefixed (dataout[0]));

					dataout[1] = (byte)((wdata>>8) & 0xFF);
				       	System.out.println( Formatter.toHexStringPrefixed (dataout[1]));

					dataout[2] = (byte)((wdata>>16) & 0xFF);
				       	System.out.println( Formatter.toHexStringPrefixed (dataout[2]));

					dataout[3] = (byte)((wdata>>24) & 0xFF);
				       	System.out.println( Formatter.toHexStringPrefixed (dataout[3]));*/


					r = jtag.transact(JT_FDATA, wdata, direction, dataout, 0, 4);
					if(r.status != JT_OK) {
						fail(r);
						System.exit(0);
					}

					System.out.println(wdata );
					System.out.println( "Returned Value");
				       	System.out.println( Formatter.toHexStringPrefixed (dataout[0]));
				       	System.out.println( Formatter.toHexStringPrefixed (dataout[1]));
				       	System.out.println( Formatter.toHexStringPrefixed (dataout[2]));
				       	System.out.println( Formatter.toHexStringPrefixed (dataout[3]));
				//}
			}else if(args[0].equals("stop")) {

					r = jtag.transact(JT_STOP, 0, 0, null, 0, 0);
					if(r.status != JT_OK) {
						fail(r);
						System.exit(0);
					}
			}else if(args[0].equals("isstop")) {
					byte data[] = new byte[4];
					r = jtag.transact(JT_ISSTOP, 0, 0, 	data, 0, 1);
					if(r.status != JT_OK) {
						fail(r);
						System.exit(0);
					}
					System.out.print(data[0]);
			}else if(args[0].equals("go")) {

					r = jtag.transact(JT_GO, 0, 0, null, 0, 0);
					if(r.status != JT_OK) {
						fail(r);
						System.exit(0);
					}



			} else if(args[0].equals("program")) {
				if(args.length > 3) {
					int	count = 1, device = Integer.parseInt(args[1]), serialno;
					if(args.length > 4)
						count = Integer.parseInt(args[4]);
					serialno = Integer.parseInt(args[3]);
					for(;;) {
						if(serialno != 0) {
							URL url = new URL("http", "tomcat.htsoft.com", "/intranet/getser.php?serial=" + serialno);
							br = new BufferedReader(new InputStreamReader(url.openStream()));
							s = br.readLine();
							if(s.lastIndexOf("-") != -1)
								s = s.substring(s.lastIndexOf("-")+1);
							if(s.length() > 12)
								s = s.substring(s.length()-12);
							System.err.println("Read serial " + s);
							br.close();
						} else
							s = null;
						for(;;) {
							while(!jtag.connect(device)) {
								System.err.print("*");
								Thread.sleep(500);
							}
							System.err.println("");
							if(!jtag.program(args[2], s)) {
								System.err.println("Programming failed for " + s);
								Thread.sleep(1000);
								continue;
							}
							break;
						}

						System.err.println("Programming successful for " + s);
						if(--count == 0)
							break;
						while(jtag.connect(device)) {
							System.err.print("-");
							Thread.sleep(100);
						}
						System.err.println("");
						serialno++;
					}
				} else
					System.err.println("Usage: program <device> <firmware> <serialno> [<count>]");
			}
			//jtag.disconnect();
		} catch(Exception e)
		{
			System.err.println("Error running enumdev: " + e.getMessage());
			e.printStackTrace();
			System.exit(1);
		}
		System.exit(0);
	}


static long readCPURegister(long regNum){


		byte regVal[] = new byte[4];
		byte regFive[] = new byte[4];

		long nRegVal = 0;
		long nRegFive;
		Result r;

		long instMfC0 = 0;
	//	regNum	= Long.parseLong(regName.substring(2, registerName.length()), 16);
		String selectNum;
		if(regNum == 32){
			//the PC when the program entered into debug mode will b stored in
			// CP0 DEPC. Here the number is manipulated such that DEPC is read
			regNum = 57;
		}

		try{

			if(regNum >32 ){
				//save reg 5
				r = jtag.transact(JProgram.JT_READREG, 5, 0, regFive,0,0/*regFive.length*/);
				nRegFive = r.value;


				//0x40050000 | ((unsigned long)13<<11) )
				instMfC0 = 0x40000000L | ((long)(5<<16))| ((long)((regNum & 0xFF) -33 )<<11);
				if(( regNum / 0x100 ) > 0){
					instMfC0 |= ((regNum/ 0x100) & 0x7);

				}



				/*dataOut[0] = (byte)(instMfC0 & 0xFF);
				dataOut[1] = (byte)((instMfC0>>8) & 0xFF);
				dataOut[2] = (byte)((instMfC0>>16) & 0xFF);
				dataOut[3] = (byte)((instMfC0>>24) & 0xFF);*/

				executeInstruction(instMfC0);


				r = jtag.transact(JProgram.JT_READREG, 5, 0, null,0,0/*regVal.length*/);
				nRegVal = (long)(r.value & 0xFFFFFFFF);

				//restore register 5
				regFive[0] = (byte)(nRegFive & 0xF);
				regFive[1] = (byte)((nRegFive>>8) & 0xFF);
				regFive[2] = (byte)((nRegFive>>16) & 0xFF);
				regFive[3] = (byte)((nRegFive>>24) & 0xFF);

				r = jtag.transact(JProgram.JT_WRITEREG, 5, -1, regFive,4,0);


			}else{
				r = jtag.transact(JProgram.JT_READREG, regNum, JProgram.JT_SFR, regVal,0,0/*regVal.length*/);
				System.out.println(r.value);
				nRegVal = r.value;
				System.out.println("0x"+ Formatter.toHexString(r.value, 8 , false));

			}


			}catch(Exception e){
				System.out.println(e.toString());
			}

		/*regnVal = (long)(regVal[3]<<24)& 0xFF000000;
		regnVal |= (long)(regVal[2]<<16) & 0xFF0000;
		regnVal |= (long)(regVal[1]<<8) & 0xFF00;
		regnVal |= (long)(regVal[0] & 0xFF);*/




		return (nRegVal & 0xFFFFFFFF);

	}

static void executeInstruction (long instruction){
		byte dataOut[] = new byte[4];
		dataOut[0] = (byte)(instruction & 0xFF);
		dataOut[1] = (byte)((instruction>>8) & 0xFF);
		dataOut[2] = (byte)((instruction>>16) & 0xFF);
		dataOut[3] = (byte)((instruction>>24) & 0xFF);
		try{
			jtag.transact(JProgram.JT_EXECINSTR, instruction, -1, dataOut,0,4);
		}catch (Exception e){
		System.out.println(e.toString());

		}

	}


}


