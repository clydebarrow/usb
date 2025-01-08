#ifndef _CY8C24X94_H
#define _CY8C24X94_H

//=============================================================================
//=============================================================================
//      Register Space, Bank 0
//=============================================================================
//=============================================================================

//-----------------------------------------------
//  Port Registers
//  Note: Also see this address range in Bank 1.
//-----------------------------------------------
// Port 0
volatile __ioport unsigned char	PRT0DR @	0x000;	// Port 0 Data Register
volatile __ioport unsigned char	PRT0IE @	0x001;	// Port 0 Interrupt Enable Register
volatile __ioport unsigned char	PRT0GS @	0x002;	// Port 0 Global Select Register
volatile __ioport unsigned char	PRT0DM2 @	0x003;	// Port 0 Drive Mode 2
// Port 1
volatile __ioport unsigned char	PRT1DR @	0x004;	// Port 1 Data Register
volatile __ioport unsigned char	PRT1IE @	0x005;	// Port 1 Interrupt Enable Register
volatile __ioport unsigned char	PRT1GS @	0x006;	// Port 1 Global Select Register
volatile __ioport unsigned char	PRT1DM2 @	0x007;	// Port 1 Drive Mode 2
// Port 2
volatile __ioport unsigned char	PRT2DR @	0x008;	// Port 2 Data Register
volatile __ioport unsigned char	PRT2IE @	0x009;	// Port 2 Interrupt Enable Register
volatile __ioport unsigned char	PRT2GS @	0x00A;	// Port 2 Global Select Register
volatile __ioport unsigned char	PRT2DM2 @	0x00B;	// Port 2 Drive Mode 2
// Port 3
volatile __ioport unsigned char	PRT3DR @	0x00C;	// Port 3 Data Register
volatile __ioport unsigned char	PRT3IE @	0x00D;	// Port 3 Interrupt Enable Register
volatile __ioport unsigned char	PRT3GS @	0x00E;	// Port 3 Global Select Register
volatile __ioport unsigned char	PRT3DM2 @	0x00F;	// Port 3 Drive Mode 2
// Port 4
volatile __ioport unsigned char	PRT4DR @	0x010;	// Port 4 Data Register
volatile __ioport unsigned char	PRT4IE @	0x011;	// Port 4 Interrupt Enable Register
volatile __ioport unsigned char	PRT4GS @	0x012;	// Port 4 Global Select Register
volatile __ioport unsigned char	PRT4DM2 @	0x013;	// Port 4 Drive Mode 2
// Port 5
volatile __ioport unsigned char	PRT5DR @	0x014;	// Port 5 Data Register
volatile __ioport unsigned char	PRT5IE @	0x015;	// Port 5 Interrupt Enable Register
volatile __ioport unsigned char	PRT5GS @	0x016;	// Port 5 Global Select Register
volatile __ioport unsigned char	PRT5DM2 @	0x017;	// Port 5 Drive Mode 2
// Port 7
volatile __ioport unsigned char	PRT7DR @	0x01C;	// Port 7 Data Register
volatile __ioport unsigned char	PRT7IE @	0x01D;	// Port 7 Interrupt Enable Register
volatile __ioport unsigned char	PRT7GS @	0x01E;	// Port 7 Global Select Register
volatile __ioport unsigned char	PRT7DM2 @	0x01F;	// Port 7 Drive Mode 2

//-----------------------------------------------
//  Digital PSoC(tm) block Registers
//  Note: Also see this address range in Bank 1.
//-----------------------------------------------

// Digital PSoC block 00, Basic Type B
volatile __ioport unsigned char	DBB00DR0 @	0x020;	// data register 0
volatile __ioport unsigned char	DBB00DR1 @	0x021;	// data register 1
volatile __ioport unsigned char	DBB00DR2 @	0x022;	// data register 2
volatile __ioport unsigned char	DBB00CR0 @	0x023;	// control & status register 0

// Digital PSoC block 01, Basic Type B
volatile __ioport unsigned char	DBB01DR0 @	0x024;	// data register 0
volatile __ioport unsigned char	DBB01DR1 @	0x025;	// data register 1
volatile __ioport unsigned char	DBB01DR2 @	0x026;	// data register 2
volatile __ioport unsigned char	DBB01CR0 @	0x027;	// control & status register 0

// Digital PSoC block 02, Communication Type B
volatile __ioport unsigned char	DCB02DR0 @	0x028;	// data register 0
volatile __ioport unsigned char	DCB02DR1 @	0x029;	// data register 1
volatile __ioport unsigned char	DCB02DR2 @	0x02A;	// data register 2
volatile __ioport unsigned char	DCB02CR0 @	0x02B;	// control & status register 0

// Digital PSoC block 03, Communication Type B
volatile __ioport unsigned char	DCB03DR0 @	0x02C;	// data register 0
volatile __ioport unsigned char	DCB03DR1 @	0x02D;	// data register 1
volatile __ioport unsigned char	DCB03DR2 @	0x02E;	// data register 2
volatile __ioport unsigned char	DCB03CR0 @	0x02F;	// control & status register 0

//-----------------------------------------------
// PMA Data Registers
//-----------------------------------------------

