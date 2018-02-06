#include <Base.h>
#include <Uefi.h>

#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>
#include <string.h>

#include <Protocol/UnicodeCollation.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/DevicePathFromText.h>
#include <Protocol/DevicePathToText.h>
#include <Protocol/DebugPort.h>
#include <Protocol/DebugSupport.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/BlockIo.h>
#include <Protocol/BlockIo2.h>
#include <Protocol/DiskIo.h>
#include <Protocol/DiskIo2.h>
#include <Protocol/ComponentName.h>
#include <Protocol/ComponentName2.h>

#include <Guid/FileSystemInfo.h>
#include <Guid/FileInfo.h>
#include <Guid/FileSystemVolumeLabelInfo.h>

#define BUFFER_SIZE 512

static EFI_GUID BlockIoProtocolGUID = BLOCK_IO_PROTOCOL;
static EFI_GUID DevicePathGUID = DEVICE_PATH_PROTOCOL;
EFI_BLOCK_IO *blockIOProtocol;
CHAR16 EFI_SIGNATURE[] = {0x4645, 0x2049,0x4150,0x5452};

CHAR16 buff[BUFFER_SIZE];
CHAR16 buff2[4];
SIMPLE_TEXT_OUTPUT_INTERFACE *conOut;
EFI_LBA currLBA;
EFI_LBA GPTLBA;
EFI_LBA GPTHeader2;
int GPTHeaderSize;
int GPTEntrySize;
int GPTEntryCount;

//////////////////////////////////////////////
//////////////// CRC32 ///////////////////////
//////////////////////////////////////////////
UINT32  CrcTable[256] = {
  0x00000000,  0x77073096,  0xEE0E612C,  0x990951BA,  0x076DC419,  0x706AF48F,  0xE963A535,  0x9E6495A3,  0x0EDB8832,  0x79DCB8A4,  0xE0D5E91E,  0x97D2D988,
  0x09B64C2B,  0x7EB17CBD,  0xE7B82D07,  0x90BF1D91,  0x1DB71064,  0x6AB020F2,  0xF3B97148,  0x84BE41DE,  0x1ADAD47D,  0x6DDDE4EB,  0xF4D4B551,  0x83D385C7,
  0x136C9856,  0x646BA8C0,  0xFD62F97A,  0x8A65C9EC,  0x14015C4F,  0x63066CD9,  0xFA0F3D63,  0x8D080DF5,  0x3B6E20C8,  0x4C69105E,  0xD56041E4,  0xA2677172,
  0x3C03E4D1,  0x4B04D447,  0xD20D85FD,  0xA50AB56B,  0x35B5A8FA,  0x42B2986C,  0xDBBBC9D6,  0xACBCF940,  0x32D86CE3,  0x45DF5C75,  0xDCD60DCF,  0xABD13D59,
  0x26D930AC,  0x51DE003A,  0xC8D75180,  0xBFD06116,  0x21B4F4B5,  0x56B3C423,  0xCFBA9599,  0xB8BDA50F,  0x2802B89E,  0x5F058808,  0xC60CD9B2,  0xB10BE924,
  0x2F6F7C87,  0x58684C11,  0xC1611DAB,  0xB6662D3D,  0x76DC4190,  0x01DB7106,  0x98D220BC,  0xEFD5102A,  0x71B18589,  0x06B6B51F,  0x9FBFE4A5,  0xE8B8D433,
  0x7807C9A2,  0x0F00F934,  0x9609A88E,  0xE10E9818,  0x7F6A0DBB,  0x086D3D2D,  0x91646C97,  0xE6635C01,  0x6B6B51F4,  0x1C6C6162,  0x856530D8,  0xF262004E,
  0x6C0695ED,  0x1B01A57B,  0x8208F4C1,  0xF50FC457,  0x65B0D9C6,  0x12B7E950,  0x8BBEB8EA,  0xFCB9887C,  0x62DD1DDF,  0x15DA2D49,  0x8CD37CF3,  0xFBD44C65,
  0x4DB26158,  0x3AB551CE,  0xA3BC0074,  0xD4BB30E2,  0x4ADFA541,  0x3DD895D7,  0xA4D1C46D,  0xD3D6F4FB,  0x4369E96A,  0x346ED9FC,  0xAD678846,  0xDA60B8D0,
  0x44042D73,  0x33031DE5,  0xAA0A4C5F,  0xDD0D7CC9,  0x5005713C,  0x270241AA,  0xBE0B1010,  0xC90C2086,  0x5768B525,  0x206F85B3,  0xB966D409,  0xCE61E49F,
  0x5EDEF90E,  0x29D9C998,  0xB0D09822,  0xC7D7A8B4,  0x59B33D17,  0x2EB40D81,  0xB7BD5C3B,  0xC0BA6CAD,  0xEDB88320,  0x9ABFB3B6,  0x03B6E20C,  0x74B1D29A,
  0xEAD54739,  0x9DD277AF,  0x04DB2615,  0x73DC1683,  0xE3630B12,  0x94643B84,  0x0D6D6A3E,  0x7A6A5AA8,  0xE40ECF0B,  0x9309FF9D,  0x0A00AE27,  0x7D079EB1,
  0xF00F9344,  0x8708A3D2,  0x1E01F268,  0x6906C2FE,  0xF762575D,  0x806567CB,  0x196C3671,  0x6E6B06E7,  0xFED41B76,  0x89D32BE0,  0x10DA7A5A,  0x67DD4ACC,
  0xF9B9DF6F,  0x8EBEEFF9,  0x17B7BE43,  0x60B08ED5,  0xD6D6A3E8,  0xA1D1937E,  0x38D8C2C4,  0x4FDFF252,  0xD1BB67F1,  0xA6BC5767,  0x3FB506DD,  0x48B2364B,
  0xD80D2BDA,  0xAF0A1B4C,  0x36034AF6,  0x41047A60,  0xDF60EFC3,  0xA867DF55,  0x316E8EEF,  0x4669BE79,  0xCB61B38C,  0xBC66831A,  0x256FD2A0,  0x5268E236,
  0xCC0C7795,  0xBB0B4703,  0x220216B9,  0x5505262F,  0xC5BA3BBE,  0xB2BD0B28,  0x2BB45A92,  0x5CB36A04,  0xC2D7FFA7,  0xB5D0CF31,  0x2CD99E8B,  0x5BDEAE1D,
  0x9B64C2B0,  0xEC63F226,  0x756AA39C,  0x026D930A,  0x9C0906A9,  0xEB0E363F,  0x72076785,  0x05005713,  0x95BF4A82,  0xE2B87A14,  0x7BB12BAE,  0x0CB61B38,
  0x92D28E9B,  0xE5D5BE0D,  0x7CDCEFB7,  0x0BDBDF21,  0x86D3D2D4,  0xF1D4E242,  0x68DDB3F8,  0x1FDA836E,  0x81BE16CD,  0xF6B9265B,  0x6FB077E1,  0x18B74777,
  0x88085AE6,  0xFF0F6A70,  0x66063BCA,  0x11010B5C,  0x8F659EFF,  0xF862AE69,  0x616BFFD3,  0x166CCF45,  0xA00AE278,  0xD70DD2EE,  0x4E048354,  0x3903B3C2,
  0xA7672661,  0xD06016F7,  0x4969474D,  0x3E6E77DB,  0xAED16A4A,  0xD9D65ADC,  0x40DF0B66,  0x37D83BF0,  0xA9BCAE53,  0xDEBB9EC5,  0x47B2CF7F,  0x30B5FFE9,
  0xBDBDF21C,  0xCABAC28A,  0x53B39330,  0x24B4A3A6,  0xBAD03605,  0xCDD70693,  0x54DE5729,  0x23D967BF,  0xB3667A2E,  0xC4614AB8,  0x5D681B02,  0x2A6F2B94,
  0xB40BBE37,  0xC30C8EA1,  0x5A05DF1B,  0x2D02EF8D
};

