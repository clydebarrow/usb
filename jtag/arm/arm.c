#include	<8051.h>
#include	"jtag_arm.h"
#include	"arm.h"

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
#define TDI	P20
#define TDO	P24


static bit flash_inited;		// if we have initialized flash programming

/*Macro to create a rising edge over the TCK line*/
#define PULSE_TCK() ((TCK = 0), (TCK = 1))
xdata break_points_st g_break_points_t[20]; 
xdata int g_no_breakpoints = 0;

static reentrant unsigned long
arm_read_icebreakreg(unsigned char reg);
static reentrant void
arm_write_icebreakreg(unsigned char reg, unsigned long data);
static reentrant unsigned char arm_stop_core(void);
//static xdata unsigned long g_icebrkregs[12];
static reentrant unsigned long arm_read_reg(unsigned long reg);
static int g_present_chain ;
static reentrant BOOL arm_write_reg(unsigned long reg, unsigned long value);
static reentrant unsigned char  arm_write_cpsr(unsigned long mask);
static reentrant unsigned long arm_read_cpsr(void);
unsigned char arm_restore_pc(long numdecrement);




/*Configures the outpiut/ input regietsers
  Bring the TAP state machine to Run-Test Idle state
 */
static void
arm_reset(void)
{
	//Configure P2
	//P2.2 P2.1 P2.4 to output and P2.0 for input
	//P2 Input config register 0xF3
	//P2MDIN  |= 0x10;//P2.4 as input
	//P2MDOUT |= 0x07; // p2.1, p2.2 p2.4 to be pushpul

	TMS = 1;
	//pulse clock for 9
	PULSE_TCK();
	PULSE_TCK();
	PULSE_TCK();
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
arm_shift_ir(unsigned short ircommand, unsigned char to_rtidle)
{


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

	/*Now we are ready to send the ircommand*/
	TDI = ircommand & 0x01;
	PULSE_TCK();
	ircommand >>= 1;

	TDI = ircommand & 0x01;
	PULSE_TCK();
	ircommand >>= 1;

	TDI = ircommand & 0x01;
	PULSE_TCK();
	ircommand >>= 1;

	TMS = 1;
	TDI = ircommand & 0x01;
	PULSE_TCK();

	/*to UpDate IR*/
	PULSE_TCK();

	if(to_rtidle == 1 )
	{
		TMS = 0;
		PULSE_TCK();
	}

	/*bring back to select dr scan*/

	TMS = 1;
	PULSE_TCK();

}

static void
arm_write_dr(unsigned long data, int n_bits)
{

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
		TDI = data & 0x01;
		PULSE_TCK();
		data >>= 1;
		n_bits--;
	}
	TDI = data & 0x01;
	TMS = 1;
	PULSE_TCK();

	/*to UpDate DR*/
	PULSE_TCK();
	/*bring back to Select-DR scan*/

	TMS = 1;
	PULSE_TCK();

}



/*Reads the data from TDO
 * Assumption: The TAP state machine is at Run-Test Idle state*/
static unsigned long
arm_read_dr(void)
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
		PULSE_TCK();
		data |= (unsigned long)TDO <<  i ;


	}
	/*Now exit the DR related state and recover the last bit*/
	TMS = 1;
	PULSE_TCK();
	data |= (unsigned long)TDO << i;
	PULSE_TCK();
	/*Bring the state machine back to run-test Idle*/
	TMS = 0;
	PULSE_TCK();
	TMS = 1;
	PULSE_TCK();
	return data;


}

/*Selecting the scan chain
 * Special requirement: Run Test Idle state should never be reached
 * during the process of scan chain selection*/
void
arm_select_scanchain(unsigned char scan_no)
{


	arm_shift_ir(0x02, 0);
	arm_write_dr(scan_no, 4 );
 	arm_shift_ir(0x0C, 0);
	g_present_chain = scan_no;




}



/*Clocks out the given 32 bit data throu JTAG DR
 * Assumption: The TAP state machine is at the stage Run-Test/Idle ie this
 * function is called right after the reset*/