volatile __ioport unsigned char	PMA0_DR @	0x040;	// PMA 0 data register
volatile __ioport unsigned char	PMA1_DR @	0x041;	// PMA 1 data register
volatile __ioport unsigned char	PMA2_DR @	0x042;	// PMA 2 data register
volatile __ioport unsigned char	PMA3_DR @	0x043;	// PMA 3 data register
volatile __ioport unsigned char	PMA4_DR @	0x044;	// PMA 4 data register
volatile __ioport unsigned char	PMA5_DR @	0x045;	// PMA 5 data register
volatile __ioport unsigned char	PMA6_DR @	0x046;	// PMA 6 data register
volatile __ioport unsigned char	PMA7_DR @	0x047;	// PMA 7 data register

//-----------------------------------------------
//  USB Control Registers
//-----------------------------------------------
volatile __ioport unsigned char	USB_SOF0 @	0x048;	// USB SOF LSB Byte
volatile __ioport unsigned char	USB_SOF1 @	0x049;	// USB SOF MSB Byte

volatile __ioport unsigned char	USB_CR0 @	0x04A;	// USB Control Register 0
#define USB_CR0_ENABLE      (0x80)
#define USB_CR0_DEVICE_ADDR (0x7F)

volatile __ioport unsigned char	USBIO_CR0 @	0x04B;	// USBIO Control Register 0
#define USBIO_CR0_TEN       (0x80)
#define USBIO_CR0_TSE0      (0x40)
#define USBIO_CR0_TD        (0x20)
#define USBIO_CR0_RD        (0x01)

volatile __ioport unsigned char	USBIO_CR1 @	0x04C;	// USBIO Control Register 1
#define USBIO_CR1_IOMODE    (0x80)
#define USBIO_CR1_DRV_MODE  (0x40)
#define USBIO_CR1_DPI       (0x20)
#define USBIO_CR1_DMI       (0x10)
#define USBIO_CR1_PS2PUEN   (0x08)
#define USBIO_CR1_USBPUEN   (0x04)
#define USBIO_CR1_DPO       (0x02)
#define USBIO_CR1_DMO       (0x01)

//-----------------------------------------------
//  Endpoint Registers
//-----------------------------------------------
volatile __ioport unsigned char	EP1_CNT1 @	0x04E;	// EP1 Count Register 1
#define EP1_CNT1_DATA_TOGGLE (0x80)
#define EP1_CNT1_DATA_VALID  (0x40)
#define EP1_CNT1_CNT_MSB     (0x01)

volatile __ioport unsigned char	EP1_CNT @	0x04F;	// EP1 Count Register 0

volatile __ioport unsigned char	EP2_CNT1 @	0x050;	// EP2 Count Register 1
#define EP2_CNT1_DATA_TOGGLE (0x80)
#define EP2_CNT1_DATA_VALID  (0x40)
#define EP2_CNT1_CNT_MSB     (0x01)

volatile __ioport unsigned char	EP2_CNT @	0x051;	// EP2 Count Register 0

volatile __ioport unsigned char	EP3_CNT1 @	0x052;	// EP3 Count Register 1
#define EP3_CNT1_DATA_TOGGLE (0x80)
#define EP3_CNT1_DATA_VALID  (0x40)
#define EP3_CNT1_CNT_MSB     (0x01)

volatile __ioport unsigned char	EP3_CNT @	0x053;	// EP3 Count Register 0

volatile __ioport unsigned char	EP4_CNT1 @	0x054;	// EP4 Count Register 1
#define EP4_CNT1_DATA_TOGGLE (0x80)
#define EP4_CNT1_DATA_VALID  (0x40)
#define EP4_CNT1_CNT_MSB     (0x01)

volatile __ioport unsigned char	EP4_CNT @	0x055;	// EP4 Control Register

volatile __ioport unsigned char	EP0_CR @	0x056;	// EP0 Control Register

volatile __ioport unsigned char	EP1_CR @	0x0C4;	// EP1 Control Register

#define EP0_CR_SETUP_RCVD    (0x80)
#define EP0_CR_IN_RCVD       (0x40)
#define EP0_CR_OUT_RCVD      (0x20)
#define EP0_CR_ACKD          (0x10)
#define EP0_CR_MODE          (0x0F)

// USB endpoint modes. Only defining the ones we're interested in
//
#define	USBMODE_CTRL_IDLE		0x1		// accept control transfers, NAK others
#define	USBMODE_CTRL_ACK		0xB		// control transfer successful
#define	USBMODE_CTRL_DONE		0x2		// control transfer complete
#define	USBMODE_CTRL_STALL		0x3		// stall in/out
#define	USBMODE_CTRL_IN			0xF		// accept control transfers, ack IN, accept 0 for out
#define	USBMODE_CTRL_IN0		0x6		// accept control transfers, reply to in with 0 bytes
#define	USBMODE_ACKIN			0xD		// data is ready to send to IN request
#define	USBMODE_NAKIN			0xC		// in fifo empty
#define	USBMODE_ACKOUT			0x9		// Ready to receive data
#define	USBMODE_NAKOUT			0x8		// out fifo has data, nak further outs
#define USBMODE_ISOOUT			0x5
#define USBMODE_ISOIN			0x7

volatile __ioport unsigned char	EP0_CNT @	0x057;	// EP0 Count Register
#define EP0_CNT_DATA_TOGGLE (0x80)
#define EP0_CNT_DATA_VALID  (0x40)
#define EP0_CNT_BYTE_CNT    (0x0F)