UINT32 xcrc32 (CHAR16 *buf, int len)
{
    UINT32 Crc = 0xffffffff;
    UINT8   *Ptr = (UINT8*)buf;
    int i;
    for (i = 0; i < len; i++, Ptr++) {
        Crc = (Crc >> 8) ^ CrcTable[(UINT8) Crc ^ *Ptr];
    }
    return Crc ^ 0xffffffff;
}

///////////////////////////////////////////////////
/////////////////// MISC //////////////////////////
///////////////////////////////////////////////////

char getText(CHAR16* text,int size, EFI_SYSTEM_TABLE *systemTable)
{
    int i;
    EFI_INPUT_KEY* key;
	EFI_EVENT event = systemTable->ConIn->WaitForKey;
    UINTN index;
    for(i=0;i<size;)
    {
        systemTable->BootServices->WaitForEvent(1, &event, &index);
        systemTable->ConIn->ReadKeyStroke(systemTable->ConIn,key);
        if(key->ScanCode == (UINT16)0x17)
        {
            return 0;
            
        }
        if(key->UnicodeChar != 0x00)
        {
            if(key->UnicodeChar == (CHAR16)'\r')
            {
                text[i] = 0x0000;
                break;
            }
            if(key->UnicodeChar == (CHAR16)8)
            {
                if(i!=0)
                {
                    text[--i] = 0x0000;    
                    Print(&(key->UnicodeChar));
                }
                continue;
            }
            Print(&(key->UnicodeChar));
            text[i++] = key->UnicodeChar;
        }
    }
    return 1;
}

long long pow(int x,int a)
{
    long r=1;
    for(;a>0;a--)
    {
        r*=x;
    }
    return r;
}

int arr2int(CHAR16* arr, int size)
{
    int i,r=0;
    for(i=0;i<size;i++)
        r+= ((int)(arr[i]-(CHAR16)'0'))*pow(10,size-i-1);
    return r;
}

int arr16_2int(CHAR16* arr, int size)
{
    int i,r=0;
    for(i=0;i<size;i++)
    {
        if(arr[i] >= (CHAR16)'0' && arr[i] <= (CHAR16)'9')
            r+= ((int)(arr[i]-(CHAR16)'0'))*pow(16,size-i-1);
        else if(arr[i] == (CHAR16)'a' || arr[i] == (CHAR16)'A')
            r+= ((int)(10))*pow(16,size-i-1);
        else if(arr[i] == (CHAR16)'b' || arr[i] == (CHAR16)'B')
            r+= ((int)(11))*pow(16,size-i-1);
        else if(arr[i] == (CHAR16)'c' || arr[i] == (CHAR16)'C')
            r+= ((int)(12))*pow(16,size-i-1);
        else if(arr[i] == (CHAR16)'d' || arr[i] == (CHAR16)'D')
            r+= ((int)(13))*pow(16,size-i-1);
        else if(arr[i] == (CHAR16)'e' || arr[i] == (CHAR16)'E')
            r+= ((int)(14))*pow(16,size-i-1);
        else if(arr[i] == (CHAR16)'f' || arr[i] == (CHAR16)'F')
            r+= ((int)(15))*pow(16,size-i-1);
    }
    return r;
}

EFI_LBA arr2lba(CHAR16* arr, int size)
{
    int i;
    EFI_LBA r=0;
    for(i=0;i<size;i++)
        r+= ((EFI_LBA)(arr[i]-(CHAR16)'0'))*pow(10,size-i-1);
    return r;
}
/*
 * ret:
 *  -1 -> s1 > s2 
 *   0 -> s1 == s2
 *   1 -> s1 < s2
 *   -2 -> error
 */