static unsigned long
arm_execute_instr(unsigned long data_out, unsigned char breakpt)
{
	unsigned long data_in = 0;
	int i;

	/*Bring the TAP to Capture DR state
	 */
	TDI = 0;
	TMS = 0;
	PULSE_TCK();
	PULSE_TCK(); // Shift DR


	/*Send in the BREAKPT bit*/
	TDI = breakpt & 0x01;
	PULSE_TCK();
	data_in |= TDO;
	data_in = 0; /*ignore the incoming Breakpoint bit*/

	/*Now we are ready to send the data*/
	for(i = 0 ; i != 31 ; i++) {
		/*Data to be shifted in MSB -- LSB order*/
		TDI = (data_out & (1L<<31)) != 0;
		PULSE_TCK();

		data_in |= TDO;

		data_in <<= 1;
		data_out <<= 1;
	}
	TDI = (data_out & (1L<<31)) != 0;
	TMS = 1;
	PULSE_TCK();
	data_in |= (TDO != 0);

	/*to UpDate DR*/
	PULSE_TCK();
	/*bring back to desired state*/

	TMS = 0 ;
	PULSE_TCK();
	TMS = 1;
	PULSE_TCK();

	return data_in;

}







/* Reads the given ICEBreaker register and returs a 32 bit value
 * assumption: Called when the state machine is at Run-Test Idle
 * state*/
static reentrant unsigned long
arm_read_icebreakreg(unsigned char reg)
{
	unsigned char i = 0;
	unsigned long data = 0;
	/*Select SCAN chanin 2 for reading ICEBreaker reg*/

	/*Bring the TAP to Capture DR state
	 */
	TMS = 0;
	PULSE_TCK();
	// into shift dr state
	PULSE_TCK();

	for(i = 0; i != 5; i++ )
	{
		TDI = reg & 0x01;
		PULSE_TCK();
		reg >>= 1;

	}
	/*send in the read bit*/
	TDI = 0;
	TMS = 1;
	PULSE_TCK(); /*Clearing of TDI to indicate it is a read operation
		      *and bringing the stae machine to Exit-1 DR are
		      *achived at the same time*/

	PULSE_TCK(); /*bring the state machine to the Update DR state*/

	/*bring back to SCDR*/
	TMS = 1;
	PULSE_TCK();

	/*Now a 32 bit value is ready to be shifter out of scan chain*/

	TMS = 0;
	PULSE_TCK();
	PULSE_TCK();
	/*now we are ready to read the data from TDO*/
	for(i = 0; i != 31; i++)
	{
		PULSE_TCK();
		data |= (unsigned long)TDO <<  i ;


	}
	/*Now exit the DR related state and recover the last bit*/
	TMS = 1;
	PULSE_TCK(); // Exit1 DR
	data |= (unsigned long)TDO <<  i ;
	PULSE_TCK(); // UpDate DR
	TMS = 0;
	PULSE_TCK(); // Run Test Idle
	TMS = 1;
	PULSE_TCK(); // SDR

	return data;

}

/* Reads the given ICEBreaker register and returs a 32 bit value
 * assumption: Called when the state machine is at Run-Test Idle
 * state*/
static reentrant void
arm_write_icebreakreg(unsigned char reg, unsigned long data)
{
	unsigned char i = 0;
	/*Select SCAN chanin 2 for reading ICEBreaker reg*/


	/*Bring the TAP to Capture DR state
	 */
	TMS = 0;
	PULSE_TCK();
	TDI = 0;
	// into shift dr state
	PULSE_TCK();

	for(i = 0; i != 32; i++ )
	{
		TDI = data & 0x01;
		PULSE_TCK();
		data >>=1;
	}

	for(i = 0; i != 5; i++ )
	{
		TDI = reg & 0x01;
		PULSE_TCK();
		reg>>= 1 ;
	}
	/*send in the write bit*/
	TDI = 1;
	TMS = 1;
	PULSE_TCK(); /*Clearing of TDI to indicate it is a write operation
		      *and bringing the stae machine to Exit-1 DR are
		      *achived at the same time*/
	PULSE_TCK();

	TMS = 0;
	PULSE_TCK(); // Run test idle

	TMS = 1;
	PULSE_TCK(); /*bring the state machine to the Update DR state*/


	return;

}
static void
Fix(void)
{
	arm_read_reg(ARM_R0);
	arm_select_scanchain(SCAN_CHAIN_2);
}


/* Loads the given binary content to given memory address
 * Input:-
 * data :pointer to the buffer to be written
 * data_len: length of data in bytes
 * address : address of the memory location where the
 * data has to be written to
 **/

