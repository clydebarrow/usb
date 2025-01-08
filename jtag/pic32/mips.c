#include	<8051.h>
#include	<stdlib.h>
#include	"jtag_mips.h"
#include	"mips.h"

/*
 * 	JTAG/C2 interface for Silicon Labs 8051 devices
 *
 * 	JTAG connector ports
 *	P2.5		JTAG pin 11	C2CK	(aka /RST)
 *	P2.3		JTAG pin 15	C2DAT
 *
 */

/*#define	C2CK	(P2_BITS.B5)
#define	C2D		(P2_BITS.B3)

#define	ClkStrobe()	((C2CK = 0), (C2CK = 1))// pulse clock low
#define	ClkEnable()	(P2MDOUT |= (1<<5))		// set clock driver as push/pull
#define	ClkDisable()(P2MDOUT &= ~(1<<5))	// set clock driver as open drain
#define	DatEnable()	(P2MDOUT |= (1<<3))		// set data driver as push/pull
#define	DatDisable() (P2MDOUT &= ~(1<<3))	// set data driver as open drain*/

#define	usDelay(n)	_delay(n*24)			// busy delay in microseconds


/*// C2 Registers
#define	FPDAT		0xB4
#define	FPCTL		0x02
#define	DEVICEID	0x00
#define	REVID		0x01

// C2 status return codes

#define	INVALID_COMMAND	0x00
#define	COMMAND_FAILED	0x02
#define	COMMAND_OK		0x0D

//	C2	interface	commands

#define	GET_VERSION		0x01
#define	BLOCK_READ		0x06
#define	BLOCK_WRITE		0x07
#define	PAGE_ERASE		0x08
#define	DEVICE_ERASE	0x03
#define	REG_READ		0x09
#define	REG_WRITE		0x0A
#define	RAM_READ		0x0B
#define	RAM_WRITE		0x0C*/

// ARM Related Defines
#define IDCODE			0x0E

// FPCTL codes
/*#define INIT_KEY1   0x02        // first key
#define INIT_KEY2   0x01        // second key
#define RESUME_EXEC 0x08        // resume code execution*/

#define TMS	P21
#define TCK	P22
#define TDI	P17
#define NTRST	P16
#define TDO	P20
#define MCLR	P23
//Define for MIPS TAP and ETAP
#define MTAP_SELETAP 0x05
#define MTAP_COMMAND 0x07
#define MTAP_SELMTAP 0x04
#define MTAP_IDCODE  0x01
#define ETAP_ADR    0x08
#define ETAP_DATA    0x09
#define ETAP_CONTROL 0x0A
#define ETAP_BOOT    0x0C
#define ETAP_NBOOT   0x0D
#define ETAP_FDATA   0x0E

//Defines for MTAP DR Commands
#define MTAP_DRSTATUS 	0x00
#define MTAP_ASRT_RST 	0x00
#define MTAP_DASRT_RST 	0x00
#define MTAP_ERASE 	0xFC
#define MTAP_FLSH_ENABLE 0xFE
#define MTAP_FLSH_DISABLE 0xFD

//ETAP Control register bits
#define ETAP_CTRL_EXEC_INSTR	0xC000
#define ETAP_CTRL_DBGMODE	0x8
#define CP0DEBUG_REG		23


static bit flash_inited;		// if we have initialized flash programming

/*Macro to create a rising edge over the TCK line*/
#define PULSE_TCK() ((TCK = 1), (TCK = 0))
xdata int g_no_breakpoints = 0;

static reentrant unsigned char mips_stop_core(void);
//static xdata unsigned long g_icebrkregs[12];
static reentrant unsigned long mips_read_reg(unsigned long reg);
static int g_present_chain ;
static reentrant BOOL mips_write_reg(unsigned long reg, unsigned long value);
static reentrant unsigned char  mips_write_cpsr(unsigned long mask);
static reentrant unsigned long mips_read_cpsr(void);
unsigned char mips_restore_pc(long numdecrement);
static reentrant unsigned long
mips_execute_instr(unsigned  long instr);
unsigned long test;
xdata unsigned long commands[6];
static reentrant unsigned long mips_fastdata(unsigned long data, unsigned char read);




/*Configures the outpiut/ input regietsers
  Bring the TAP state machine to Run-Test Idle state
 */