int strcmp16(CHAR16* s1, CHAR16* s2)
{
    if(sizeof(s1) > sizeof(s2))
    {
        return -1;
    }
    else if(sizeof(s1) < sizeof(s2))
    {
        return 1;
    }
    else
    {
        int i;
        for(i=0;i<sizeof(s1)/sizeof(CHAR16);i++)
        {
            if(s1[i] - s2[i])
            {
                return (int)s1[i] - s2[i];
            }
        }
        return 0;
    }
    return -2;
}

void strcpy16(CHAR16* source, CHAR16* dest)
{
    while(*source)
    {
        *dest = *source;
        source++;dest++;
    }
    return;
}

unsigned int BE2int(CHAR16* block, int size)
{
    unsigned int i,r=0;
    for(i=0;i<size;i++)
        r+= ((unsigned int)block[i])<<(16*(i));
    return r;
}

unsigned long long BE2long(CHAR16* block, int size)
{
    unsigned long long i,r=0;
    for(i=0;i<size;i++)
        r+= ((unsigned long long)block[i])<<(16*(i));
    return r;
}

///////////////////////////////////////////////////
/////////////////// PRINTS ////////////////////////
///////////////////////////////////////////////////

void printInt(SIMPLE_TEXT_OUTPUT_INTERFACE *conOut, unsigned int value) 
{
	CHAR16 out[32];
	CHAR16 *ptr = out;
	if (value == 0) {
		conOut->OutputString(conOut, L"0");
		return;
	}

	ptr += 31;
	*--ptr = 0;
	unsigned int tmp = value;// >= 0 ? value : -value; 
	
	while (tmp) {
		*--ptr = '0' + tmp % 10;
		tmp /= 10;
	}
	conOut->OutputString(conOut, ptr);
}

void printLong(SIMPLE_TEXT_OUTPUT_INTERFACE *conOut, unsigned long long value) 
{
	CHAR16 out[64];
	CHAR16 *ptr = out;
	if (value == 0) {
		conOut->OutputString(conOut, L"0");
		return;
	}

	ptr += 63;
	*--ptr = 0;
	unsigned long long tmp = value;// >= 0 ? value : -value; 
	
	while (tmp) {
		*--ptr = '0' + tmp % 10;
		tmp /= 10;
	}
	conOut->OutputString(conOut, ptr);
}

void printCHAR16(SIMPLE_TEXT_OUTPUT_INTERFACE *conOut, CHAR16 c)
{
    CHAR16 out[4];
	CHAR16 *ptr = out;
	if (c == 0) {
		conOut->OutputString(conOut, L"0000");
		return;
	}

	ptr += 3;
	*--ptr = 0;
	CHAR16 tmp = c;// >= 0 ? value : -value; 
	
	while (tmp) {
		if(tmp % 16 < 10)
            *--ptr = '0' + tmp % 16;
        else if(tmp % 16 == 10)
            *--ptr = 'A';
        else if(tmp % 16 == 11)
            *--ptr = 'B';
        else if(tmp % 16 == 12)
            *--ptr = 'C';
        else if(tmp % 16 == 13)
            *--ptr = 'D';
        else if(tmp % 16 == 14)
            *--ptr = 'E';
        else if(tmp % 16 == 15)
            *--ptr = 'F';
		tmp /= 16;
	}
	conOut->OutputString(conOut, ptr);
}

void printGUID(SIMPLE_TEXT_OUTPUT_INTERFACE *conOut, CHAR16* buff)
{
    CHAR16 tmp;
    printCHAR16(conOut,*(buff+1));
    printCHAR16(conOut,*buff);
    Print(L"-");
    printCHAR16(conOut,*(buff+2));
    Print(L"-");
    printCHAR16(conOut,*(buff+3));
    Print(L"-");
    tmp = (*(buff+4)>>8)|(*(buff+4)<<8);
    printCHAR16(conOut,tmp);
    Print(L"-");
    tmp = (*(buff+5)>>8)|(*(buff+5)<<8);
    printCHAR16(conOut,tmp);
    tmp = (*(buff+6)>>8)|(*(buff+6)<<8);
    printCHAR16(conOut,tmp);
    tmp = (*(buff+7)>>8)|(*(buff+7)<<8);
    printCHAR16(conOut,tmp);
}

void printGPTHeader()
{
    Print(L"==========GPT HEADER==========\n\r");
    
    Print(L"REVISON NO:\t");
    printInt(conOut,buff[5]);
    Print(L".");
    printInt(conOut,buff[4]);
    Print(L"\n\r");
    
    Print(L"HEADER SIZE:\t");
    GPTHeaderSize = BE2int(buff+6,2);
    printInt(conOut,GPTHeaderSize);
    Print(L"\n\r");
    
    Print(L"HEADER CRC32:\t");
    printInt(conOut,BE2int(buff+8,2));
    Print(L"\n\r");
    
    Print(L"RESERVED:\t");
    printInt(conOut,BE2int(buff+10,2));
    Print(L"\n\r");
    
    Print(L"MY LBA:\t");
    printLong(conOut,BE2long(buff+12,4));
    Print(L"\n\r");
    
    Print(L"ALTERNATE LBA:\t");
    GPTHeader2 = (EFI_LBA)BE2long(buff+16,4);
    printLong(conOut,BE2long(buff+16,4));
    Print(L"\n\r");
    
    Print(L"FIRST USABLE LBA:\t");
    printLong(conOut,BE2long(buff+20,4));
    Print(L"\n\r");

    Print(L"LAST USABLE LBA:\t");
    printLong(conOut,BE2long(buff+24,4));
    Print(L"\n\r");
    
    Print(L"DISK GUID:\t");
    printGUID(conOut,buff+28);
    Print(L"\n\r");
    
    Print(L"PARTITION ENTRY LBA:\t");
    GPTLBA = (EFI_LBA)BE2long(buff+36,4);
    printLong(conOut,BE2long(buff+36,4));
    Print(L"\n\r");
    
    Print(L"NUMBER OF PARTITION ENTRIES:\t");
    GPTEntryCount = BE2int(buff+40,2);
    printInt(conOut,BE2int(buff+40,2));
    Print(L"\n\r");
    
    Print(L"SIZE OF PARTITION ENTRY:\t");
    GPTEntrySize = BE2int(buff+42,2);
    printInt(conOut,GPTEntrySize);
    Print(L"\n\r");
    
    Print(L"PARTITION ENTRY ARRAY CRC32:\t");
    printInt(conOut,BE2int(buff+44,2));
    Print(L"\n\r");
}