static reentrant unsigned char
arm_write_memory(unsigned char xdata *data, unsigned long data_len, unsigned long address, unsigned char mem)
{
	int i= 0, incr = 0;
	int timeout = ARM_MEMACC_TIMEOUT*2;
	unsigned long R0, R1, write_command = ARM_INSTR_NOP;
	unsigned long datain = 0;
	




	arm_write_cpsr(ARM_FLSH_PROGMODE);


	arm_select_scanchain(SCAN_CHAIN_1);
	R0 = arm_read_reg(ARM_R0);
	R1 = arm_read_reg(ARM_R1);
	arm_write_reg(ARM_R5, (mem == JT_FLASH));
	if(mem == JT_FLASH ){
		write_command = 0xE1C010B0; // STRH
		incr = 2;
	}else{
		write_command = 0xE5001000; // STR
		incr = 4;
	}
		

	for(i = 0; i< data_len; i+=incr){
		arm_write_reg(ARM_R0, address+i);
		datain  =  *(data+i);
		datain |=  ((unsigned long)*(data+i+1) << 8);
		datain |=  ((unsigned long)*(data+i+2) << 16);
		datain |=  ((unsigned long)*(data+i+3) << 24);

		arm_write_reg(ARM_R1, datain);
		arm_execute_instr(ARM_INSTR_NOP,  0);
		arm_execute_instr(ARM_INSTR_NOP,  1);
		arm_execute_instr(write_command,  0);
		arm_execute_instr(ARM_INSTR_NOP,  0);
		arm_shift_ir(IR_COMM_RESTART, 1);


		/*Read the Debug Status Register */
		/*Both DBGACK and nMREQ bits should be set*/
        	Fix();
		while( (arm_read_icebreakreg(ICEBREAK_REG_DEBSTS) & ICEBREAK_REG_DBGACK_MSK)   != ICEBREAK_REG_DBGACK_MSK)
		{
			if(--timeout < 0 )
			{
				arm_write_reg(ARM_R0, R0);
				arm_write_reg(ARM_R1, R1);

				return FALSE;
			}
			//usDelay(10000);
		}  
		arm_read_reg(ARM_R0);


	}
	arm_restore_pc((0x8 *data_len)+1 );
	arm_write_reg(ARM_R1, arm_read_icebreakreg(ICEBREAK_REG_DEBSTS)); 

	//arm_write_reg(ARM_R0, R0);
	//arm_write_reg(ARM_R1, R1);


	return TRUE;

}
#if 0

static reentrant unsigned char
arm_write_memory(unsigned char xdata *data, unsigned long data_len, unsigned long address)
{
	int i= 0;
	int timeout = ARM_MEMACC_TIMEOUT*2;
	unsigned long R0, R1;
	unsigned long datain = 0;




	arm_write_cpsr(ARM_FLSH_PROGMODE);


	arm_select_scanchain(SCAN_CHAIN_1);
	R0 = arm_read_reg(ARM_R0);
	R1 = arm_read_reg(ARM_R1);

	datain  =  *(data+i);
	datain |=  ((unsigned long)*(data+i+1) << 8);
	datain |=  ((unsigned long)*(data+i+2) << 16);
	datain |=  ((unsigned long)*(data+i+3) << 24);

	arm_write_reg(ARM_R14, address);

	arm_execute_instr(0xE89E0001,  0);
	arm_execute_instr(ARM_INSTR_NOP,  0);
	arm_execute_instr(ARM_INSTR_NOP,  0);
	arm_execute_instr(datain,  0);
	arm_execute_instr(ARM_INSTR_NOP,  0);
	arm_execute_instr(ARM_INSTR_NOP,  0);

	arm_write_reg(ARM_R14, address);
	arm_execute_instr(ARM_INSTR_NOP,  0);
	arm_execute_instr(ARM_INSTR_NOP,  1);
	arm_execute_instr(0xE88E0001, 0);
	arm_execute_instr(ARM_INSTR_NOP,  0);
	arm_shift_ir(IR_COMM_RESTART, 1);


	Fix();
	while( (arm_read_icebreakreg(ICEBREAK_REG_DEBSTS) & ICEBREAK_REG_DBGACK_MSK)   != ICEBREAK_REG_DBGACK_MSK)
	{
		if(--timeout < 0 )
		{
			arm_write_reg(ARM_R0, R0);
			arm_write_reg(ARM_R1, R1);
				return FALSE;
		}
	}
	arm_read_reg(ARM_R0);
	
	
	return TRUE;

	

	for(i = 0; i< data_len; i++){

		arm_write_reg(ARM_R1, *(data+i));
		arm_execute_instr(ARM_INSTR_NOP,  0);
		arm_execute_instr(ARM_INSTR_NOP,  1);
		arm_execute_instr(0xE4C01001,  0);//STR [R0], R1 and R0+1->R0
		arm_execute_instr(ARM_INSTR_NOP,  0);
		arm_shift_ir(IR_COMM_RESTART, 1);


		/*Read the Debug Status Register */
		/*Both DBGACK and nMREQ bits should be set*/
        	Fix();
		while( (arm_read_icebreakreg(ICEBREAK_REG_DEBSTS) & ICEBREAK_REG_DBGACK_MSK)   != ICEBREAK_REG_DBGACK_MSK)
		{
			if(--timeout < 0 )
			{
				arm_write_reg(ARM_R0, R0);
				arm_write_reg(ARM_R1, R1);

				return FALSE;
			}
			//usDelay(10000);
		}


	}
	arm_restore_pc((0x8 *data_len)+1 );

	arm_write_reg(ARM_R0, R0);
	arm_write_reg(ARM_R1, R1);


	return TRUE;

}