static void
mips_reset(void)
{
	//Configure P2
	//P2.2 P2.1 P2.4 to output and P2.0 for input
	//P2 Input config register 0xF3
	//P2MDIN  |= 0x10;//P2.4 as input
	//P2MDOUT |= 0x07; // p2.1, p2.2 p2.4 to be pushpul
	TDI = 0;
	TMS = 1;
	//pulse clock for 9
	PULSE_TCK();
	PULSE_TCK();
	PULSE_TCK();
	PULSE_TCK();
	PULSE_TCK();
	PULSE_TCK();


	TMS = 0;
	PULSE_TCK();	// Run-Test/Idle
	TMS = 1;
	PULSE_TCK();	// Select DR
	g_present_chain = -1;


}
/*Clocks out the given 4 bit IR command throu JTAG
 * Assumption: The TAP state machine is at the stage Run-Test/Idle ie this
 * function is called right after the reset*/
static void
mips_shift_ir(unsigned short ircommand, unsigned char to_rtidle, int reg_len)
{
	short i = 0;
	/*Bring the TAP to Select-IR-Scan by setting TMS = 1
	 *and pulsing the TCK*/
	TMS = 1;
	PULSE_TCK();


	/*Bring the TAP to Capture IR state
	 */
	TMS = 0;
	PULSE_TCK();
	// into shift ir state
	PULSE_TCK();
	for(i=reg_len;i!=1;i--){
		TDI = ircommand & 0x01;
		PULSE_TCK();
		ircommand >>= 1;
	}




	TDI = ircommand & 0x01;
	TMS = 1;
	PULSE_TCK();

	/*to UpDate IR*/
	PULSE_TCK();

	//if(to_rtidle == 1 )
	//{
		TMS = 0;
		PULSE_TCK();
	//}

	/*bring back to select dr scan*/

	TMS = 1;
	PULSE_TCK();

}

static unsigned long
mips_write_dr(unsigned long data, int n_bits)
{
	unsigned long data_out = 0;
	int i = 0;

	/*Bring the TAP to Capture DR state
	 */
	TMS = 0;
	PULSE_TCK();
	// into shift dr state
	PULSE_TCK();

	/*Now we are ready to send the data*/
	while(n_bits != 1 )
	{
		/*Data to be shifted in LSB -- MSB order*/
		data_out |= (unsigned long)TDO <<  i ;
		TDI = data & 0x01;
		PULSE_TCK();
		data >>= 1;
		n_bits--;
		i++;
	}
	data_out |= (unsigned long)TDO << i;
	TDI = data & 0x01;
	TMS = 1;
	PULSE_TCK();
	PULSE_TCK();
	TMS = 0; //to run test idle
	PULSE_TCK();


	/*bring back to Select-DR scan*/
	TMS = 1;
	PULSE_TCK();
	return data_out;

}


#if 0
/*Reads the data from TDO
 * Assumption: The TAP state machine is at Run-Test Idle state*/
static unsigned long
mips_read_dr(void)
{
	char i;
	unsigned long data = 0 ;
	/*to Select DR-Scan state*/
//	TMS = 1;
//	PULSE_TCK();
	/*to Capture DR state*/
	TMS = 0;
	PULSE_TCK();
	PULSE_TCK();
	/*now we are ready to read the data from TDO*/
	for(i = 0; i != 31; i++)
	{
		data |= (unsigned long)TDO <<  i ;
		PULSE_TCK();


	}
	/*Now exit the DR related state and recover the last bit*/
	TMS = 1;
	PULSE_TCK();
	data |= (unsigned long)TDO << i;
	PULSE_TCK();
	/*Bring the state machine back to run-test Idle*/
	/*TMS = 0;
	PULSE_TCK();*/
	TMS = 1;
	PULSE_TCK();
	return data;


}

#endif






/* Loads the given binary content to given memory address
 * Input:-
 * data :pointer to the buffer to be written
 * data_len: length of data in bytes
 * address : address of the memory location where the
 * data has to be written to
 **/