void printGPTHeaderEdit(char pos)
{
    Print(L"==========GPT HEADER==========\n\r");
    
    if(pos == 0)
        Print(L"#>REVISON NO:\t");
    else
        Print(L"REVISON NO:\t");
    printInt(conOut,buff[5]);
    Print(L".");
    printInt(conOut,buff[4]);
    Print(L"\n\r");
    
    if(pos == 1)
        Print(L"#>HEADER SIZE:\t");
    else
        Print(L"HEADER SIZE:\t");
    printInt(conOut,BE2int(buff+6,2));
    Print(L"\n\r");
    
    if(pos == 2)
        Print(L"#>HEADER CRC32:\t");
    else
        Print(L"HEADER CRC32:\t");
    printInt(conOut,BE2int(buff+8,2));
    Print(L"\n\r");
    
    if(pos == 3)
        Print(L"#>RESERVED:\t");
    else
        Print(L"RESERVED:\t");
    printInt(conOut,BE2int(buff+10,2));
    Print(L"\n\r");
    
    if(pos == 4)
        Print(L"#>MY LBA:\t");
    else
        Print(L"MY LBA:\t");
    printLong(conOut,BE2long(buff+12,4));
    Print(L"\n\r");
    
    if(pos == 5)
        Print(L"#>ALTERNATE LBA:\t");
    else
        Print(L"ALTERNATE LBA:\t");
    printLong(conOut,BE2long(buff+16,4));
    Print(L"\n\r");
    
    if(pos == 6)
        Print(L"#>FIRST USABLE LBA:\t");
    else
        Print(L"FIRST USABLE LBA:\t");
    printLong(conOut,BE2long(buff+20,4));
    Print(L"\n\r");

    if(pos == 7)
        Print(L"#>LAST USABLE LBA:\t");
    else
        Print(L"LAST USABLE LBA:\t");
    printLong(conOut,BE2long(buff+24,4));
    Print(L"\n\r");
    
    if(pos == 8)
        Print(L"#>DISK GUID:\t");
    else
        Print(L"DISK GUID:\t");
    printGUID(conOut,buff+28);
    Print(L"\n\r");
    
    if(pos == 9)
        Print(L"#>PARTITION ENTRY LBA:\t");
    else
        Print(L"PARTITION ENTRY LBA:\t");
    GPTLBA = (EFI_LBA)BE2long(buff+36,4);
    printLong(conOut,BE2long(buff+36,4));
    Print(L"\n\r");
    
    if(pos == 10)
        Print(L"#>NUMBER OF PARTITION ENTRIES:\t");
    else
        Print(L"NUMBER OF PARTITION ENTRIES:\t");
    GPTEntryCount = BE2int(buff+40,2);
    printInt(conOut,BE2int(buff+40,2));
    Print(L"\n\r");
    
    if(pos == 11)
        Print(L"#>SIZE OF PARTITION ENTRY:\t");
    else
        Print(L"SIZE OF PARTITION ENTRY:\t");
    GPTEntrySize = BE2int(buff+42,2);
    printInt(conOut,GPTEntrySize);
    Print(L"\n\r");
    
    if(pos == 12)
        Print(L"#>PARTITION ENTRY ARRAY CRC32:\t");
    else
        Print(L"PARTITION ENTRY ARRAY CRC32:\t");
    printInt(conOut,BE2int(buff+44,2));
    Print(L"\n\r");
}

void printGPTEntry(int entryNo)
{
    if(entryNo/(BUFFER_SIZE/GPTEntrySize)+GPTLBA != currLBA)
    {
        blockIOProtocol->ReadBlocks(blockIOProtocol,blockIOProtocol->Media->MediaId,entryNo/(BUFFER_SIZE/GPTEntrySize)+GPTLBA,BUFFER_SIZE,&buff);
    }
    int offset = entryNo%(BUFFER_SIZE/GPTEntrySize)*GPTEntrySize/2;
    Print(L"==========GPT ENTRY no: ");
    printInt(conOut,entryNo);
    Print(L"==========\n\r");
    
    Print(L"Partition Type GUID:\t");
    printGUID(conOut,buff+offset);
    Print(L"\n\r");
    
    Print(L"Unique Partition GUID:\t");
    printGUID(conOut,buff+8+offset);
    Print(L"\n\r");
    
    Print(L"Starting LBA:\t");
    printLong(conOut,BE2long(buff+16+offset,4));
    Print(L"\n\r");
    
    Print(L"Ending LBA:\t");
    printLong(conOut,BE2long(buff+20+offset,4));
    Print(L"\n\r");
    
    Print(L"Partition Name:\t");
    Print(buff+28+offset);
    Print(L"\n\r");
}

