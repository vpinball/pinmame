// based on java source by maddes

// checks and corrects, and optionally writes checksum corrected rom for WPC based machines
// arguments: wpc_cc.exe in.rom [out.rom]

#include <vector>

#define DEVELOPMENT_CHECKSUM 0x00FF

static unsigned short calcShortChecksum(const std::vector<unsigned char>& data)
{
	unsigned short sum = 0;
	for (size_t i = 0; i < data.size(); i++)
		sum += data[i];
	return sum;
}

static unsigned short getShort(const unsigned char* const ary, const unsigned int ofs)
{
	return ((unsigned short)(ary[ofs]) << 8) | (unsigned short)(ary[ofs+1]);
}

static char* toHex(const unsigned char c)
{
	if (c <= 0xF)
		return "0";
	else
		return "";
}

static char* toHex(const unsigned short c)
{
	if (c <= 0xF)
		return "000";
	else if (c <= 0xFF)
		return "00";
	else if (c <= 0xFFF)
		return "0";
	else
		return "";
}

int main(int argc, char* argv[])
{
	FILE* fp;
	fopen_s(&fp, argv[1], "rb");
	fseek(fp, 0u, SEEK_END);
	const size_t size = ftell(fp);
	fseek(fp, 0u, SEEK_SET);
	std::vector<unsigned char> data;
	data.resize(size);
	fread(data.data(), 1u, size, fp);
	fclose(fp);

	// calculate checksum/correction offsets
	const unsigned int ofsCO_high = size - 20;
	const unsigned int ofsCO_low = ofsCO_high + 1;
	const unsigned int ofsCS_high = ofsCO_low + 1;
	const unsigned int ofsCS_low = ofsCS_high + 1;

	// Validate checksum
	printf("Read correction: %s%X%s%X\n", toHex(data[ofsCO_high]), data[ofsCO_high], toHex(data[ofsCO_low]), data[ofsCO_low]);
	printf("Read checksum: %s%X%s%X is ", toHex(data[ofsCS_high]), data[ofsCS_high], toHex(data[ofsCS_low]), data[ofsCS_low]);

	const unsigned short dataCSorg = getShort(data.data(), ofsCS_high);
	const unsigned short dataCOorg = getShort(data.data(), ofsCO_high);

	const unsigned short CalcCSorg = calcShortChecksum(data);

	bool checksum_ok = true;
	if (CalcCSorg != dataCSorg)
		checksum_ok = false;
	else if (data[ofsCO_low] != 0xFF - data[ofsCS_high])
		checksum_ok = false;
	else if (dataCOorg == DEVELOPMENT_CHECKSUM)
		checksum_ok = false;

	if (!checksum_ok)
	{
		printf("INVALID!!!\n\n");
		printf("Calculated checksum: %s%X\n\n", toHex(CalcCSorg), CalcCSorg);
	}
	else
		printf("OK!\n\n");


	// start correcting
	const unsigned char lowbyte = data[ofsCS_low];
	printf("Re-calculating checksum for version low byte %s%X\n",toHex(lowbyte),lowbyte);

	// Initialize ROM for checksum calculation
	data[ofsCO_high] = 0x00;
	data[ofsCO_low] = 0xFF;
	data[ofsCS_high] = 0x00;
	data[ofsCS_low] = 0xFF;

	printf(" Initializing ROM...\n");
	printf("  Correction slot: %s%X%s%X\n",toHex(data[ofsCO_high]),data[ofsCO_high],toHex(data[ofsCO_low]),data[ofsCO_low]);
	printf("  Checksum slot: %s%X%s%X\n",toHex(data[ofsCS_high]),data[ofsCS_high],toHex(data[ofsCS_low]),data[ofsCS_low]);

	// Get checksum for initialized ROM
	const unsigned short calcCS = calcShortChecksum(data);
	printf(" Calculated checksum: %s%X\n",toHex(calcCS),calcCS);

	// Calculate 100% Williams style checksum and correction
	printf(" Calculating Williams style checksum...\n");

	// CO high byte (compensates CS low byte)
	// CO high byte = 0xFF - CS low byte
	data[ofsCO_high] = 0xFF - (calcCS & 0xFF);
	printf("  Correction high byte: FF - %s%X = %s%X\n",toHex((unsigned char)(calcCS & 0xFF)),calcCS & 0xFF,toHex(data[ofsCO_high]),data[ofsCO_high]);

	// CS low byte (can be set to anything as it was compensated to zero difference before)
	data[ofsCS_low] = lowbyte;

	// CS high byte
	data[ofsCS_high] = (calcCS >> 8) & 0xFF;

	// CO low byte (compensates CS high byte)
	// CO low byte = 0xFF - CS high byte
	data[ofsCO_low] = 0xFF - data[ofsCS_high];
	printf("  Correction low byte: FF - %s%X = %s%X\n\n",toHex(data[ofsCS_high]),data[ofsCS_high],toHex(data[ofsCO_low]),data[ofsCO_low]);

	printf("Final correction: %s%X%s%X\n",toHex(data[ofsCO_high]),data[ofsCO_high],toHex(data[ofsCO_low]),data[ofsCO_low]);
	printf("Final checksum: %s%X%s%X is ",toHex(data[ofsCS_high]),data[ofsCS_high],toHex(data[ofsCS_low]),data[ofsCS_low]);

	// Validate checksum
	const unsigned short dataCS = getShort(data.data(), ofsCS_high);
	const unsigned short dataCO = getShort(data.data(), ofsCO_high);

	const unsigned short newCalcCS = calcShortChecksum(data);

	checksum_ok = true;
	if (newCalcCS != dataCS)
		checksum_ok = false;
	else if (data[ofsCO_low] != 0xFF - data[ofsCS_high])
		checksum_ok = false;
	else if (dataCO == DEVELOPMENT_CHECKSUM)
		checksum_ok = false;

	if (!checksum_ok)
	{
		printf("INVALID!!!\n\n");
		printf("Calculated checksum: %s%X\n", toHex(newCalcCS),newCalcCS);
	}
	else
		printf("OK!\n");

	// optionally, write corrected file
	if((argc >= 3) && (argv[2]))
	{
		fopen_s(&fp, argv[2], "wb");
		fwrite(data.data(), 1u, size, fp);
		fclose(fp);
	}
}