volatile __ioport unsigned char	EP0_DR[8] @	0x058;	// EP0 Data Register array
volatile __ioport unsigned char	EP0_DR0 @	0x058;	// EP0 Data Register 0
volatile __ioport unsigned char	EP0_DR1 @	0x059;	// EP0 Data Register 0
volatile __ioport unsigned char	EP0_DR2 @	0x05A;	// EP0 Data Register 0
volatile __ioport unsigned char	EP0_DR3 @	0x05B;	// EP0 Data Register 0
volatile __ioport unsigned char	EP0_DR4 @	0x05C;	// EP0 Data Register 0
volatile __ioport unsigned char	EP0_DR5 @	0x05D;	// EP0 Data Register 0
volatile __ioport unsigned char	EP0_DR6 @	0x05E;	// EP0 Data Register 0
volatile __ioport unsigned char	EP0_DR7 @	0x05F;	// EP0 Data Register 0

//-----------------------------------------------
//  Analog Control Registers
//-----------------------------------------------

volatile __ioport unsigned char	AMX_IN @	0x060;	// Analog Input Multiplexor Control
#define AMX_IN_ACI1       (0x0C)
#define AMX_IN_ACI0       (0x03)

volatile __ioport unsigned char	AMUXCFG @	0x061;	// Analog MUX Configuration
#define AMUXCFG_BCOL1MUX  (0x80)
#define AMUXCFG_ACOLOMUX  (0x40)
#define AMUXCFG_INTCAP    (0x30)
#define AMUXCFG_MUXCLK    (0x0E)
#define AMUXCFG_EN        (0x01)

volatile __ioport unsigned char	ARF_CR @	0x063;	// Analog Reference Control Register
#define ARF_CR_HBE      (0x40)
#define ARF_CR_REF      (0x38)
#define ARF_CR_REFPWR   (0x07)
#define ARF_CR_SCPWR    (0x03)

volatile __ioport unsigned char	CMP_CR0 @	0x064;	// Analog Comparator Bus Register 0
#define CMP_CR0_COMP1    (0x20)
#define CMP_CR0_COMP0    (0x10)
#define CMP_CR0_AINT1    (0x02)
#define CMP_CR0_AINT0    (0x01)

volatile __ioport unsigned char	ASY_CR @	0x065;	// Analog Synchronizaton Control Register
#define ASY_CR_SARCOUNT (0x70)
#define ASY_CR_SARSIGN  (0x08)
#define ASY_CR_SARCOL   (0x06)
#define ASY_CR_SYNCEN   (0x01)

volatile __ioport unsigned char	CMP_CR1 @	0x066;	// Analog Comparator Bus Register 1
#define CMP_CR1_CLDIS1  (0x20)
#define CMP_CR1_CLDIS0  (0x10)
#define CMP_CR1_CLK1X1  (0x02)
#define CMP_CR1_CLK1X0  (0x01)


//-----------------------------------------------
//  Analog PSoC block Registers
//
//  Note: the following registers are mapped into
//  both register bank 0 AND register bank 1.
//-----------------------------------------------

// Continuous Time PSoC Block Type B Row 0 Col 0
volatile __ioport unsigned char	ACB00CR3 @	0x70;	// Control register 3
volatile __ioport unsigned char	ACB00CR0 @	0x71;	// Control register 0
volatile __ioport unsigned char	ACB00CR1 @	0x72;	// Control register 1
volatile __ioport unsigned char	ACB00CR2 @	0x73;	// Control register 2

// Continuous Time PSoC Block Type B Row 0 Col 1
volatile __ioport unsigned char	ACB01CR3 @	0x74;	// Control register 3
volatile __ioport unsigned char	ACB01CR0 @	0x75;	// Control register 0
volatile __ioport unsigned char	ACB01CR1 @	0x76;	// Control register 1
volatile __ioport unsigned char	ACB01CR2 @	0x77;	// Control register 2

// Switched Cap PSoC Block Type C Row 1 Col 0
volatile __ioport unsigned char	ASC10CR0 @	0x80;	// Control register 0
volatile __ioport unsigned char	ASC10CR1 @	0x81;	// Control register 1
volatile __ioport unsigned char	ASC10CR2 @	0x82;	// Control register 2
volatile __ioport unsigned char	ASC10CR3 @	0x83;	// Control register 3

// Switched Cap PSoC Block Type D Row 1 Col 1
volatile __ioport unsigned char	ASD11CR0 @	0x84;	// Control register 0
volatile __ioport unsigned char	ASD11CR1 @	0x85;	// Control register 1
volatile __ioport unsigned char	ASD11CR2 @	0x86;	// Control register 2
volatile __ioport unsigned char	ASD11CR3 @	0x87;	// Control register 3

// Switched Cap PSoC Block Type D Row 2 Col 0
volatile __ioport unsigned char	ASD20CR0 @	0x90;	// Control register 0
volatile __ioport unsigned char	ASD20CR1 @	0x91;	// Control register 1
volatile __ioport unsigned char	ASD20CR2 @	0x92;	// Control register 2
volatile __ioport unsigned char	ASD20CR3 @	0x93;	// Control register 3

// Switched Cap PSoC Block Type C Row 2 Col 1
volatile __ioport unsigned char	ASC21CR0 @	0x94;	// Control register 0
volatile __ioport unsigned char	ASC21CR1 @	0x95;	// Control register 1
volatile __ioport unsigned char	ASC21CR2 @	0x96;	// Control register 2
volatile __ioport unsigned char	ASC21CR3 @	0x97;	// Control register 3