void printGPTEntryEdit(int entryNo, char pos)
{
    if(entryNo/(BUFFER_SIZE/GPTEntrySize)+GPTLBA != currLBA)
    {
        blockIOProtocol->ReadBlocks(blockIOProtocol,blockIOProtocol->Media->MediaId,entryNo/(BUFFER_SIZE/GPTEntrySize)+GPTLBA,BUFFER_SIZE,&buff);
    }
    int offset = entryNo%(BUFFER_SIZE/GPTEntrySize)*GPTEntrySize/2;
    Print(L"==========GPT ENTRY no: ");
    printInt(conOut,entryNo);
    Print(L"==========\n\r");
    
    if(pos == 0)
        Print(L"#>Partition Type GUID:\t");
    else
        Print(L"Partition Type GUID:\t");
    printGUID(conOut,buff+offset);
    Print(L"\n\r");
    
    if(pos == 1)
        Print(L"#>Unique Partition GUID:\t");
    else
        Print(L"Unique Partition GUID:\t");
    printGUID(conOut,buff+8+offset);
    Print(L"\n\r");
    
    if(pos == 2)
        Print(L"#>Starting LBA:\t");
    else
        Print(L"Starting LBA:\t");
    printLong(conOut,BE2long(buff+16+offset,4));
    Print(L"\n\r");
    
    if(pos == 3)
        Print(L"#>Ending LBA:\t");
    else
        Print(L"Ending LBA:\t");
    printLong(conOut,BE2long(buff+20+offset,4));
    Print(L"\n\r");
    
    //attributes
    
    //name
    if(pos == 4)
        Print(L"#>Partition Name:\t");
    else
        Print(L"Partition Name:\t");
    Print(buff+28+offset);
    Print(L"\n\r");
}

void printHelp()
{
    Print(L"\n\re - edit mode\n\r");
    Print(L"q - back/exit\n\r");
    Print(L"t - GPT entrys mode\n\r");
    Print(L"Enter - accept string (in input mode)\n\r");
    Print(L"Esc - refuse string (in input mode)\n\r");
    Print(L"Arrow up/arrow down - nawigate in edit mode\n\r");
    Print(L"Arrow left/arrow right - nawigate in edit mode\n\r");
    Print(L"h - show help\n\r");
}

////////////////////////////////////////
//////////////// EDITS /////////////////
////////////////////////////////////////

void editGPTEntryPos(int entryNo, char pos,EFI_SYSTEM_TABLE *systemTable)
{
    int offset = entryNo%(BUFFER_SIZE/GPTEntrySize)*GPTEntrySize/2;

    char status;
    Print(L"VALUE >>>\t");
    
    if(pos == 0)
    {
        CHAR16 text[37];
        unsigned int tmp;
        if(getText(text,37,systemTable))
        {
            status = 1;
            tmp = arr16_2int(text,8);
            buff[offset+1] = tmp >> 16;
            buff[offset] = tmp & 0x0000ffff;
            tmp = arr16_2int(text+9,4);
            buff[offset+2] = tmp & 0x0000ffff;
            tmp = arr16_2int(text+14,4);
            buff[offset+3] = tmp & 0x0000ffff;
            tmp = arr16_2int(text+19,4);
            buff[offset+4] = (tmp<<8) | (tmp>>8);
            tmp = arr16_2int(text+24,4);
            buff[offset+5] = (tmp<<8) | (tmp>>8);
            tmp = arr16_2int(text+28,4);
            buff[offset+6] = (tmp<<8) | (tmp>>8);
            tmp = arr16_2int(text+32,4);
            buff[offset+7] = (tmp<<8) | (tmp>>8);
        }
        else
            status = 0;
    }
    if(pos == 1)
    {
        CHAR16 text[37];
        unsigned int tmp;
        if(getText(text,37,systemTable))
        {
            status = 1;
            tmp = arr16_2int(text,8);
            buff[offset+9] = tmp >> 16;
            buff[offset+8] = tmp & 0x0000ffff;
            tmp = arr16_2int(text+9,4);
            buff[offset+10] = tmp & 0x0000ffff;
            tmp = arr16_2int(text+14,4);
            buff[offset+11] = tmp & 0x0000ffff;
            tmp = arr16_2int(text+19,4);
            buff[offset+12] = (tmp<<8) | (tmp>>8);
            tmp = arr16_2int(text+24,4);
            buff[offset+13] = (tmp<<8) | (tmp>>8);
            tmp = arr16_2int(text+28,4);
            buff[offset+14] = (tmp<<8) | (tmp>>8);
            tmp = arr16_2int(text+32,4);
            buff[offset+15] = (tmp<<8) | (tmp>>8);
        }
        else
            status = 0;
    }
    if(pos == 2)
    {
        int size;
        EFI_LBA tmp;
        CHAR16 text[20];
        
        if(getText(text,20,systemTable))
        {
            status = 1;
            for(size=0;text[size] != 0x0000;size++);  
            tmp = arr2lba(text,size);
            buff[offset+16] = tmp & 0x000000000000ffff;
            buff[offset+17] = (tmp >> 16)& 0x000000000000ffff;
            buff[offset+18] = (tmp >> 32)& 0x000000000000ffff;
            buff[offset+19] = tmp >> 48;
        }
        else
            status = 0;
    }
    if(pos == 3)
    {
        int size;
        EFI_LBA tmp;
        CHAR16 text[20];
        
        if(getText(text,20,systemTable))
        {
            status = 1;
            for(size=0;text[size] != 0x0000;size++);  
            tmp = arr2lba(text,size);
            buff[offset+20] = tmp & 0x000000000000ffff;
            buff[offset+21] = (tmp >> 16)& 0x000000000000ffff;
            buff[offset+22] = (tmp >> 32)& 0x000000000000ffff;
            buff[offset+23] = tmp >> 48;
        }
        else
            status = 0;
    }
     if(pos == 4)
    {
        CHAR16 text[36];
        if(getText(text,35,systemTable))
        {
            text[35] = 0x0000;
            status = 1;
            strcpy16(text, buff+offset+28);
        }
        else
            status = 0;
    }
    if(status)
    {
        CHAR16 buffHeader[BUFFER_SIZE];
        CHAR16 buffGPT[GPTEntrySize*GPTEntrySize];
        
        blockIOProtocol->WriteBlocks(blockIOProtocol,blockIOProtocol->Media->MediaId,entryNo/(BUFFER_SIZE/GPTEntrySize)+GPTLBA,BUFFER_SIZE,&buff);
        blockIOProtocol->WriteBlocks(blockIOProtocol,blockIOProtocol->Media->MediaId,GPTHeader2 - GPTEntryCount/(BUFFER_SIZE/GPTEntrySize) + entryNo/(BUFFER_SIZE/GPTEntrySize),BUFFER_SIZE,&buff);
        
        blockIOProtocol->FlushBlocks(blockIOProtocol);
        blockIOProtocol->ReadBlocks(blockIOProtocol,blockIOProtocol->Media->MediaId,GPTLBA,GPTEntrySize*GPTEntrySize,&buffGPT);
        
        blockIOProtocol->ReadBlocks(blockIOProtocol,blockIOProtocol->Media->MediaId,1,BUFFER_SIZE,&buffHeader);
        
        UINT32 crc = xcrc32(buffGPT,GPTEntrySize*GPTEntrySize);
        
        buffHeader[45] = crc>>16;
        buffHeader[44] = crc&0x0000ffff;
        
        //header crc32
        buffHeader[8] = 0;
        buffHeader[9] = 0;
        crc = xcrc32(buffHeader,GPTHeaderSize);
        buffHeader[9] = crc>>16;
        buffHeader[8] = crc&0x0000ffff;
        blockIOProtocol->WriteBlocks(blockIOProtocol,blockIOProtocol->Media->MediaId,1,BUFFER_SIZE,&buffHeader);
        blockIOProtocol->WriteBlocks(blockIOProtocol,blockIOProtocol->Media->MediaId,GPTHeader2,BUFFER_SIZE,&buffHeader);
        blockIOProtocol->FlushBlocks(blockIOProtocol);
    }
    
}

