//package	com.htsoft;

import	java.util.*;

/**
 *	A class that provides formatted string conversions that
 *	are missing from java.lang.*
 */

public class Formatter
{
	
	private static char[]	map = { '0', '1', '2', '3', '4', '5', '6', '7',
					'8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
	/**
	 *	Convert a number to a hex string with the specified number
	 *	of digits.
	 *	
	 *	@param	val	The value
	 *	@param	width	The required width (including prefix if present)
	 *	@param	prefix	If true will be prefixed with "0x"
	 */
	public static String toHexString(int val, int width, boolean prefix)
	{
		return toHexString((long)val, width, prefix);
	}

	/**
	 *	Convert a long number to a hex string with the specified number
	 *	of digits.
	 *	
	 *	@param	val	The value
	 *	@param	width	The required width (including prefix if present)
	 *	@param	prefix	If true will be prefixed with "0x"
	 */
	public static String toHexString(long val, int width, boolean prefix)
	{
		StringBuffer	buf;
		int		i, j;
		
		if(width < 18)
			buf = new StringBuffer(18);	// enough for 16 digits + prefix
		else
			buf = new StringBuffer(width+2);
		if(prefix) {
			buf.append("0x");
			width -= 2;
		}
		i = 8;					// max number of digits
		if(width < 1)
			width = 1;
		if(i < width)
			i = width;
		width--;
		do {
			i--;
			if(i >= 8)
				j = 0;
			else
				j = (int)(val >>> (i*4)) & 0xF;
			if(i == width || j != 0) {
				buf.append(map[j]);
				width = i-1;
			}
		} while(i != 0);
		return buf.toString();
	}

	/**
	 *	Convert a number to an unprefixed hex string with the specified number
	 *	of digits.
	 *	
	 *	@param	val	The value
	 *	@param	width	The required number of digits
	 */
	public static String toHexString(int val, int width)
	{
		return toHexString((long)val, width, false);
	}

	public static String toHexString(long val, int width)
	{
		return toHexString(val, width, false);
	}

	/**
	 *	Convert a number to a prefixed hex string with the specified number
	 *	of digits.
	 *	
	 *	@param	val	The value
	 *	@param	width	The required number of digits
	 */
	public static String toHexStringPrefixed(long val, int width)
	{
		return toHexString(val, width, true);
	}

	public static String toHexStringPrefixed(int val, int width)
	{
		return toHexString((long)val, width, true);
	}

	/**
	 *	Convert a number to a hex string with no specified number
	 *	of digits.
	 *	
	 *	@param	val	The value
	 */
	public static String toHexString(long val)
	{
		return toHexString(val, 0, false);
	}

	public static String toHexString(int val)
	{
		return toHexString((long)val, 0, false);
	}

	/**
	 *	Convert a number to a prefixed hex string with no specified number
	 *	of digits
	 *	
	 *	@param	val	The value
	 */
	public static String toHexStringPrefixed(long val)
	{
		return toHexString(val, 0, true);
	}
	
	public static String toHexStringPrefixed(int val)
	{
		return toHexString((long)val, 0, true);
	}
	
	/**
	 *	Take a string and pad it to the specified number of characters
	 *	Spaces are added for padding.
	 */
	public static String padString(String s, int width)
	{
		return padString( s, width, " " );
	}

	/**
	 * pads a string with a specified number of characters with a specified character
	 */
	public static String padString(String s, int width, String character)
	{
		StringBuffer	n;

		if(s.length() >= width)
			return s;
		n = new StringBuffer(width);
		n.append(s);
		width -= s.length();
		do
			n.append(character);
		while(--width != 0);
		return n.toString();
	}
	
	public static void main(String args[])
	{
		if(args.length != 0) {
			//System.out.println(args[0] + " = " + toHexString(Integer.parseInt(args[0], 16), 4));
		}
		System.exit(0);
	}

	/**
	 * returns a value from Hex string
	 */	
	public static long fromHexString(String hex)
	{
		long	val;
		int	i;
		
		val = 0;
		i = 0;
		if(hex == null)
			return 0;
		while(i != hex.length()) {
			switch(hex.charAt(i)) {
				
			case '0': case '1': case '2': case '3': case '4':
			case '5': case '6': case '7': case '8': case '9':
			case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
			case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
				  val *= 16;
				  val += Character.digit(hex.charAt(i), 16);
				  i++;
				  continue;
				  
			default:
				  break;
			}
			break;
		}
		return val;
	}

	/**
	 * checks if string is a valid hex number or not
	 */
	public static boolean isValidHexString( String hex )
	{
		int i = 0;
		
		if( hex == null || hex.equals("") )
			return false;

		while(i != hex.length()) {
			switch(hex.charAt(i)) {
				
			case '0': case '1': case '2': case '3': case '4':
			case '5': case '6': case '7': case '8': case '9':
			case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
			case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
				i++;
				continue;
				  
			default:
				return false;
			}
		}
		return true;
	}

	/**
	 * converts a number to a binary format
	 *
	 * @param val	the value to be returned as a binary string.
	 * @param width	the width of the return string in bits. 
	 * 		If the return string is too wide, it gets truncated.
	 */
	public static String toBinaryString( int val, int width )
	{
		String	str = "";
		int 	mask = 1;
		
		while( width-- > 0 )
		{
			if( (val&mask) == mask )
				str = "1"+str;
			else
				str = "0"+str;

			mask = mask << 1;
		}

		return str;
	}
	
	/**
	 * checks if string is a valid binary number or not
	 */
	public static boolean isValidBinaryString( String bin )
	{
		int i=0;
		
		if( bin == null || bin.equals("") )
			return false;

		while( i != bin.length() )
		{
			switch( bin.charAt(i) )
			{
				case '0':
				case '1':
					i++;
					continue;
				default:
					return false;
			}
		}
	
		return true;
	}

	/**
	 * converts a binary string to an int
	 */
	public static int fromBinaryString( String bin )
	{
		if( bin == null || bin.equals("") )
			return 0;

		int 	value = 0,
			ind = bin.length(),
			i = 0;

		while( i != bin.length() )
		{
			ind--;
			if( bin.charAt(i) == '1' )
				value += Math.pow( 2.0, (double)(ind) );
			i++;
		}

		return value;	
	}
	
}