#endif

static reentrant unsigned long
arm_read_reg(unsigned long reg)
{
	unsigned long val = 0;

	
	arm_select_scanchain(SCAN_CHAIN_1);
	arm_execute_instr(ARM_INSTR_STRR | (reg<<12), 0);

	arm_execute_instr(ARM_INSTR_NOP, 0);
	arm_execute_instr(ARM_INSTR_NOP,  0);
	arm_execute_instr(ARM_INSTR_NOP,  0);

	/*Now recover the 32 value of the register*/
	val = arm_execute_instr(ARM_INSTR_NOP,  0);
	/*if(reg == ARM_PC)
	{
		val = val - 0x24;
	}*/

	//restoring PC
	arm_execute_instr(ARM_INSTR_NOP,  0);
	arm_execute_instr(ARM_INSTR_NOP, 1);
	arm_execute_instr(0xEAFFFFF5, 0);
	arm_shift_ir(IR_COMM_RESTART, 1);	
	usDelay(100);

	arm_select_scanchain(SCAN_CHAIN_2);
	if((arm_read_icebreakreg(ICEBREAK_REG_DEBSTS) & ICEBREAK_REG_DBGACK_MSK)
		!= ICEBREAK_REG_DBGACK_MSK )
	{
		arm_select_scanchain(SCAN_CHAIN_1);
		return 0x0;
	}
	arm_select_scanchain(SCAN_CHAIN_1);
	return val;
	//return g_icebrkregs[0];

}


static reentrant unsigned long
arm_abortstatus(void)
{
	return 0;	
		
	
}

static unsigned char arm_init_debug(void)
{
	arm_select_scanchain(SCAN_CHAIN_2);
	arm_write_icebreakreg(0x0, 0x20);
	if(arm_read_icebreakreg(0x00) & 0x20 != 0x20 )
		return FALSE;

	arm_write_icebreakreg(0x1, 0);
	arm_write_icebreakreg(0x8, 0);
	arm_write_icebreakreg(0x9, 0);
	arm_write_icebreakreg(0xA, 0);
	arm_write_icebreakreg(0xB, 0);
	arm_write_icebreakreg(0xC, 0);
	arm_write_icebreakreg(0xD, 0x0);
	arm_write_icebreakreg(0x10, 0x0);
	arm_write_icebreakreg(0x11, 0);
	arm_write_icebreakreg(0x12, 0x0);
	arm_write_icebreakreg(0x13, 0x0);
	arm_write_icebreakreg(0x14, 0);
	arm_write_icebreakreg(0x15, 0);
	arm_write_icebreakreg(0x16, 0x0);
	arm_write_icebreakreg(0x0, 0x00);

	if(arm_read_icebreakreg(0x00) & 0x20 != 0x00 )
		return FALSE;

	return TRUE;
}