void editGPTHeaderPos(char pos,EFI_SYSTEM_TABLE *systemTable)
{
    char status;
    Print(L"VALUE >>>\t");
    if(pos == 0)
    {
        int high, low;
        CHAR16 text[8];
        
        if(getText(text,8,systemTable))
        {
            status = 1;
            for(high=0;text[high] != (CHAR16)'.';high++);
            for(low=0;text[high+low+1] != 0x0000;low++);        
            buff[5] = arr2int(text,high);
            buff[4] = arr2int(text+high+1,low);
        }
        else
            status = 0;
    }
    
    if(pos == 1)
    {
        int size;
        CHAR16 text[11];
        
        if(getText(text,11,systemTable))
        {
            status = 1;
            for(size=0;text[size] != 0x0000;size++);  
            GPTHeaderSize = arr2int(text,size);
            buff[7] = GPTHeaderSize & 0x0000ffff;
            buff[6] = GPTHeaderSize >> 16;
        }
        else
            status = 0;
    }
    
    if(pos == 2)
    {
        int size;
        CHAR16 text[11];
        
        if(getText(text,11,systemTable))
        {
            status = 0;
            for(size=0;text[size] != 0x0000;size++);  
            int crc = arr2int(text,size);
            buff[9] = crc>>16;
            buff[8] = crc&0x0000ffff;
            blockIOProtocol->WriteBlocks(blockIOProtocol,blockIOProtocol->Media->MediaId,currLBA,BUFFER_SIZE,&buff);
            blockIOProtocol->WriteBlocks(blockIOProtocol,blockIOProtocol->Media->MediaId,GPTHeader2,BUFFER_SIZE,&buff);
            blockIOProtocol->FlushBlocks(blockIOProtocol);
        }
        else
            status = 0;
    }
    
    if(pos == 3)
    {
        int size;
        int tmp;
        CHAR16 text[11];
        
        if(getText(text,11,systemTable))
        {
            status = 1;
            for(size=0;text[size] != 0x0000;size++);  
            tmp = arr2int(text,size);
            buff[10] = tmp & 0x0000ffff;
            buff[11] = tmp >> 16;
        }
        else
            status = 0;
    }
    
    if(pos == 4)
    {
        int size;
        EFI_LBA tmp;
        CHAR16 text[20];
        
        if(getText(text,20,systemTable))
        {
            status = 1;
            for(size=0;text[size] != 0x0000;size++);  
            tmp = arr2lba(text,size);
            buff[12] = tmp & 0x000000000000ffff;
            buff[13] = (tmp >> 16)& 0x000000000000ffff;
            buff[14] = (tmp >> 32)& 0x000000000000ffff;
            buff[15] = tmp >> 48;
        }
        else
            status = 0;
    }
    if(pos == 5)
    {
        int size;
        EFI_LBA tmp;
        CHAR16 text[20];
        
        if(getText(text,20,systemTable))
        {
            status = 1;
            for(size=0;text[size] != 0x0000;size++);  
            tmp = arr2lba(text,size);
            buff[16] = tmp & 0x000000000000ffff;
            buff[17] = (tmp >> 16)& 0x000000000000ffff;
            buff[18] = (tmp >> 32)& 0x000000000000ffff;
            buff[19] = tmp >> 48;
        }
        else
            status = 0;
    }
    if(pos == 6)
    {
        int size;
        EFI_LBA tmp;
        CHAR16 text[20];
        
        if(getText(text,20,systemTable))
        {
            status = 1;
            for(size=0;text[size] != 0x0000;size++);  
            tmp = arr2lba(text,size);
            buff[20] = tmp & 0x000000000000ffff;
            buff[21] = (tmp >> 16)& 0x000000000000ffff;
            buff[22] = (tmp >> 32)& 0x000000000000ffff;
            buff[23] = tmp >> 48;
        }
        else
            status = 0;
    }

    if(pos == 7)
    {
        int size;
        EFI_LBA tmp;
        CHAR16 text[20];
        
        if(getText(text,20,systemTable))
        {
            status = 1;
            for(size=0;text[size] != 0x0000;size++);  
            tmp = arr2lba(text,size);
            buff[24] = tmp & 0x000000000000ffff;
            buff[25] = (tmp >> 16)& 0x000000000000ffff;
            buff[26] = (tmp >> 32)& 0x000000000000ffff;
            buff[27] = tmp >> 48;
        }
        else
            status = 0;
    }
    
    if(pos == 8)
    {
        CHAR16 text[37];
        unsigned int tmp;
        if(getText(text,37,systemTable))
        {
            status = 1;
            tmp = arr16_2int(text,8);
            buff[29] = tmp >> 16;
            buff[28] = tmp & 0x0000ffff;
            tmp = arr16_2int(text+9,4);
            buff[30] = tmp & 0x0000ffff;
            tmp = arr16_2int(text+14,4);
            buff[31] = tmp & 0x0000ffff;
            tmp = arr16_2int(text+19,4);
            buff[32] = (tmp<<8) | (tmp>>8);
            tmp = arr16_2int(text+24,4);
            buff[33] = (tmp<<8) | (tmp>>8);
            tmp = arr16_2int(text+28,4);
            buff[34] = (tmp<<8) | (tmp>>8);
            tmp = arr16_2int(text+32,4);
            buff[35] = (tmp<<8) | (tmp>>8);
        }
        else
            status = 0;
    }
    
    if(pos == 9)
    {
        int size;
        EFI_LBA tmp;
        CHAR16 text[20];
        
        if(getText(text,20,systemTable))
        {
            status = 1;
            for(size=0;text[size] != 0x0000;size++);  
            tmp = arr2lba(text,size);
            buff[36] = tmp & 0x000000000000ffff;
            buff[37] = (tmp >> 16)& 0x000000000000ffff;
            buff[38] = (tmp >> 32)& 0x000000000000ffff;
            buff[39] = tmp >> 48;
        }
        else
            status = 0;
    }
    
    if(pos == 10)
    {
        int size;
        int tmp;
        CHAR16 text[11];
        
        if(getText(text,11,systemTable))
        {
            status = 1;
            for(size=0;text[size] != 0x0000;size++);  
            tmp = arr2int(text,size);
            buff[40] = tmp & 0x0000ffff;
            buff[41] = tmp >> 16;
        }
        else
            status = 0;
    }
    
    if(pos == 11)
    {
        int size;
        int tmp;
        CHAR16 text[11];
        
        if(getText(text,11,systemTable))
        {
            status = 1;
            for(size=0;text[size] != 0x0000;size++);  
            tmp = arr2int(text,size);
            buff[42] = tmp & 0x0000ffff;
            buff[43] = tmp >> 16;
        }
        else
            status = 0;
    }
    
    if(pos == 12)
    {
        int size;
        CHAR16 text[11];
        
        if(getText(text,11,systemTable))
        {
            status = 1;
            for(size=0;text[size] != 0x0000;size++);  
            int crc = arr2int(text,size);
            buff[45] = crc>>16;
            buff[44] = crc&0x0000ffff;
        }
        else
            status = 0;
    }
    if(status)
    {
        buff[8] = 0;
        buff[9] = 0;
        UINT32 crc = xcrc32(buff,GPTHeaderSize);
        buff[9] = crc>>16;
        buff[8] = crc&0x0000ffff;
        blockIOProtocol->WriteBlocks(blockIOProtocol,blockIOProtocol->Media->MediaId,currLBA,BUFFER_SIZE,&buff);
        blockIOProtocol->WriteBlocks(blockIOProtocol,blockIOProtocol->Media->MediaId,GPTHeader2,BUFFER_SIZE,&buff);
        blockIOProtocol->FlushBlocks(blockIOProtocol);
    }
}


