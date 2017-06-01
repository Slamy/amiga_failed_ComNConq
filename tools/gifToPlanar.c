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
#define TRANSPARENT_PIXEL			255
//#define DEBUG

struct paletteEntry
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
	char* comment;

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
			//char comment[200];

			char *readPtr = buf;
			red = strtol(readPtr, &readPtr, 10);
			assert(readPtr);
			green = strtol(readPtr, &readPtr, 10);
			assert(readPtr);
			blue = strtol(readPtr, &readPtr, 10);
			assert(readPtr);
			while (*readPtr=='\t' || *readPtr==' ')
				readPtr++;

			//assert (sscanf(buf,"%d %d %d %s",&red,&green,&blue, &comment)!=EOF);
			//printf("%d %d %d %d\n",paletteEntrys,red,green,blue);
			//printf ("%s",readPtr);
			//assert(paletteEntrys<=MAX_PALETTE_SIZE);

			palette[paletteEntrys].r=red;
			palette[paletteEntrys].g=green;
			palette[paletteEntrys].b=blue;
			palette[paletteEntrys].comment=strdup(readPtr);

			paletteEntrys++;

		}
	}
	fclose(f);
}



//Gif nach Line Interleaved Planar
void gifToILBM(char *giffile, char *outfile, int bitplanes)
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
	int bytesPerLine = width/8;
	//int wordsPerLine = width/16;
	int bitmapSize = bytesPerLine * height * bitplanes;

	printf ("Breite: %d   Höhe: %d   Bitplanes: %d\n",width,height,bitplanes);

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
		//Reverse Engineered nötig
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


#define TILE_WIDTH	16
#define TILE_HEIGHT	16

//Gif nach Line Interleaved Planar
void gifTileMap16ToILBM(char *giffile, char *outfile, int bitplanes)
{
	int i,j,y,x;

	FILE *fb;
	GifFileType *GifFile;
	int error;

	GifFile=DGifOpenFileName(giffile,&error);
	assert (GifFile!=NULL);
	assert (DGifSlurp(GifFile)!=GIF_ERROR);


	int tileCols = GifFile->Image.Width/16;
	int tileRows = GifFile->Image.Height/16;

	assert((GifFile->Image.Width%16)==0); //Bündig auf Wortbreite
	assert((GifFile->Image.Height%16)==0); //Bündig auf Wortbreite

	int width=TILE_WIDTH;
	int height=TILE_HEIGHT;

	int bytesPerLine = width/8;
	//int wordsPerLine = width/16;
	int bitmapSize = bytesPerLine * height * bitplanes * tileCols * tileRows;

	printf ("Breite: %d   Höhe: %d   Bitplanes: %d\n",width,height,bitplanes);

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
		//Reverse Engineered nötig
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
	int ty,tx;

	for (ty=0; ty < tileRows; ty++)
	{
		for (tx=0; tx < tileCols; tx++)
		{
	for (y=0; y < height; y++)
	{

		gifPixel = (ty * TILE_HEIGHT + y) * GifFile->Image.Width + tx * TILE_WIDTH;
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
	}
	}
	fb=fopen(outfile,"wb");
	assert(fb!=NULL);
	fwrite(bitmap,1,bitmapSize,fb);
	fclose(fb);

	DGifCloseFile(GifFile,&error);
}

//Gif nach Line Interleaved Planar
void gifToMaskedAcbm(char *giffile, char *outfile, int bitplanes)
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
	int bytesPerLine = width/8;
	//int wordsPerLine = width/16;
	int bitmapSize = bytesPerLine * height * (bitplanes+1);
	int maxColors=1;

	for (i=0; i< bitplanes; i++)
	{
		maxColors*=2;
	}

	printf ("Breite: %d   Höhe: %d   Bitplanes: %d / %d\n",width,height,bitplanes, maxColors);

	assert((width%16)==0); //Bündig auf Wortbreite

	uint8_t *bitmap = malloc(bitmapSize);
	assert(bitmap);
	uint8_t *bitmapPlanePtr[10];
	uint8_t *maskPlanePtr;

	/*
	 * Ich gehe von Line Interleaved Bit Planar aus.
	 * Zeile 0, Plane 0
	 * Zeile 0, Plane 1
	 * Zeile 0, Plane 2
	 * Zeile 1, Plane 0
	 */
	int bitplaneByteModulo = 0;

	//Pointer auf die einzelnen Planes
	for (i=0;i < bitplanes; i++)
	{
		bitmapPlanePtr[i] = &bitmap[bytesPerLine*height*i];
	}
	maskPlanePtr = &bitmap[bytesPerLine*height*bitplanes];

	//wofür war das nochmal ?
	int transparent=-1;
	for (i=0;i<GifFile->SavedImages->ExtensionBlockCount;i++)
	{
		//Reverse Engineered nötig
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
					chunky[i]=TRANSPARENT_PIXEL;
				else
					chunky[i]=getIndexOfColor(GifFile->SColorMap->Colors[GifFile->SavedImages->RasterBits[gifPixel]]);

				if (chunky[i] >= maxColors)
					chunky[i] = TRANSPARENT_PIXEL; //keine erreichbare Farbe ist auch transparent...
				gifPixel++;
#ifdef DEBUG

				if (chunky[i]==TRANSPARENT_PIXEL)
				{
					int i;
					for (i=0 ; i< bitplanes; i++)
						printf("X");
				}
				else
				{
					printBin(chunky[i],bitplanes);
				}
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

			//Nun die Maske
			int chunkyMask = 1 << i;
			*maskPlanePtr=0; //Erst mal auf 0 setzen.

			int wordMask= 1<<(8-1);
			for (j = 0; j < 8 ; j++)
			{
				if (chunky[j] != TRANSPARENT_PIXEL)
					*maskPlanePtr |= wordMask;

				wordMask>>=1;
			}

#ifdef DEBUG
			printf(" MASK ");
			printBin(*maskPlanePtr,8);
			printf(" ");
#endif

			maskPlanePtr++;

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

void gimpPalette()
{
	int i;
	printf("GIMP Palette\n");
	printf("Name: Export\n");
	printf("#\n");
	for (i=0;i<paletteEntrys;i++)
	{
		unsigned int r = (palette[i].r << 4) | palette[i].r;
		unsigned int g = (palette[i].g << 4) | palette[i].g;
		unsigned int b = (palette[i].b << 4) | palette[i].b;

		printf("%d %d %d\t%s",r,g,b,palette[i].comment);
	}
}

int main(int argc,char **argv)
{
	assert(argc>=2);

	if (!strcmp(argv[1],"gifToILBM"))
	{
		assert(argc==6);
		readPaletteFile(argv[3]);
		gifToILBM(argv[2],argv[5],atoi(argv[4]));
	}
	else if (!strcmp(argv[1],"gifToSprite"))
	{
		assert(argc==5);
		readPaletteFile(argv[3]);
		gifToILBM(argv[2],argv[4],2);
	}
	else if (!strcmp(argv[1],"gifTileMap16ToILBM"))
	{
		assert(argc==6);
		readPaletteFile(argv[3]);
		gifTileMap16ToILBM(argv[2],argv[5],atoi(argv[4]));
	}
	else if (!strcmp(argv[1],"gifToMaskedACBM"))
	{
		assert(argc==6);
		readPaletteFile(argv[3]);
		gifToMaskedAcbm(argv[2],argv[5],atoi(argv[4]));
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
	else if (!strcmp(argv[1],"gimpPalette"))
	{
		assert(argc==3);
		readPaletteFile(argv[2]);
		gimpPalette();
	}
	else
	{
		return 1;
	}


	return 0;
}



