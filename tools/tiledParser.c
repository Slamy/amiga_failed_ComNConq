#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

struct enemyStart
{
	int x,y,type;
};

struct enemyStart *enemies[100];
int anz=0;
int i,o;

void main(int argc, char **argv)
{
	assert(argc==3);

	FILE *in=fopen(argv[1],"r");
	FILE *out=fopen(argv[2],"w");
	char buffer[1024];
	
	for(;;)
	{
		if (!fgets(buffer,1024,in))
			break;
		
		if (strstr(buffer,"<data encoding=\"csv\">"))
		{
			fprintf(out,"const unsigned char leveldata[]={\n");
			for(;;)
			{
				if (!fgets(buffer,1024,in))
				{
					printf("Tragisch 1 !\n");
					exit(1);
				}
				
				if (strstr(buffer,"</data>"))
				{
					fprintf(out,"	};\n\n");
					break;
				}
				else
				{
					fprintf(out,"	%s",buffer);
				}
			}
		}
		
		if (strstr(buffer," <objectgroup"))
		{
			
			for(;;)
			{
				if (!fgets(buffer,1024,in))
				{
					printf("Tragisch 2 !\n");
					exit(1);
				}
				
				if (strstr(buffer,"</objectgroup>"))
				{
					//fprintf(out,"	};\n\n");
					break;
				}
				else
				{
					int type=0,x=0,y=0;
					if (sscanf(buffer,"  <object id=\"%*d\" gid=\"%d\" x=\"%d\" y=\"%d\"/>",&type,&x,&y)<3)
					{
						printf("Konnte <object> nicht parsen :%s\n",buffer);
						exit(1);
					}
					
					//printf("%d %d %d\n",type,x,y);
					enemies[anz]=malloc(sizeof(struct enemyStart));
					enemies[anz]->x=x-320;
					enemies[anz]->y=y;
					enemies[anz]->type=type;
					anz++;
					
					//fprintf(out,"	%d,%d,0\n",x,y);
					//fprintf(out,"	%s",buffer);
				}
				
			}
			
			for (o=0;o<anz;o++)
			{
				for (i=0;i<anz-1;i++)
				{
					if (enemies[i]->x > enemies[i+1]->x)
					{
						struct enemyStart *swap;
						
						swap=enemies[i+1];
						enemies[i+1]=enemies[i];
						enemies[i]=swap;
					}
				}
			}
			
			fprintf(out,"const short enemyStart[]={\n");
			
			for (i=0;i<anz;i++)
			{
				int writeType;
				switch (enemies[i]->type)
				{
					case 6: writeType=0; break;
					case 11: writeType=4; break;
					case 10: writeType=6; break;
					default:
						assert(0);
				}
				
				fprintf(out,"	%d,%d,%d,\n",enemies[i]->x,enemies[i]->y,writeType);
			}
			
			
			fprintf(out,"	-1};\n\n");
		}
	}
	
	
	fclose(in);
	fclose(out);
	
}
