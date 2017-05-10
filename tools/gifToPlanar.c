/*
 * gifToPlanar.c
 *
 *  Created on: 23.04.2017
 *      Author: andre
 */

#include <gif_lib.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>

#define	MAX_PALETTE_SIZE			32


struct paletteEntry
{
	uint8_t r;
	uint8_t g;
	uint8_t b;

} palette[MAX_PALETTE_SIZE];

int paletteEntrys=0;

void printBin(uint32_t val, int anz)
{
	int i;
	int mask = 1 << (anz-1);
	for (i=0; i< anz ; i++)
	{
		if (mask&val)
			printf("1");
		else
			printf("0");
		val<<=1;
	}
}


int getIndexOfColor(GifColorType col)
{
	int i;
	for (i=0;i<paletteEntrys;i++)
	{
		if ((col.Red>>4)	== palette[i].r &&
			(col.Green>>4	== palette[i].g &&
			(col.Blue>>4)	== palette[i].b))
			return i;
	}

	printf ("Farbe %d %d %d nicht in Palette enthalten !\n",(col.Red>>4),(col.Green>>4),(col.Blue>>4));
	exit(1);
	return 0;
}

void readPaletteFile (char *path)
{
	FILE *f=fopen(path,"r");
	char buf[200];
	assert(f!=NULL);

	for (;;)
	{
		if (fgets(buf,200,f)==NULL)
			return;

		if (buf[0]!='\n')
		{
			int red,green,blue;

			assert (sscanf(buf,"%d %d %d",&red,&green,&blue)!=EOF);

			//printf ("%d\n",index);
			assert(paletteEntrys<=MAX_PALETTE_SIZE);

			palette[paletteEntrys].r=red;
			palette[paletteEntrys].g=green;
			palette[paletteEntrys].b=blue;

			paletteEntrys++;

		}
	}
	fclose(f);
}

//Konvertiert 16 Chunky Pixels nach Planar
void chunkToPlanar(uint8_t *chunky, uint16_t *planar, int planarModulo)
{

}

