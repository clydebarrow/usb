import	java.io.*;
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

public class HTUpdate
{


	/*
	 *
	 * C Defines...

typedef struct
{
	unsigned long	sig;			// Boot load signature - HTBL
	unsigned long	tag;			// tag for response packet
	unsigned long	addr;			// address to download data
	unsigned char	cmd;			// command
	unsigned char	len;			// length of data
	unsigned char   pad[2];         // dummies to pad it out
}	BCMD;

#define	BL_SIG		0x4854424c		// big endian
#define	BL_RES		0x48544252		// response sig

#define	BL_SIG		0x4c425448		// little endian
#define	BL_RES		0x52425448		// little endian

typedef struct
{
	unsigned long	sig;			// response signature - HTBR
	unsigned long	tag;			// matching tag
	unsigned long	value;			// value if required
	unsigned char	status;			// status - 0 ok, 1 not ok
	unsigned char	sense;			// more info about what is wrong.
	unsigned char   pad[2];         // dummies to pad it out

}	BRES;

#define	BL_IDENT	0x01			// ident command - response is product number
#define	BL_DNLD		0x02			// download data
#define	BL_DONE		0x03			// download complete
#define	BL_QBASE	0x04			// query base for download

// sense codes

#define	BL_BADREQ	0x01			// unknown request
#define	BL_BADADDR	0x02			// bad address
#define	BL_PROGFAIL	0x03			// programming failure

*/

/* Bootloader stuff */

	private static final String	BL_VIDPID = "0x1725:0xFFFF";
	private static final int	BL_SIG =	0x4c425448;		// little endian
	private static final int	BL_RES =	0x52425448;		// little endian
	private static final int	BL_IDENT =	0x01;			// ident command - response is product number
	private static final int	BL_DNLD =	0x02;			// download data
	private static final int	BL_DONE	=	0x03;			// download complete
	private static final int	BL_QBASE =	0x04;			// query base for download

	private static final int	BL_OK = 	0x00;			// ok
	private static final int	BL_BADREQ =	0x01;			// unknown request
	private static final int	BL_BADADDR =	0x02;			// bad address
	private static final int	BL_PROGFAIL =	0x03;			// programming failure


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
	private static final int	FW_TAG =	0x554322BF;

	private InputStream	rdr;
	private OutputStream	wtr;
	private int			sequence;
	private int			productCode = 0;
	private int			dlAddr = 0;
	private Result		lastResult;


	public HTUpdate(String serno) throws IOException
	{
		Result	r;
		Process	p;
		BufferedReader	br;
		BufferedWriter	bw;

		//System.err.println("Running " +JT_MACDEV + " open " + BL_VIDPID + " " + serno);
		p = Runtime.getRuntime().exec(JT_MACDEV + " open " + BL_VIDPID + " " + serno, null, null);

		rdr = p.getInputStream(); 
		wtr = p.getOutputStream(); 
		sequence = 0xFFFFFFFF;
		r = transact(BL_IDENT);
		if(r.status == BL_OK) {
			productCode = r.value;
			r = transact(BL_QBASE);
			if(r.status == BL_OK)
				dlAddr = r.value;
		}
	}


	// write a 4 byte value, little endian, into a buffer
	private void put4(byte[] buf, int offs, int value)
	{
		buf[offs+0] = (byte)(value & 0xFF);
		buf[offs+1] = (byte)((value >> 8) & 0xFF);
		buf[offs+2] = (byte)((value >> 16) & 0xFF);
		buf[offs+3] = (byte)((value >> 24) & 0xFF);
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

	/**
	 * 	Function: transact
	 * 	Send a command to the device and receive a reply.
	 */

	Result transact(int cmd, int addr, byte[] buffer, int len) throws IOException
	{
		Result	res = new Result();
		byte jCmd[] = new byte[16];

		res.cmd = cmd;
		//System.err.println("Transact: " + cmd + "/" + sequence);
		put4(jCmd, 0, BL_SIG);
		put4(jCmd, 4, sequence);
		put4(jCmd, 8, addr);
		jCmd[12] = (byte)cmd;
		jCmd[13] = (byte)len;
		jCmd[14] = (byte)0;
		jCmd[15] = (byte)0;
		wtr.write(jCmd);
		wtr.flush();
		try {
			Thread.sleep(10);
		} catch(Exception e)
		{
		}
		if(len != 0) {
			wtr.write(buffer, 0, len);
			wtr.flush();
		}
		//System.err.println("Reading response");
		if(rdr.read(jCmd, 0, jCmd.length) != jCmd.length)
			throw new IOException("Short read");
		if(get4(jCmd, 0) != BL_RES)
			throw new IOException("Bad signature");
		if(get4(jCmd, 4) != sequence)
			throw new IOException("Sequence error - expected " + sequence + " but saw " + get4(jCmd, 4));
		res.value = get4(jCmd, 8);
		res.status = jCmd[12];
		res.sense = jCmd[13];
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


	Result transact(int cmd) throws IOException
	{
		return transact(cmd, 0, null, 0);
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
		int		addr, len, version, tlen, offs, i;
		Result	r;

		if(infile.read(buffer) != 20 || get4(buffer, 0) != FW_TAG) 
			throw new IOException("Bad firmware header");
		i = get4(buffer, 4);
		if(i != productCode)
			throw new IOException("Product code " + i + " should be "+ productCode);
		version = get4(buffer, 8);
		addr = get4(buffer, 12);
		if(addr != dlAddr)
			throw new IOException("Download addr " + Formatter.toHexStringPrefixed(addr) + 
					" should be " + Formatter.toHexStringPrefixed(dlAddr));
		len = get4(buffer, 16);
		buffer = new byte[len];
		if(infile.read(buffer) != len)
			throw new IOException("Short read on firmware file");
		infile.close();
		tempbuf = new byte[64];
		offs = 0;
		while(offs != len) {
				tlen = len-offs;
			if(tlen > tempbuf.length)
				tlen = tempbuf.length;
			System.arraycopy(buffer, offs, tempbuf, 0, tlen);
			r = transact(BL_DNLD, addr+offs, tempbuf, tlen);
			if(r.status != BL_OK) {
				fail(r);
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
		HTUpdate	jtag = null;
		Result		r;

		if(args.length != 1) {
			System.err.println("Usage: HTUpdate <firmware file>");
			System.exit(1);
		}
		devs = new ArrayList();
		try {
			//System.err.println("Running: " + JT_MACDEV + " enum " + JT_VIDPID);
			p = Runtime.getRuntime().exec(JT_MACDEV + " enum " + BL_VIDPID, null, null);
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
			System.err.println("Using jtag device " +BL_VIDPID + "/" + s);
			jtag = new HTUpdate(s);
			jtag.program(args[0]);
		} catch(Exception e)
		{
			System.err.println("Exception: " + e.getMessage());
			e.printStackTrace();
			System.exit(1);
		}
		try {
			jtag.disconnect();
		} catch(Exception e) {
		}
		System.exit(0);
	}
}