//-----------------------------------------------
//  Global General Purpose Data Registers
//-----------------------------------------------
volatile __ioport unsigned char	TMP_DR0 @	0x6C;	// Temp Data Register 0
volatile __ioport unsigned char	TMP_DR1 @	0x6D;	// Temp Data Register 1
volatile __ioport unsigned char	TMP_DR2 @	0x6E;	// Temp Data Register 2
volatile __ioport unsigned char	TMP_DR3 @	0x6F;	// Temp Data Register 3

//-----------------------------------------------
//  Row Digital Interconnects
//
//  Note: the following registers are mapped into
//  both register bank 0 AND register bank 1.
//-----------------------------------------------

volatile __ioport unsigned char	RDI0RI @	0xB0;	// Row Digital Interconnect Row 0 Input
volatile __ioport unsigned char	RDI0SYN @	0xB1;	// Row Digital Interconnect Row 0 Sync Reg
volatile __ioport unsigned char	RDI0IS @	0xB2;	// Row 0 Input Select Register
volatile __ioport unsigned char	RDI0LT0 @	0xB3;	// Row 0 Look Up Table Register 0
volatile __ioport unsigned char	RDI0LT1 @	0xB4;	// Row 0 Look Up Table Register 1
volatile __ioport unsigned char	RDI0RO0 @	0xB5;	// Row 0 Output Register 0
volatile __ioport unsigned char	RDI0RO1 @	0xB6;	// Row 0 Output Register 1

//-----------------------------------------------
//  Ram Page Pointers
//-----------------------------------------------
volatile __ioport unsigned char	CUR_PP @	0x0D0;	// Current   Page Pointer
volatile __ioport unsigned char	STK_PP @	0x0D1;	// Stack     Page Pointer
volatile __ioport unsigned char	IDX_PP @	0x0D3;	// Index     Page Pointer
volatile __ioport unsigned char	MVR_PP @	0x0D4;	// MVI Read  Page Pointer
volatile __ioport unsigned char	MVW_PP @	0x0D5;	// MVI Write Page Pointer

//-----------------------------------------------
//  I2C Configuration Registers
//-----------------------------------------------
volatile __ioport unsigned char	I2C_CFG @	0x0D6;	// I2C Configuration Register
#define I2C_CFG_PINSEL         (0x40)
#define I2C_CFG_BUSERR_IE      (0x20)
#define I2C_CFG_STOP_IE        (0x10)
#define I2C_CFG_CLK_RATE_100K  (0x00)
#define I2C_CFG_CLK_RATE_400K  (0x04)
#define I2C_CFG_CLK_RATE_50K   (0x08)
#define I2C_CFG_CLK_RATE_1M6   (0x0C)
#define I2C_CFG_CLK_RATE       (0x0C)
#define I2C_CFG_PSELECT_MASTER (0x02)
#define I2C_CFG_PSELECT_SLAVE  (0x01)

volatile __ioport unsigned char	I2C_SCR @	0x0D7;	// I2C Status and Control Register
#define I2C_SCR_BUSERR       (0x80)
#define I2C_SCR_LOSTARB      (0x40)
#define I2C_SCR_STOP         (0x20)
#define I2C_SCR_ACK          (0x10)
#define I2C_SCR_ADDR         (0x08)
#define I2C_SCR_XMIT         (0x04)
#define I2C_SCR_LRB          (0x02)
#define I2C_SCR_BYTECOMPLETE (0x01)

volatile __ioport unsigned char	I2C_DR @	0x0D8;	// I2C Data Register

volatile __ioport unsigned char	I2C_MSCR @	0x0D9;	// I2C Master Status and Control Register
#define I2C_MSCR_BUSY    (0x08)
#define I2C_MSCR_MODE    (0x04)
#define I2C_MSCR_RESTART (0x02)
#define I2C_MSCR_START   (0x01)

//-----------------------------------------------
//  System and Global Resource Registers
//-----------------------------------------------
volatile __ioport unsigned char	INT_CLR0 @	0x0DA;	// Interrupt Clear Register 0
volatile __ioport unsigned char	INT_CLR1 @	0x0DB;	// Interrupt Clear Register 1
volatile __ioport unsigned char	INT_CLR2 @	0x0DC;	// Interrupt Clear Register 2
volatile __ioport unsigned char	INT_CLR3 @	0x0DD;	// Interrupt Clear Register 3

volatile __ioport unsigned char	INT_MSK3 @	0x0DE;	// I2C and Software Mask Register
#define INT_MSK3_ENSWINT          (0x80)
#define INT_MSK3_I2C              (0x01)

volatile __ioport unsigned char	INT_MSK2 @	0x0DF;	// General Interrupt Mask Register
#define  INT_MSK2_WAKEUP          (0x80)
#define  INT_MSK2_EP4             (0x40)
#define  INT_MSK2_EP3             (0x20)
#define  INT_MSK2_EP2             (0x10)
#define  INT_MSK2_EP1             (0x08)
#define  INT_MSK2_EP0             (0x04)
#define  INT_MSK2_SOF             (0x02)
#define  INT_MSK2_BUS_RESET       (0x01)

volatile __ioport unsigned char	INT_MSK0 @	0x0E0;	// General Interrupt Mask Register
#define  INT_MSK0_VC3             (0x80)
#define  INT_MSK0_SLEEP           (0x40)
#define  INT_MSK0_GPIO            (0x20)
#define  INT_MSK0_ACOLUMN_1       (0x04)
#define  INT_MSK0_ACOLUMN_0       (0x02)
#define  INT_MSK0_VOLTAGE_MONITOR (0x01)