//Gif nach Line Interleaved Planar
void GifToPlanarBitmap(char *giffile, char *outfile)
{
	int i,j,y,x;

	FILE *fb;
	GifFileType *GifFile;
	int error;

	GifFile=DGifOpenFileName(giffile,&error);
	assert (GifFile!=NULL);
	assert (DGifSlurp(GifFile)!=GIF_ERROR);


	int width=GifFile->Image.Width;
	int height=GifFile->Image.Height;
	int bitplanes=5; //TODO Erstmal fest 3 Bitplanes annehmen
	int bytesPerLine = width/8;
	//int wordsPerLine = width/16;
	int bitmapSize = bytesPerLine * height * bitplanes;

	printf ("Breite: %d   Höhe: %d\n",width,height);

	assert((width%16)==0); //Bündig auf Wortbreite

	uint8_t *bitmap = malloc(bitmapSize);
	assert(bitmap);
	uint8_t *bitmapPlanePtr[10];

	/*
	 * Ich gehe von Line Interleaved Bit Planar aus.
	 * Zeile 0, Plane 0
	 * Zeile 0, Plane 1
	 * Zeile 0, Plane 2
	 * Zeile 1, Plane 0
	 */
	int bitplaneByteModulo = (bitplanes-1) * bytesPerLine;

	//Pointer auf die einzelnen Planes
	for (i=0;i < bitplanes; i++)
		bitmapPlanePtr[i] = &bitmap[bytesPerLine*i];

	//wofür war das nochmal ?
	int transparent=-1;
	for (i=0;i<GifFile->SavedImages->ExtensionBlockCount;i++)
	{
		//Reverse Engineered FIXME
		if (GifFile->SavedImages->ExtensionBlocks[i].Function==249 && GifFile->SavedImages->ExtensionBlocks[i].ByteCount==4)
		{
			transparent=GifFile->SavedImages->ExtensionBlocks[i].Bytes[3];
			printf ("Transparent:%d\n",transparent);
		}
	}

	/*
	 * Es werden immer 8 Chunky-Pixel aus dem Gif geholt und dann nach Planar übersetzt.
	 * Der Amiga arbeitet Wort-orientiert. Aber da es eine Big Endianess Maschine ist, arbeiten wir besser
	 * mit Bytes, um in keine Endianness-Probleme zu geraten.
	 */

	int gifPixel=0;

	for (y=0; y < height; y++)
	{
		for (x=0; x < bytesPerLine; x++)
		{
			uint8_t chunky[8];

#ifdef DEBUG
			printf("Chunky: ");
#endif
			for (i = 0; i < 8; i++)
			{
				if (transparent==GifFile->SavedImages->RasterBits[gifPixel])
					chunky[i]=0;
				else
					chunky[i]=getIndexOfColor(GifFile->SColorMap->Colors[GifFile->SavedImages->RasterBits[gifPixel]]);
				gifPixel++;
#ifdef DEBUG
				printBin(chunky[i],3);
				printf(" ");
#endif
			}
#ifdef DEBUG
			printf("\n");

			printf("Planar: ");
#endif
			//Wir iterieren am besten über jedes Zielwort
			for (i=0; i < bitplanes; i++)
			{
				int chunkyMask = 1 << i;
				*bitmapPlanePtr[i]=0; //Erst mal auf 0 setzen.

				/*
				 * Wir haben uns also eine Bitplane ausgesucht, die wir generieren wollen.
				 * chunkyMask wählt das benötigte Bit aus Chunky Bytes aus.
				 * Wir iterieren nun über jedes Bit des Zielwortes bzw.
				 * über jedes Chunky Byte und prüfen, ob eine 1 gesetzt werden muss.
				 */

				int wordMask= 1<<(8-1);
				for (j = 0; j < 8 ; j++)
				{
					if (chunky[j] & chunkyMask)
						*bitmapPlanePtr[i] |= wordMask;

					wordMask>>=1;
				}

#ifdef DEBUG
				printBin(*bitmapPlanePtr[i],8);
				printf(" ");
#endif

				bitmapPlanePtr[i]++;
			}
#ifdef DEBUG
			printf("\n");
#endif
		}

		//Den Modulo auf alle Bitplane pointer anwenden, da aktuelle Zeile abgeschlossen
		for (i=0;i < bitplanes; i++)
			bitmapPlanePtr[i]+=bitplaneByteModulo;
	}

	fb=fopen(outfile,"wb");
	assert(fb!=NULL);
	fwrite(bitmap,1,bitmapSize,fb);
	fclose(fb);

	DGifCloseFile(GifFile,&error);
}



void gifToPalette(char *giffile)
{
	int i;

	GifColorType Color;

	GifFileType *GifFile;

	int error;
	GifFile=DGifOpenFileName(giffile,&error);
	assert (GifFile!=NULL);
	assert (DGifSlurp(GifFile)!=GIF_ERROR);

	int coloranz=GifFile->SColorMap->ColorCount;
	for (i=0;i<coloranz;i++)
	{
		Color=GifFile->SColorMap->Colors[i];
		printf ("%d %d %d\n",Color.Red>>4,Color.Green>>4,Color.Blue>>4);
	}
}

void amigaPalette()
{
	int i;
	for (i=0;i<paletteEntrys;i++)
	{
		unsigned int value = (palette[i].r << 8) | (palette[i].g << 4) | (palette[i].b);

		printf("custom.color[%d] = 0x%04x;\n",i,value);

	}
}


int main(int argc,char **argv)
{
	assert(argc>=2);

	if (!strcmp(argv[1],"gifToPlanar"))
	{
		assert(argc==5);
		readPaletteFile(argv[3]);
		GifToPlanarBitmap(argv[2],argv[4]);
	}
	else if (!strcmp(argv[1],"gifToPalette"))
	{
		assert(argc==3);
		gifToPalette(argv[2]);
	}
	else if (!strcmp(argv[1],"amigaPalette"))
	{
		assert(argc==3);
		readPaletteFile(argv[2]);
		amigaPalette();
	}


	return 0;
}



