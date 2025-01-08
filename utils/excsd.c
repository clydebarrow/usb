#include	<stdio.h>

static struct
{
	unsigned char *	name;
	unsigned int	numbits;
	unsigned int	value;
}	fields[] = 
{
	{ "always 1", 1,0},
	{ "CRC", 7,0},
	{ "Reserved", 2,0},
	{ "File format", 2,0},
	{ "temporary write protection", 1,0},
	{ "permanent write protection", 1,0},
	{ "copy flag", 1,0},
	{ "File format group FILE_FORMAT_GRP", 1,0},
	{ "Reserved", 5,0},
	{ "partial blocks for write allowed", 1,0},
	{ "max. write block length", 4,0},
	{ "write speed factor", 3,0},
	{ "Reserved MMC compatibility", 2,0},
	{ "write protect group enable", 1,0},
	{ "write protect group size", 7,0},
	{ "erase sector size", 7,0},
	{ "erase single block enable", 1,0},
	{ "C_SIZE_MULT", 3,0},
	{ "max. write current @VDD max", 3,0},
	{ "max. write current @VDD min", 3,0},
	{ "max. read current @VDD max", 3,0},
	{ "max. read current @VDD min", 3,0},
	{ "C_SIZE", 12,0},
	{ "Reserved", 2,0},
	{ "DSR implemented", 1,0},
	{ "read block misalignment", 1,0},
	{ "write block misalignment", 1,0},
	{ "partial blocks for read allowed", 1,0},
	{ "read data block length", 4,0},
	{ "card command classes", 12,0},
	{ "TRAN_SPEED", 8,0},
	{ "NSAC", 8,0},
	{ "TAAC", 8,0},
	{ "Reserved", 6,0},
	{ "CSD structure", 2,0},
};


unsigned char	csd[16], csdrev[16];

static unsigned long
get_bits(unsigned bitno, unsigned numbits)
{
	unsigned long	n;
	unsigned int	i;

	n = 0;
	i = bitno / 8;
	n = csdrev[i++];
	n += csdrev[i++] << 8;
	n += csdrev[i++] << 16;
	n += csdrev[i++] << 24;
	n >>= bitno % 8;
	return n & ((1 << numbits)-1);
}

main(int argc, char ** argv)
{
	unsigned int i, bitno, val;

	if(argc > 1) {
		if(!freopen(argv[1], "rb", stdin)) {
			perror(argv[1]);
			exit(1);
		}
	}
	if(fread(csd, sizeof csd, 1, stdin) != 1) {
		perror("read");
		exit(1);
	}
	for(i = 0 ; i != sizeof csd ; i++)
		csdrev[15-i] = csd[i];

	bitno = 0;
	for(i = 0 ; i != sizeof fields/sizeof fields[0] ; i++) {
		val = get_bits(bitno, fields[i].numbits);
		fields[i].value = val;
		printf("%s = %d (0x%X)\n", fields[i].name, val, val);
		bitno += fields[i].numbits;
	}
	exit(0);
}