volatile __ioport unsigned char	INT_MSK1 @	0x0E1;	// Digital PSoC block Mask Register
#define  INT_MSK1_DCB03           (0x08)
#define  INT_MSK1_DCB02           (0x04)
#define  INT_MSK1_DBB01           (0x02)
#define  INT_MSK1_DBB00           (0x01)

volatile __ioport unsigned char	INT_VC @	0x0E2;	// Interrupt vector register

volatile __ioport unsigned char	RES_WDT @	0x0E3;	// Watch Dog Timer

// DECIMATOR Registers
volatile __ioport unsigned char	DEC_DH @	0x0E4;	// Data Register (high byte)
volatile __ioport unsigned char	DEC_DL @	0x0E5;	// Data Register ( low byte)
volatile __ioport unsigned char	DEC_CR0 @	0x0E6;	// Data Control Register
volatile __ioport unsigned char	DEC_CR1 @	0x0E7;	// Data Control Register

// Multiplier and MAC (Multiply/Accumulate) Unit
volatile __ioport unsigned char	MUL0_X @	0x0E8;	// Multiplier X Register (write)
volatile __ioport unsigned char	MUL0_Y @	0x0E9;	// Multiplier Y Register (write)
volatile __ioport unsigned char	MUL0_DH @	0x0EA;	// Multiplier Result Data (high byte read)
volatile __ioport unsigned char	MUL0_DL @	0x0EB;	// Multiplier Result Data ( low byte read)
volatile __ioport unsigned char	ACC0_DR1 @	0x0EC;	// ACC0_DR1 (write) [see ACC_DR1]
volatile __ioport unsigned char	ACC0_DR0 @	0x0ED;	// ACC0_DR0 (write) [see ACC_DR1]
volatile __ioport unsigned char	ACC0_DR3 @	0x0EE;	// ACC0_DR3 (write) [see ACC_DR1]
volatile __ioport unsigned char	ACC0_DR2 @	0x0EF;	// ACC0_DR2 (write) [see ACC_DR1]
volatile __ioport unsigned char	MAC_Y @	0x0ED;	// MAC Y register (write) [see ACC_DR0]
volatile __ioport unsigned char	MAC_CL0 @	0x0EE;	// MAC Clear Accum (write)[see ACC_DR3]
volatile __ioport unsigned char	MAC_CL1 @	0x0EF;	// MAC Clear Accum (write)[see ACC_DR2]
volatile __ioport unsigned char	ACC_DR1 @	0x0EC;	// MAC Accumulator (Read, byte 1)
volatile __ioport unsigned char	ACC_DR0 @	0x0ED;	// MAC Accumulator (Read, byte 0)
volatile unsigned int	ACC_LOW_WORD @ 0xEC;	// MAC accumulator low word
volatile unsigned int	ACC_HI_WORD @ 0xEE;	// MAC accumulator high word
volatile __ioport unsigned char	ACC_DR3 @	0x0EE;	// MAC Accumulator (Read, byte 3)
volatile __ioport unsigned char	ACC_DR2 @	0x0EF;	// MAC Accumulator (Read, byte 2)

//-----------------------------------------------
//  System Status and Control Register
//
//  Note: the following register is mapped into
//  both register bank 0 AND register bank 1.
//-----------------------------------------------
volatile __ioport unsigned char	CPU_F @	0xF7;	// CPU Flag Register Access

volatile __ioport unsigned char	DAC_D @	0xFD;	// DAC Data Register

volatile __ioport unsigned char	CPU_SCR1 @	0xFE;	// System Status and Control Register 1
#define  CPU_SCR1_IRESS        (0x80)
#define  CPU_SCR1_SLIMO        (0x10)
#define  CPU_SCR1_ECO_ALWD_WR  (0x08)
#define  CPU_SCR1_ECO_ALLOWED  (0x04)
#define  CPU_SCR1_IRAMDIS      (0x01)

volatile __ioport unsigned char	CPU_SCR0 @	0xFF;	// System Status and Control Register 0
#define  CPU_SCR0_GIE_MASK     (0x80)
#define  CPU_SCR0_WDRS_MASK    (0x20)
#define  CPU_SCR0_PORS_MASK    (0x10)
#define  CPU_SCR0_SLEEP_MASK   (0x08)
#define  CPU_SCR0_STOP_MASK    (0x01)


//=============================================================================
//=============================================================================
//      Register Space, Bank 1
//=============================================================================
//=============================================================================