static reentrant unsigned char arm_go(void)
{
	unsigned long pc_val = 0;
	int timeout = 0x400;

	

	arm_select_scanchain(SCAN_CHAIN_2);
	if((arm_read_icebreakreg(ICEBREAK_REG_DEBSTS) & 0x10)
		== 0x10 )
	{
		return FALSE;
	}
	
	/*arm_write_icebreakreg(ICEBREAK_REG_WP0_ADVALUE, 0);
	arm_write_icebreakreg(ICEBREAK_REG_WP0_ADMASK, 0xFFFFFFFF);

	arm_write_icebreakreg(ICEBREAK_REG_WP0_DVALUE, 0);
	arm_write_icebreakreg(ICEBREAK_REG_WP0_DMASK, 0xFFFFFFFF);

	arm_write_icebreakreg(ICEBREAK_REG_WP0_CTRLVALUE, 0x00000100);
	arm_write_icebreakreg(ICEBREAK_REG_WP0_CTRLMASK, 0xFFFFFFF7);*/


	arm_select_scanchain(SCAN_CHAIN_1);
	arm_execute_instr(ARM_INSTR_NOP,  1);
	arm_execute_instr(0xEAFFFFFA,  0);
	//arm_restore_dbgsts();
	//arm_select_scanchain(SCAN_CHAIN_1);	
	arm_shift_ir(IR_COMM_RESTART, 1);
	usDelay(9000);

			
	//arm_select_scanchain(SCAN_CHAIN_2);
	/*while((arm_read_icebreakreg(ICEBREAK_REG_DEBSTS) & 0x1)
		== 0x01 )
	{
		usDelay(100);
		if(timeout-- < 0)
			return FALSE;
	}*/

	return TRUE; 

}

 unsigned char arm_restore_pc(long numdecrement)
{
	unsigned long pcvalue = 0;
	arm_select_scanchain(SCAN_CHAIN_1);
	pcvalue -= numdecrement;
	arm_execute_instr(ARM_INSTR_NOP, 1);
	arm_execute_instr(0xEAFFFFFF & pcvalue, 0);
	arm_shift_ir(IR_COMM_RESTART, 1);	
	usDelay(100);
	arm_select_scanchain(SCAN_CHAIN_2);
	if((arm_read_icebreakreg(ICEBREAK_REG_DEBSTS) & ICEBREAK_REG_DBGACK_MSK)
		!= ICEBREAK_REG_DBGACK_MSK )
	{
		return FALSE;
	}
	return TRUE;	


}


static reentrant BOOL
arm_write_reg(unsigned long reg, unsigned long value)
{
	
	
	arm_select_scanchain(SCAN_CHAIN_1);

	arm_execute_instr(ARM_INSTR_LDRR |(reg<<12),  0);
	arm_execute_instr(ARM_INSTR_NOP,  0);
	arm_execute_instr(ARM_INSTR_NOP,  0);
	arm_execute_instr(value,  0);

	arm_execute_instr(ARM_INSTR_NOP,  0);
	if(reg == ARM_PC)
	{
		arm_execute_instr(ARM_INSTR_NOP,  0);
		arm_execute_instr(ARM_INSTR_NOP,  0);

		
		arm_restore_pc(0xB);
		
		
	}else{

		
		arm_restore_pc(5);

	}
	arm_select_scanchain(SCAN_CHAIN_1);	
	arm_read_reg(ARM_R0);
	return TRUE;



}


static reentrant unsigned char arm_isstopped(void)
{
	arm_select_scanchain(SCAN_CHAIN_2);
	if((arm_read_icebreakreg(ICEBREAK_REG_DEBSTS) & 0x01 )
			!= 0x01)
	{
		return 0;
	}
	return 1;
}


	
static reentrant unsigned char
arm_stop_core(void)
{
	int timeout = 0x400;

	arm_select_scanchain(SCAN_CHAIN_2);

	arm_write_icebreakreg(ICEBREAK_REG_DEBCTL, 0x20);
	arm_write_icebreakreg(ICEBREAK_REG_WP0_ADVALUE, 0);
	arm_write_icebreakreg(ICEBREAK_REG_WP0_ADMASK, 0xFFFFFFFF);

	arm_write_icebreakreg(ICEBREAK_REG_WP0_DVALUE, 0);
	arm_write_icebreakreg(ICEBREAK_REG_WP0_DMASK, 0xFFFFFFFF);

	arm_write_icebreakreg(ICEBREAK_REG_WP0_CTRLVALUE, 0x00000100);
	arm_write_icebreakreg(ICEBREAK_REG_WP0_CTRLMASK, 0xFFFFFFF7);
	arm_write_icebreakreg(ICEBREAK_REG_DEBCTL, 0x00);

	/*arm_write_icebreakreg(ICEBREAK_REG_DEBCTL, 2);
	usDelay(5000);*/
	
	/*Read the Debug Status Register */
	/*Both DBGACK and nMREQ bits should be set*/

	while((arm_read_icebreakreg(ICEBREAK_REG_DEBSTS) & 0x01)
			!= 0x01 )
	{
		if(timeout--){
			usDelay(1000);
			return FALSE;
		}
	}
	
	arm_shift_ir(IR_COMM_RESTART, 1);
	//arm_reset();
	usDelay(5000);
	/*This read is required for most of the operations to work correctly. Dont remove!! */
	arm_read_reg(ARM_R0);
	return TRUE;


}