/**
 * efi_main - The entry point for the EFI application
 * @image: firmware-allocated handle that identifies the image
 * @SystemTable: EFI system table
 */



EFI_STATUS
EFIAPI
UefiMain(EFI_HANDLE image, EFI_SYSTEM_TABLE *systemTable)
{
	EFI_BOOT_SERVICES *bs = systemTable->BootServices;

	conOut = systemTable->ConOut;
    EFI_HANDLE handles[100];
    EFI_DEVICE_PATH *devicePath;
    
    UINTN bufferSize = 100 * sizeof(EFI_HANDLE);
    int i, noOfHandles;

    EFI_STATUS status = bs->LocateHandle( 
        ByProtocol, 
        &BlockIoProtocolGUID, 
        NULL, /* Ignored for AllHandles or ByProtocol */
        &bufferSize, handles);

    noOfHandles = bufferSize == 0 ? 0 : bufferSize / sizeof(EFI_HANDLE);
    
    //gpt reader vars
    currLBA = 1;
    
    //seeking for disc with gpt
    if (EFI_ERROR(status)) {
        conOut->OutputString(conOut, L"Failed to LocateHandles!\r\n");
        return status;
    }
    for (i = 0; i < noOfHandles; i++) 
    {
        status = bs->HandleProtocol(handles[i], &DevicePathGUID, (void *) &devicePath);
        if (EFI_ERROR(status) || devicePath == NULL) 
        {
        continue;
        }
        status = bs->HandleProtocol(handles[i], &BlockIoProtocolGUID, (void *) &blockIOProtocol);
        if (EFI_ERROR(status) || blockIOProtocol == NULL) {
        continue;
        }

        if(blockIOProtocol->Media != NULL)
        {
            EFI_STATUS s = blockIOProtocol->ReadBlocks(blockIOProtocol,blockIOProtocol->Media->MediaId,currLBA,BUFFER_SIZE,&buff);
            if(s == EFI_SUCCESS)
            {
                buff2[0] = buff[0];
                buff2[1] = buff[1];
                buff2[2] = buff[2];
                buff2[3] = buff[3];
                Print(L"SUCCESS\n\r");
                if(!strcmp16(EFI_SIGNATURE,buff2))
                {
                    Print(L"GPT FOUND!\n\r");
                    break;
                }
            }
            else if (s == EFI_DEVICE_ERROR)
                Print(L"\n\rDEVICE ERROR\n\r");
            else if (s == EFI_MEDIA_CHANGED)
                Print(L"\n\rMEDIA CHANGED\n\r");
            else if (s == EFI_NO_MEDIA)
                Print(L"\n\rNO_MEDIA\n\r");
            else if (s == EFI_BAD_BUFFER_SIZE)
                Print(L"\n\rBAD BUFFER SIZE\n\r");
            else if (s == EFI_INVALID_PARAMETER)
                Print(L"\n\rINVALID PARAMETER\n\r");
        }
    }
    
    //app vars
	UINTN index;
    int entryNo;
    EFI_INPUT_KEY* key;
	EFI_EVENT event = systemTable->ConIn->WaitForKey;
    char GPTViewerHandler = 1;
    char menu = 0;
    char pos = 0;
    
    systemTable->ConOut->ClearScreen(systemTable->ConOut);
    printHelp();
    //main app's loop
    while(GPTViewerHandler)
    {
        //menu switch
        switch(menu)
        {       
            case 0:
                printGPTHeader();
                break;
            case 1:
                printGPTHeaderEdit(pos);
                break;
            case 2:
                printGPTEntry(entryNo);
                break;
            case 3:
                printGPTEntryEdit(entryNo, pos);
                break;
            case 4:
                printGPTHeaderEdit(pos);
                editGPTHeaderPos(pos,systemTable);
                menu = 1;
                systemTable->ConOut->ClearScreen(systemTable->ConOut);
                continue;
                break;
            case 5:
                printGPTEntryEdit(entryNo,pos);
                editGPTEntryPos(entryNo,pos,systemTable);
                menu = 3;
                systemTable->ConOut->ClearScreen(systemTable->ConOut);
                continue;
                break;
            default:
                break;
        }
        
        //getting key
        systemTable->BootServices->WaitForEvent(1, &event, &index);
        systemTable->ConIn->ReadKeyStroke(systemTable->ConIn,key);
        systemTable->ConOut->ClearScreen(systemTable->ConOut);
        
        
        if(key->UnicodeChar == (CHAR16)'e' && menu == 0)
        {
            menu = 1;
            pos = 0;
        }
         else if(key->UnicodeChar == (CHAR16)'q' && menu == 0)
        {
            GPTViewerHandler = 0;
        }
        else if(key->UnicodeChar == (CHAR16)'t' && menu==0)
        {
            entryNo=0;
            menu = 2;
        }
        
        else if(key->UnicodeChar == (CHAR16)'e' && menu == 1)
        {
            menu = 4;
        }
        else if(key->UnicodeChar == (CHAR16)'q' && menu == 1)
        {
            menu = 0;
        }
        else if(key->ScanCode == (UINT16)0x02 && menu==1)
        {
            if(pos == 12)
                pos=0;
            else
                pos++;
        }
        else if(key->ScanCode == (UINT16)0x01 && menu==1)
        {
            if(pos == 0)
                pos=12;
            else
                pos--;
        }
        
        else if(key->UnicodeChar == (CHAR16)'e' && menu == 2)
        {
            menu = 3;
            pos = 0;
        }
        else if(key->UnicodeChar == (CHAR16)'q' && menu == 2)
        {
            currLBA=1;
            blockIOProtocol->ReadBlocks(blockIOProtocol,blockIOProtocol->Media->MediaId,currLBA,BUFFER_SIZE,&buff);
            menu = 0;
        }
        else if(key->ScanCode == (UINT16)0x03 && menu==2)
        {
            if(entryNo == GPTEntryCount-1)
                entryNo=0;
            else
                entryNo++;
        }
        else if(key->ScanCode == (UINT16)0x04 && menu==2)
        {
            if(entryNo == 0)
                entryNo=GPTEntryCount-1;
            else
                entryNo--;
        }
        
        else if(key->UnicodeChar == (CHAR16)'e' && menu == 3)
        {
            menu = 5;
        }
        
        else if(key->UnicodeChar == (CHAR16)'q' && menu == 3)
        {
            menu = 2;
        }
                else if(key->ScanCode == (UINT16)0x02 && menu==3)
        {
            if(pos == 4)
                pos=0;
            else
                pos++;
        }
        else if(key->ScanCode == (UINT16)0x01 && menu==3)
        {
            if(pos == 0)
                pos=4;
            else
                pos--;
        }
        
        if(key->UnicodeChar == (CHAR16)'h')
        {
            printHelp();
        }
    }
    return EFI_SUCCESS;
}