//-----------------------------------------------
//  Port Registers
//  Note: Also see this address range in Bank 0.
//-----------------------------------------------
// Port 0
volatile __ioport unsigned char	PRT0DM0 @	0x100;	// Port 0 Drive Mode 0
volatile __ioport unsigned char	PRT0DM1 @	0x101;	// Port 0 Drive Mode 1
volatile __ioport unsigned char	PRT0IC0 @	0x102;	// Port 0 Interrupt Control 0
volatile __ioport unsigned char	PRT0IC1 @	0x103;	// Port 0 Interrupt Control 1
// Port 1
volatile __ioport unsigned char	PRT1DM0 @	0x104;	// Port 1 Drive Mode 0
volatile __ioport unsigned char	PRT1DM1 @	0x105;	// Port 1 Drive Mode 1
volatile __ioport unsigned char	PRT1IC0 @	0x106;	// Port 1 Interrupt Control 0
volatile __ioport unsigned char	PRT1IC1 @	0x107;	// Port 1 Interrupt Control 1
// Port 2
volatile __ioport unsigned char	PRT2DM0 @	0x108;	// Port 2 Drive Mode 0
volatile __ioport unsigned char	PRT2DM1 @	0x109;	// Port 2 Drive Mode 1
volatile __ioport unsigned char	PRT2IC0 @	0x10A;	// Port 2 Interrupt Control 0
volatile __ioport unsigned char	PRT2IC1 @	0x10B;	// Port 2 Interrupt Control 1
// Port 3
volatile __ioport unsigned char	PRT3DM0 @	0x10C;	// Port 3 Drive Mode 0
volatile __ioport unsigned char	PRT3DM1 @	0x10D;	// Port 3 Drive Mode 1
volatile __ioport unsigned char	PRT3IC0 @	0x10E;	// Port 3 Interrupt Control 0
volatile __ioport unsigned char	PRT3IC1 @	0x10F;	// Port 3 Interrupt Control 1
// Port 4
volatile __ioport unsigned char	PRT4DM0 @	0x110;	// Port 4 Drive Mode 0
volatile __ioport unsigned char	PRT4DM1 @	0x111;	// Port 4 Drive Mode 1
volatile __ioport unsigned char	PRT4IC0 @	0x112;	// Port 4 Interrupt Control 0
volatile __ioport unsigned char	PRT4IC1 @	0x113;	// Port 4 Interrupt Control 1
// Port 5
volatile __ioport unsigned char	PRT5DM0 @	0x114;	// Port 5 Drive Mode 0
volatile __ioport unsigned char	PRT5DM1 @	0x115;	// Port 5 Drive Mode 1
volatile __ioport unsigned char	PRT5IC0 @	0x116;	// Port 5 Interrupt Control 0
volatile __ioport unsigned char	PRT5IC1 @	0x117;	// Port 5 Interrupt Control 1
// Port 7
volatile __ioport unsigned char	PRT7DM0 @	0x11C;	// Port 7 Drive Mode 0
volatile __ioport unsigned char	PRT7DM1 @	0x11D;	// Port 7 Drive Mode 1
volatile __ioport unsigned char	PRT7IC0 @	0x11E;	// Port 7 Interrupt Control 0
volatile __ioport unsigned char	PRT7IC1 @	0x11F;	// Port 7 Interrupt Control 1

//-----------------------------------------------
//  Digital PSoC(tm) block Registers
//  Note: Also see this address range in Bank 1.
//-----------------------------------------------

// Digital PSoC block 00, Basic Type B
volatile __ioport unsigned char	DBB00FN @	0x120;	// Function Register
volatile __ioport unsigned char	DBB00IN @	0x121;	//    Input Register
volatile __ioport unsigned char	DBB00OU @	0x122;	//   Output Register

// Digital PSoC block 01, Basic Type B
volatile __ioport unsigned char	DBB01FN @	0x124;	// Function Register
volatile __ioport unsigned char	DBB01IN @	0x125;	//    Input Register
volatile __ioport unsigned char	DBB01OU @	0x126;	//   Output Register

// Digital PSoC block 02, Communications Type B
volatile __ioport unsigned char	DCB02FN @	0x128;	// Function Register
volatile __ioport unsigned char	DCB02IN @	0x129;	//    Input Register
volatile __ioport unsigned char	DCB02OU @	0x12A;	//   Output Register

// Digital PSoC block 03, Communications Type B
volatile __ioport unsigned char	DCB03FN @	0x12C;	// Function Register
volatile __ioport unsigned char	DCB03IN @	0x12D;	//    Input Register
volatile __ioport unsigned char	DCB03OU @	0x12E;	//   Output Register

//-----------------------------------------------
//  PMA Write and Read Pointer Registers
//-----------------------------------------------
volatile __ioport unsigned char	PMA0_WA @	0x140;	// PMA 0 Write Pointer
volatile __ioport unsigned char	PMA1_WA @	0x141;	// PMA 1 Write Pointer
volatile __ioport unsigned char	PMA2_WA @	0x142;	// PMA 2 Write Pointer
volatile __ioport unsigned char	PMA3_WA @	0x143;	// PMA 3 Write Pointer
volatile __ioport unsigned char	PMA4_WA @	0x144;	// PMA 4 Write Pointer
volatile __ioport unsigned char	PMA5_WA @	0x145;	// PMA 5 Write Pointer
volatile __ioport unsigned char	PMA6_WA @	0x146;	// PMA 6 Write Pointer
volatile __ioport unsigned char	PMA7_WA @	0x147;	// PMA 7 Write Pointer

volatile __ioport unsigned char	PMA0_RA @	0x150;	// PMA 0 Read Pointer
volatile __ioport unsigned char	PMA1_RA @	0x151;	// PMA 1 Read Pointer
volatile __ioport unsigned char	PMA2_RA @	0x152;	// PMA 2 Read Pointer
volatile __ioport unsigned char	PMA3_RA @	0x153;	// PMA 3 Read Pointer
volatile __ioport unsigned char	PMA4_RA @	0x154;	// PMA 4 Read Pointer
volatile __ioport unsigned char	PMA5_RA @	0x155;	// PMA 5 Read Pointer
volatile __ioport unsigned char	PMA6_RA @	0x156;	// PMA 6 Read Pointer
volatile __ioport unsigned char	PMA7_RA @	0x157;	// PMA 7 Read Pointer