static reentrant unsigned char
mips_write_memory(unsigned char xdata *data, unsigned long data_len, unsigned long address, unsigned char mem)
{
	int i =0;
	unsigned long write_in;
	unsigned long reg5 , reg6;
	/*mips_execute_instr(0x3c04ff20);
	mips_execute_instr(0x349f0200);*/

	reg5 = mips_read_reg(5);
	reg6 = mips_read_reg(6);


	//mips_execute_instr(0x30A50000, 32); //a1 set to 0
	for(i =0; i< data_len; i+=4){
		write_in = data[i];
		write_in <<=8;
		write_in |= data[i+1];
		write_in <<=8;
		write_in |= data[i+2];
		write_in<<=8;
		write_in |= data[i+3];
		/*commands[0] = 0x30A50000;
		commands[1] = 0x3CA50000 | (address>>16 ); //store upper address to A1
		commands[2] = 0x34A50000 | (address & 0xFFFF); //and store the lower address by ORing
		commands[3] = 0x3c060000 | (write_in>>16); //store upper 16 bits of data to a2
		commands[4] = 0x34c60000 | (write_in & 0xFFFF    ); //store lower 16 bit to a2
		commands[5] = 0xACA60000 ;*/
		mips_execute_instr( 0x30A50000 );
		mips_execute_instr( 0x3CA50000 | (address>>16 ));
		mips_execute_instr( 0x34A50000 | (address & 0xFFFF ));
		mips_execute_instr( 0x3c060000 | (write_in>>16 ));
		mips_execute_instr( 0x34c60000 | (write_in & 0xFFFF ));
		mips_execute_instr( 0xACA60000 );
		address+=4;





	}
	mips_write_reg(5, reg5);
	mips_write_reg(6, reg6);



	return TRUE;

}
#if 0
static reentrant unsigned long mips_readpc(void){

	mips_execute_instr(0);
	mips_execute_instr(0x04110000L); //BAL rs, 0
	mips_execute_instr(0);
	return mips_read_reg(31);
}
#endif




static reentrant unsigned long
mips_read_reg(unsigned long reg)
{
	unsigned long val = 0, temp_reg = 4 ;

	if(reg == 4 ){
		temp_reg = 5;
	}
	reg = ((reg & 0x1F) << 16L);



	mips_execute_instr(0x0);
	mips_execute_instr(0x40800000 | ((unsigned long)temp_reg<<16) | ((unsigned long)31<<11) ) ; //Store $4 to Scrachpad;
	//mips_execute_instr(0x0);


	mips_execute_instr(0x3c00ff20 | (temp_reg<< 16L));
	mips_execute_instr(0x34000200 |  (temp_reg<< 21L)|  (temp_reg<< 16L));

	mips_execute_instr(0xAC000000L | (temp_reg << 21L ) | reg);
	mips_execute_instr(0);



	mips_shift_ir(ETAP_DATA, 0,  5);
	val =  mips_write_dr(0x0, 32);

	mips_execute_instr(0x0);
	mips_execute_instr(0x40000000 | ((unsigned long)temp_reg<<16) | ((unsigned long)31<<11) ) ; //re store $4 from Scrachpad;
	mips_execute_instr(0x0);



	return val;
}




static reentrant unsigned char mips_go(void)
{
	mips_execute_instr(0x4200001F); // DRET;
	mips_execute_instr(0x0);
	//usDelay(1000);



	return TRUE;

}
static reentrant unsigned char
mips_startfrom(unsigned long pcval){



	mips_write_reg(5 , pcval);
	mips_execute_instr(0x0);

	mips_execute_instr(0x40800000 | ((unsigned long)5<<16) | ((unsigned long)24<<11) ) ; //Store to $5 to DSAVE;
	mips_execute_instr(0xC0); // EHB;

	/*mips_execute_instr(0x0);
	mips_execute_instr((unsigned long)0x408L | (unsigned long)(5L <<21)); //JR.HB
	mips_execute_instr(0x0);
	mips_execute_instr(0x0);
	mips_execute_instr(0x0);
	mips_execute_instr(0x0); 	*/

//	mips_go();
	return TRUE;

}



#define EJTAGCTRL_PRnW 0x1<<19

static reentrant BOOL
mips_write_reg(unsigned long reg, unsigned long value)
{
	unsigned long  temp_reg = 4;

	if(reg == 4 ){
		temp_reg = 5;
	}
	reg = reg & 0x1F;

	mips_execute_instr(0x0);
	mips_execute_instr(0x40800000 | ((unsigned long)temp_reg<<16) | ((unsigned long)31<<11) ) ; //Store $4 to Scrachpad;
	mips_execute_instr(0x0);


	mips_execute_instr(0x3c00ff20 | (temp_reg<< 16L));
	mips_execute_instr(0x34000200 |  (temp_reg<< 21L)|  (temp_reg<< 16L));
	mips_execute_instr(0x0);
	mips_execute_instr(0x8c000000 |(temp_reg<< 21L) | (reg << 16));
	mips_execute_instr(0x0);
	mips_execute_instr(value);

	mips_execute_instr(0x0);
	mips_execute_instr(0x40000000 | ((unsigned long)temp_reg<<16) | ((unsigned long)31<<11) ) ; //re store $4 from Scrachpad;
	mips_execute_instr(0x0);




	return TRUE;



}



