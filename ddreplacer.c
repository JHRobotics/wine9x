#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>

const char defname[] = "ddraw.dll";
const uint8_t search[] = {
	0x44, 0x44, 0x52, 0x41, 0x57, 0x31, 0x36, 0x2E, 0x44, 0x4C, 0x4C, 0x00, 0x44, 0x44, 0x52, 0x41, // ; DDRAW16.DLL.DDRA
	0x57, 0x2E, 0x44, 0x4C, 0x4C, 0x00                                                              // ; W.DLL.
};

size_t replace_pos = 12;

int main(int argc, char **argv)
{
	int r = -1;
	int found = 0;
	
	if(argc < 3)
	{
		printf("Usage %s <path to ddraw.dll> <new name>\n", argv[0]);
		return 0;
	}
	
	const char *src = argv[1];
	const char *dest = argv[2];
	
	if(strlen(dest) != strlen(defname))
	{
		fprintf(stderr, "the length of name to replace must match original name, %u (%s) != %u (%s)\n",
			strlen(dest), dest, strlen(defname), defname);
		return 1;
	}
	
	FILE *fr = fopen(src, "rb");
	if(fr != NULL)
	{
		size_t fs = 0;
		uint8_t *mem = NULL;
		
		fseek(fr, 0, SEEK_END);
		fs = ftell(fr);
		fseek(fr, 0, SEEK_SET);
		
		mem = (uint8_t*)malloc(fs);
		if(mem != NULL)
		{
			if(fread(mem, 1, fs, fr) == fs)
			{
				fclose(fr); // for case, we replacing file
				fr = NULL;
												
				size_t i, j;
				
				for(i = 0; i < fs - sizeof(search); i++)
				{
					if(memcmp(mem+i, search, sizeof(search)) == 0)
					{
						for(j = 0; j < sizeof(defname)-1; j++)
						{
							mem[i+replace_pos+j] = toupper(dest[j]);
						}
						found++;
					}
				}
				
				if(found > 0)
				{
					FILE *fw = fopen(dest, "wb");
					if(fw)
					{
						if(fwrite(mem, 1, fs, fw) == fs)
						{
							r = 0;
						}	else fprintf(stderr, "Write file failure");
						
						fclose(fw);
					} else fprintf(stderr, "Failed open %s for write\n", dest);
				} else fprintf(stderr, "String to replace not found! (wrong DLL, or file already patched)\n");
			} else fprintf(stderr, "Read file failure\n");
			
			free(mem);
		} else fprintf(stderr, "Failed to allocate memory (%u bytes)\n", fs);
		
		if(fr != NULL) fclose(fr);
	} else fprintf(stderr, "Failed open %s for read\n", src);
	
	return r;		
}