//-----------------------------------------------
//  System and Global Resource Registers
//  Note: Also see this address range in Bank 0.
//-----------------------------------------------

volatile __ioport unsigned char	CLK_CR0 @	0x160;	// Analog Column Clock Select Register
#define CLK_CR0_ACOLUMN1              (0x0C)
#define CLK_CR0_ACOLUMN0              (0x03)

volatile __ioport unsigned char	CLK_CR1 @	0x161;	// Analog Clock Source Select Register
#define CLK_CR1_SHDIS                 (0x40)
#define CLK_CR1_ACLK1                 (0x38)
#define CLK_CR1_ACLK2                 (0x07)

volatile __ioport unsigned char	ABF_CR0 @	0x162;	// Analog Output Buffer Control Register
#define ABF_CR0_ACOL1MUX              (0x80)
#define ABF_CR0_ABUF1EN               (0x20)
#define ABF_CR0_ABUF0EN               (0x08)
#define ABF_CR0_BYPASS                (0x02)
#define ABF_CR0_PWR                   (0x01)

volatile __ioport unsigned char	AMD_CR0 @	0x163;	// Analog Modulator Control Register
#define AMD_CR0_AMOD0                 (0x07)

volatile __ioport unsigned char	CMP_GO_EN @	0x164;	// Comparator Bus To Global Out Enable
#define CMP_GO_EN_GOO5                (0x80)
#define CMP_GO_EN_GOO1                (0x40)
#define CMP_GO_EN_SEL1                (0x30)
#define CMP_GO_EN_GOO4                (0x08)
#define CMP_GO_EN_GOO0                (0x04)
#define CMP_GO_EN_SEL0                (0x03)

volatile __ioport unsigned char	CMP_GO_EN1 @	0x165;	// Comparator Bus To Global Out Enable 1
#define CMP_GO_EN1_GOO7               (0x80)
#define CMP_GO_EN1_GOO3               (0x40)
#define CMP_GO_EN1_SEL3               (0x30)
#define CMP_GO_EN1_GOO6               (0x08)
#define CMP_GO_EN1_GOO2               (0x04)
#define CMP_GO_EN1_SEL2               (0x03)

volatile __ioport unsigned char	AMD_CR1 @	0x166;	// Analog Modulator Control Register 1
#define AMD_CR1_AMOD1                 (0x07)

volatile __ioport unsigned char	ALT_CR0 @	0x167;	// Analog Look Up Table (LUT) Register 0
#define ALT_CR0_LUT1                  (0xF0)
#define ALT_CR0_LUT0                  (0x0F)


//-----------------------------------------------
//  USB Registers
//-----------------------------------------------
volatile __ioport unsigned char	USB_CR1 @	0x1C1;	// USB Control Register 1
#define USB_CR1_BUS_ACTIVITY           (0x04)
#define USB_CR1_ENABLE_LOCK            (0x02)
#define USB_CR1_REG_ENABLE             (0x01)

volatile __ioport unsigned char	EP1_CR0 @	0x1C4;	// Endpoint 1 Control Register 0
#define EP1_CR0_STALL                  (0x80)
#define EP1_CR0_NAK_INT_EN             (0x20)
#define EP1_CR0_ACKD                   (0x10)
#define EP1_CR0_MODE                   (0x0F)

volatile __ioport unsigned char	EP2_CR0 @	0x1C5;	// Endpoint 2 Control Register 0
#define EP2_CR0_STALL                  (0x80)
#define EP2_CR0_NAK_INT_EN             (0x20)
#define EP2_CR0_ACKD                   (0x10)
#define EP2_CR0_MODE                   (0x0F)

volatile __ioport unsigned char	EP3_CR0 @	0x1C6;	// Endpoint 3 Control Register 0
#define EP3_CR0_STALL                  (0x80)
#define EP3_CR0_NAK_INT_EN             (0x20)
#define EP3_CR0_ACKD                   (0x10)
#define EP3_CR0_MODE                   (0x0F)

volatile __ioport unsigned char	EP4_CR0 @	0x1C7;	// Endpoint 4 Control Register 0
#define EP4_CR0_STALL                  (0x80)
#define EP4_CR0_NAK_INT_EN             (0x20)
#define EP4_CR0_ACKD                   (0x10)
#define EP4_CR0_MODE                   (0x0F)

//-----------------------------------------------
//  Global Digital Interconnects
//-----------------------------------------------


volatile __ioport unsigned char	GDI_O_IN @	0x1D0;	// Global Dig Interconnect Odd Inputs
volatile __ioport unsigned char	GDI_E_IN @	0x1D1;	// Global Dig Interconnect Even Inputs
volatile __ioport unsigned char	GDI_O_OU @	0x1D2;	// Global Dig Interconnect Odd Outputs
volatile __ioport unsigned char	GDI_E_OU @	0x1D3;	// Global Dig Interconnect Even Outputs

//------------------------------------------------
//  Analog Mux Bus Select Registers
//------------------------------------------------