#define WPPENDING (~(1L<<22))
#define KERNEL_MODE (~(1L<<4))


static reentrant unsigned char
mips_stop_core(void)
{
	mips_write_dr(0x4D000, 32);
	mips_write_dr(0x4D000, 32);
	return TRUE;
}


/* Reads the ven number of bytes from given address
 * Input:-
 * data :pointer to the buffer to be read
 * data_len: length of data
 * address : address of the memory location where the
 * data has to be written to
 **/
static reentrant unsigned char
mips_read_memory (unsigned char xdata *data, unsigned long data_len, unsigned long address)
{
	int i =0;
	unsigned long data_out = 0;
	unsigned long reg4,/* reg31,*/ reg2, reg3;

	reg4 = mips_read_reg(4);
	//reg31 = mips_read_reg(31);
	reg2 = mips_read_reg(2);
	reg3 = mips_read_reg(3);



	mips_execute_instr(0x3c04ff20); //lui 4 , ff20
	//mips_execute_instr(0x349f0200); //ori 31, 4, 0200
	mips_execute_instr(0x34940200); //ori 4, 4, 0200
	/*mips_execute_instr(0x03e00008);
	mips_execute_instr(0x3c030000);*/

	for(i = 0;i<data_len;){
		commands[0] = 0x00631826L; //XOR V1 V1 V1
		commands[1] = 0x3c030000L | ((address>>16) & 0xFFFF); //LUI V1, upper_address
		commands[2] = 0x8c620000L | (address & 0xFFFF);//LW V0, loweraddress(V1)
		commands[3] = 0xAC820000; //sw V0 0(A0) -- A0 has FF200
		commands[4] = 0;//0x03e00008; //jr 31*/
		/*mips_shift_ir(ETAP_DATA, 0, 5);
		data_out = mips_write_dr(0x0, 32);*/
		//data_out = mips_execute_instr(0, 5);
		//
		mips_execute_instr(commands[0]);
		mips_execute_instr(commands[1]);
		mips_execute_instr(commands[2]);
		mips_execute_instr(commands[3]);
		mips_execute_instr(commands[4]);


		mips_shift_ir(ETAP_DATA, 0,  5);
		data_out = mips_write_dr(0, 32);

		data[i+3] = data_out & 0xFF; data_out>>=8;
		data[i+2] = data_out & 0xFF;data_out>>=8;
		data[i+1] = data_out & 0xFF;data_out>>=8;
		data[i] = data_out & 0xFF;
		i+= 4;
		address+= 4;

	}
	mips_write_reg(4, reg4);
	//mips_write_reg(31, reg31);
	mips_write_reg(2, reg2);
	mips_write_reg(3, reg3);



	return TRUE;

}
static reentrant unsigned long
mips_execute_instr(unsigned long instr){
	mips_shift_ir(ETAP_DATA, 0, 5);
	mips_write_dr(instr, 32);
	mips_shift_ir(ETAP_CONTROL, 0,  5);
	mips_write_dr(ETAP_CTRL_EXEC_INSTR, 32);


	return 0;

}


static reentrant unsigned long
mips_fastdata(unsigned long data, unsigned char read){
	unsigned long data_out;
	int i = 0, n_bits = 31;
	unsigned char oPrAcc;
	mips_shift_ir(MTAP_SELETAP, 0, 5);
	mips_shift_ir(ETAP_FDATA, 0, 5);

		//Bring the TAP to Capture DR state


		TMS = 0;
		PULSE_TCK();
		// into shift dr state
		PULSE_TCK();


		TDI = 0; //PrAcc
		PULSE_TCK();
		oPrAcc =   TDO;

		data_out = oPrAcc << i++ ;


		while(n_bits != 0 )
		{
			TDI = data & 0x01;
			PULSE_TCK();
			data_out = data_out | ((unsigned long)TDO <<  i) ;
			data >>= 1;
			n_bits--;
			i++;
		}

		TDI = data & 0x01;
		TMS = 1;
		PULSE_TCK();
		PULSE_TCK();

		TMS = 0; //to run test idle
		PULSE_TCK();


		/*bring back to Select-DR scan*/
		TMS = 1;
		PULSE_TCK();




	return data_out;

}