#define ARMV4_5_T_BXPC		(0x4778 | (0x4700  << 16))

static reentrant  unsigned char
arm_write_cpsr(unsigned long mask)
{
	unsigned long R0 = 0;
	arm_select_scanchain(SCAN_CHAIN_1);

	arm_execute_instr(ARMV4_5_T_BXPC, 0);
	arm_execute_instr(ARM_INSTR_NOP, 0);
	arm_execute_instr(ARM_INSTR_NOP, 0);
	arm_execute_instr(0, 0);
	arm_execute_instr(ARM_INSTR_NOP, 0);
	return TRUE;
}


static reentrant unsigned long
arm_read_cpsr(void)
{
	unsigned long cpsrval = 0;
	unsigned long R0 = arm_read_reg(ARM_R0);
	arm_select_scanchain(SCAN_CHAIN_1);

	arm_execute_instr(ARM_INSTR_READCPSR, 0);
	arm_execute_instr(ARM_INSTR_NOP, 0);
	arm_execute_instr(ARM_INSTR_NOP, 0);
	cpsrval = arm_read_reg(ARM_R0);
	arm_write_reg(ARM_R0, R0);
	return cpsrval;
}





/* Reads the ven number of bytes from given address
 * Input:-
 * data :pointer to the buffer to be read
 * data_len: length of data
 * address : address of the memory location where the
 * data has to be written to
 **/
static reentrant unsigned char
arm_read_memory (unsigned char xdata *data, unsigned long data_len, unsigned long address)
{
	unsigned long R0, R1;
	unsigned long value = 0;
	int i = 0, timeout = ARM_MEMACC_TIMEOUT;
	/*Select the SCAN_1*/

	if(arm_stop_core() != TRUE )
	{

		return FALSE;

	}
	arm_write_cpsr(ARM_FLSH_PROGMODE);
	R0 = arm_read_reg(ARM_R0);
	R1 = arm_read_reg(ARM_R1);


	for(i = 0; i< data_len; i+=4){
		arm_write_reg(ARM_R0, address+i);
		arm_execute_instr(ARM_INSTR_NOP,  0);

		arm_execute_instr(ARM_INSTR_NOP, 0x01); // next instruction in system time
		arm_execute_instr(ARM_INSTR_LDRR1R0,  0);
		arm_execute_instr(ARM_INSTR_NOP,  0);


		arm_shift_ir(IR_COMM_RESTART, 1);
		usDelay(20000);
		/*Read the Debug Status Register */
		/*Both DBGACK and nMREQ bits should be set*/

		Fix();
		while( (arm_read_icebreakreg(ICEBREAK_REG_DEBSTS) & ICEBREAK_REG_DBGACK_MSK)   != ICEBREAK_REG_DBGACK_MSK)
		{
			if(--timeout < 0 )
				return FALSE;
			usDelay(100);
		}
		
		arm_read_reg(ARM_R0);

		R1 = arm_read_reg(ARM_R1);
		data[i] = R1 & 0xFF;
		data[i+1] = (R1>>8) & 0xFF;
		data[i+2] = (R1>>16) & 0xFF;
		data[i+3] = (R1>>24) & 0xFF;

	}

	/*Restore the value of R0 and R1*/
	arm_write_reg(ARM_R0, R0);
	arm_write_reg(ARM_R1, R1);

	return TRUE;

}




// connect the target device, get device ID



static reentrant BOOL
arm_connect(void)
{
	unsigned char i;



	arm_reset();		// reset chip
	usDelay(2);
	arm_shift_ir(0x0E, 0);
	dev_id = arm_read_dr();
	arm_init_debug();
	g_no_breakpoints = 0;
	return TRUE;
}
// disconnect the target - just reset it

static reentrant BOOL
arm_disconnect(void)
{
	arm_reset();		// reset chip
	return TRUE;
}


// erase the entire device memory

static reentrant BOOL
arm_device_erase(void)
{
	return TRUE;
}


const dev_int 	arm7_interface =
{
	JT_ARM7,
	arm_connect,
	arm_device_erase,
	arm_disconnect,
	arm_write_memory,
	arm_read_memory,
	arm_read_reg,
	arm_write_reg,
	arm_read_cpsr,
	arm_write_cpsr,	
	arm_stop_core,
	/*arm_set_break,
	arm_clear_break,*/
	arm_write_icebreakreg,
	arm_read_icebreakreg,
	arm_go,
	arm_isstopped	
};