volatile __ioport unsigned char	MUX_CR0 @	0x1D8;	// Analog Mux Port 0 Bit Enables
volatile __ioport unsigned char	MUX_CR1 @	0x1D9;	// Analog Mux Port 1 Bit Enables
volatile __ioport unsigned char	MUX_CR2 @	0x1DA;	// Analog Mux Port 2 Bit Enables
volatile __ioport unsigned char	MUX_CR3 @	0x1DB;	// Analog Mux Port 3 Bit Enables
volatile __ioport unsigned char	MUX_CR4 @	0x1EC;	// Analog Mux Port 4 Bit Enables
volatile __ioport unsigned char	MUX_CR5 @	0x1ED;	// Analog Mux Port 5 Bit Enables

//------------------------------------------------
//  Clock and System Control Registers
//------------------------------------------------

volatile __ioport unsigned char	OSC_GO_EN @	0x1DD;	// Oscillator to Global Outputs Enable Register (RW)
#define OSC_GO_EN_SLPINT              (0x80)
#define OSC_GO_EN_VC3                 (0x40)
#define OSC_GO_EN_VC2                 (0x20)
#define OSC_GO_EN_VC1                 (0x10)
#define OSC_GO_EN_SYSCLKX2            (0x08)
#define OSC_GO_EN_SYSCLK              (0x04)
#define OSC_GO_EN_CLK24M              (0x02)
#define OSC_GO_EN_CLK32K              (0x01)

volatile __ioport unsigned char	OSC_CR4 @	0x1DE;	// Oscillator Control Register 4
#define OSC_CR4_VC3SEL                (0x03)

volatile __ioport unsigned char	OSC_CR3 @	0x1DF;	// Oscillator Control Register 3

volatile __ioport unsigned char	OSC_CR0 @	0x1E0;	// System Oscillator Control Register 0
#define OSC_CR0_32K_SELECT            (0x80)
#define OSC_CR0_PLL_MODE              (0x40)
#define OSC_CR0_NO_BUZZ               (0x20)
#define OSC_CR0_SLEEP                 (0x18)
#define OSC_CR0_SLEEP_512Hz           (0x00)
#define OSC_CR0_SLEEP_64Hz            (0x08)
#define OSC_CR0_SLEEP_8Hz             (0x10)
#define OSC_CR0_SLEEP_1Hz             (0x18)
#define OSC_CR0_CPU                   (0x07)
#define OSC_CR0_CPU_3MHz              (0x00)
#define OSC_CR0_CPU_6MHz              (0x01)
#define OSC_CR0_CPU_12MHz             (0x02)
#define OSC_CR0_CPU_24MHz             (0x03)
#define OSC_CR0_CPU_1d5MHz            (0x04)
#define OSC_CR0_CPU_750kHz            (0x05)
#define OSC_CR0_CPU_187d5kHz          (0x06)
#define OSC_CR0_CPU_93d7kHz           (0x07)

volatile __ioport unsigned char	OSC_CR1 @	0x1E1;	// System V1/V2 Divider Control Register
#define OSC_CR1_VC1                   (0xF0)
#define OSC_CR1_VC2                   (0x0F)

volatile __ioport unsigned char	OSC_CR2 @	0x1E2;	// Oscillator Control Register 2
#define OSC_CR2_PLLGAIN               (0x80)
#define OSC_CR2_EXTCLKEN              (0x04)
#define OSC_CR2_IMODIS                (0x02)
#define OSC_CR2_SYSCLKX2DIS           (0x01)

volatile __ioport unsigned char	VLT_CR @	0x1E3;	// Voltage Monitor Control Register
#define VLT_CR_SMP                    (0x80)
#define VLT_CR_PORLEV                 (0x30)
#define VLT_CR_POR_MID                (0x10)
#define VLT_CR_POR_HIGH               (0x20)
#define VLT_CR_LVDTBEN                (0x08)
#define VLT_CR_VM                     (0x07)

volatile __ioport unsigned char	VLT_CMP @	0x1E4;	// Voltage Monitor Comparators Register
#define VLT_CMP_PUMP                  (0x04)
#define VLT_CMP_LVD                   (0x02)
#define VLT_CMP_PPOR                  (0x01)

volatile __ioport unsigned char	DEC_CR2 @	0x1E7;	// Data Control Register

volatile __ioport unsigned char	IMO_TR @	0x1E8;	// Internal Main Oscillator Trim Register
volatile __ioport unsigned char	ILO_TR @	0x1E9;	// Internal Low-speed Oscillator Trim
volatile __ioport unsigned char	BDG_TR @	0x1EA;	// Band Gap Trim Register
volatile __ioport unsigned char	ECO_TR @	0x1EB;	// External Oscillator Trim Register
volatile __ioport unsigned char	IMO_TR2 @	0x1EF;	// Internal Main Oscillator Gain Trim Register

volatile __ioport unsigned char	DAC_CR @	0x1FD;	// DAC Control Register



#define DAC_CR_SPLIT_MUX              (0x80)
#define DAC_CR_MUXCLK_GE              (0x40)
#define DAC_CR_RANGE                  (0x08)
#define DAC_CR_OSCMODE                (0x06)
#define DAC_CR_ENABLE                 (0x01)

// interrupt vectors for the USB stuff

#define	USB_BUS_RESET_VECTOR	0x40
#define	USB_SOF_VECTOR			0x44
#define	USB_EP0_VECTOR			0x48
#define	USB_EP1_VECTOR			0x4C
#define	USB_EP2_VECTOR			0x50
#define	USB_EP3_VECTOR			0x54
#define	USB_EP4_VECTOR			0x58
#define	USB_WAKEUP_VECTOR		0x5C

#endif