static reentrant BOOL
mips_isrunning(void){

	mips_shift_ir(ETAP_CONTROL, 0,  5);
	test = mips_write_dr(0x4C000, 32);
	if((mips_write_dr(0x4C000, 32) & ETAP_CTRL_DBGMODE) == ETAP_CTRL_DBGMODE ){
		return FALSE;
	}
	return TRUE;
}
static reentrant unsigned long
mips_accesscp0reg(unsigned char reg, unsigned long val, unsigned char w){
	unsigned long reg5 = mips_read_reg(5);
	unsigned long instr;
	if(w == 1 ){
		mips_write_reg(5, val);
		instr = 0x40850000;
	}else{
		instr = 0x40050000;
	}
//	test = instr | (((unsigned long)reg & 0xFF) << 11);
	mips_execute_instr(instr |( (unsigned long)reg  << 11));
	//just use the instr variable instead of decaliring a new one
	instr =  mips_read_reg(5);
	mips_write_reg(5, reg5);
	return instr;


}

static reentrant unsigned char
mips_step(void){
	unsigned long debugreg = mips_accesscp0reg(CP0DEBUG_REG,0,0);
	test = debugreg;
	mips_accesscp0reg(CP0DEBUG_REG, debugreg | 0x100100, 1);
	mips_go();
	mips_accesscp0reg(CP0DEBUG_REG, (debugreg | 0x00100000)& ~(1<< 8), 1);
	return TRUE;
}


// connect the target device, get device ID
static reentrant void
mips_chiperase(void){
	TDI = 0;
	MCLR = 0;
	mips_reset();
	mips_shift_ir(0x4, 0, 5);
	mips_shift_ir(0x7, 0, 5);
	test = mips_write_dr(0x0, 8);
	test = mips_write_dr(0x0, 8);
	test = mips_write_dr(0xFE, 8);
	test = mips_write_dr(0x0, 8);
	test = mips_write_dr(0x0, 8);
	test = mips_write_dr(0xFC, 8);
	test = mips_write_dr(0x0, 8);
	test = mips_write_dr(0x0, 8);

}


static reentrant BOOL
mips_connect(void)
{


	TDI = 0;
	MCLR = 0;
	mips_reset();		// reset chip

/*	mips_shift_ir(0x7, 0, 5);
	test = mips_write_dr(0x0, 32);
	//MCLR = 1;
	test = mips_write_dr(0x0, 8);
	test = mips_write_dr(0xFE, 8);
	test = mips_write_dr(0x0, 8);
	test = mips_write_dr(0x0, 8);
	test = mips_write_dr(0xFC, 8);
	test = mips_write_dr(0x0, 8);
	test = mips_write_dr(0x0, 8);*/



	mips_shift_ir(0x1, 0, 5);
	dev_id = mips_write_dr(0x0, 32);
	mips_shift_ir(MTAP_SELETAP, 0, 5);
	mips_shift_ir(ETAP_BOOT, 0, 5);
	MCLR = 1;
	mips_shift_ir(MTAP_SELETAP, 0, 5);
	//mips_stop_core();
	mips_shift_ir(ETAP_CONTROL, 0, 5);
	mips_write_dr(0x4D000, 32);
	if((mips_write_dr(0x4c000, 32) & 0x00008) != 0x00008 ){
		return FALSE;
	}
	test = dev_id;
	return TRUE;
}
// disconnect the target - just reset it

static reentrant BOOL
mips_disconnect(void)
{
	mips_reset();		// reset chip
	return TRUE;
}



// erase the entire device memory

static reentrant BOOL
mips_device_erase(void)
{
	return TRUE;
}


const dev_int 	mips_interface =
{
	JT_ARM7,
	mips_connect,
	mips_device_erase,
	mips_disconnect,
	mips_write_memory,
	mips_read_memory,
	mips_read_reg,
	mips_execute_instr,
	mips_fastdata,
	mips_write_reg,
	mips_stop_core,
	mips_go,
	mips_step,
	mips_startfrom,
	mips_isrunning,
	mips_chiperase

};




