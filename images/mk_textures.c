/*
 * mk_textures.c - build the texture arrays
 * 04-12-20 E. Brombaugh
 */

#include <stdio.h>
#include <stdlib.h>

#define NUM_FILES 7

char *files[] = 
{
	"ericb_avatar.png",
	"brick_bw.png",
	"wood.png",
	"rocks.png",
	"lichen.png",
	"moss.png",
	"stonewall.png",
};

int main(int argc, char **argv)
{
	int i, j, k;
	unsigned short word;
	unsigned char buffer[2];
	char command[256];
	FILE *infile, *outfile;
	
	if(!(outfile = fopen("textures.h", "w")))
	{
		fprintf(stderr, "Can't open output file\n");
		exit(-1);
	}
	
	fprintf(outfile, "// textures.h - image textures\n");
	fprintf(outfile, "const uint16_t texture[%d][4096] = {\n", NUM_FILES);
	
	for(i=0;i<NUM_FILES;i++)
	{
		sprintf(command, "ffmpeg -vcodec png -i %s -vcodec rawvideo -f rawvideo -pix_fmt rgb565 temp.565", files[i]);
		system(command);
		
		if(!(infile = fopen("temp.565", "rb")))
		{
			fprintf(stderr, "Can't open input file\n");
			exit(-1);
		}
		
		fprintf(outfile, "\t// %s\n\t{\n", files[i]);
	
		for(j=0;j<4096;j+=8)
		{
			fprintf(outfile, "\t\t");
			for(k=0;k<8;k++)
			{
				fread(buffer, sizeof(char), 2, infile);
				word = buffer[0]<<8 | buffer[1];
				fprintf(outfile, "0x%04X, ", word);
			}
			fprintf(outfile, "\n");
		}
		fprintf(outfile, "\t},\n");
		fclose(infile);
		system("rm temp.565");
	}

	fprintf(outfile, "};\n");
	fclose(outfile);
	
	return 0;
}