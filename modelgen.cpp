#define WIN32_LEAN_AND_MEAN

#include<objbase.h>
#include<windows.h>
#include<windowsx.h>
#include<stdio.h>
#include<string.h>
#include<malloc.h>
#include<commdlg.h>

#define TRUE 1
#define FALSE 0

#define SF 3

void InitializeModelFile(char *filename,long numobj);
void EncodeBumpMap(char *filename,char mirrorflag);
void EncodeAnchoredBumpMap(char *filename,long num,long *anchors);
void BeginRender(void);
void EncodeTexture(char *filename);
void WriteStringToFile(FILE *file,char *string);
void CloseModelFile(void);
void OpenInputFile(char *filename);
void GetNextCommand(void);
void GetNextNumber(long *num);
void ParseIString(void);
void CloseInputFile(void);

FILE *modelfile;
FILE *inputfile;
char istring[256];
long objnum=0;
long *numanc;

int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nCmdShow)
{
	OPENFILENAME ofn;
	char filename[512];
	char filter[]={'3','-','D',' ','M','o','d','e','l',' ','G','e','n','e','r','a','t','o','r',' ','F','i','l','e','s',' ','(','*','.','g','e','n',')',0,'*','.','g','e','n',0,0};
	
	strcpy(filename,"");
	ZeroMemory(&ofn,sizeof(ofn));
	ofn.lStructSize=sizeof(ofn);
	ofn.lpstrFile=filename;
	ofn.nMaxFile=512;
	ofn.lpstrFilter=filter;
	ofn.nFilterIndex=1;
	ofn.lpstrTitle="Select File for Scripting";
	ofn.Flags=OFN_EXPLORER|
			  OFN_FILEMUSTEXIST|
			  OFN_HIDEREADONLY|
			  OFN_LONGNAMES;
	ofn.lpstrDefExt="gen";
	if(!GetOpenFileName(&ofn))
		return(0);
	
	char instr[256];
	char done=0;
	long num;

	OpenInputFile(filename);
	GetNextCommand();
	strcpy(instr,istring);
	while(!done)
	{
		if(!strcmp(instr,"INITIALIZE_FILE"))
		{
			GetNextCommand();
			ParseIString();
			strcpy(filename,istring);
			GetNextNumber(&num);
			InitializeModelFile(filename,num);
			GetNextCommand();
			strcpy(instr,istring);
			numanc=(long*)malloc(num*sizeof(long));
		}
		if(!strcmp(instr,"NORMAL_BUMP_MAP"))
		{
			GetNextCommand();
			if(istring[0]!='"')
				strcpy(instr,istring);
			else
			{
				ParseIString();
				strcpy(filename,istring);
				EncodeBumpMap(filename,FALSE);
			}
		}
		if(!strcmp(instr,"MIRROR_BUMP_MAP"))
		{
			GetNextCommand();
			if(istring[0]!='"')
				strcpy(instr,istring);
			else
			{
				ParseIString();
				strcpy(filename,istring);
				EncodeBumpMap(filename,TRUE);
			}
		}
		if(!strcmp(instr,"ANCHOR_BUMP_MAP"))
		{
			GetNextCommand();
			if(istring[0]!='"')
				strcpy(instr,istring);
			else
			{
				ParseIString();
				strcpy(filename,istring);
				long num;
				GetNextNumber(&num);
				long *anchors;
				anchors=(long*)malloc(num*3*sizeof(long));
				long count;
				for(count=0;count<num*3;count++)
					GetNextNumber(&anchors[count]);
				EncodeAnchoredBumpMap(filename,num,anchors);
				free(anchors);
			}
		}
		if(!strcmp(instr,"BEGIN_RENDER"))
		{
			BeginRender();
			GetNextCommand();
			strcpy(instr,istring);
		}
		if(!strcmp(instr,"WRITE_INSTRUCTION"))
		{
			GetNextCommand();
			if(istring[0]!='"')
				strcpy(instr,istring);
			else
			{
				ParseIString();
				WriteStringToFile(modelfile,istring);
			}
		}
		if(!strcmp(instr,"RENDER_TEXTURE"))
		{
			GetNextCommand();
			if(istring[0]!='"')
				strcpy(instr,istring);
			else
			{
				ParseIString();
				EncodeTexture(istring);
			}
		}
		if(!strcmp(instr,"CLOSE_FILE"))
		{
			CloseModelFile();
			done=1;
			free(numanc);
		}
	}
	CloseInputFile();
	return(0);
}

void InitializeModelFile(char *filename,long numobj)
{
	char string[256];
	
	modelfile=fopen(filename,"wb");	
	sprintf(string,"3-D Model Script: %s",filename);
	WriteStringToFile(modelfile,string);
	sprintf(string,"Number of objects: <%d>",numobj);
	WriteStringToFile(modelfile,string);
	WriteStringToFile(modelfile,"");
}

void EncodeBumpMap(char *filename,char mirrorflag)
{
	char string[256];
	long nv,vn,x,y;
	FILE *imagefile;
	unsigned char *imagedata;
	unsigned char *palettedata;
	char done;
	long count,count2;
	long face;
	
	numanc[objnum]=0;
	sprintf(string,"Object %d:",objnum++);
	WriteStringToFile(modelfile,string);
	imagefile=fopen(filename,"rb");
	for(count=0;count<128;count++)
		getc(imagefile);
	imagedata=(unsigned char*)malloc(64000);
	palettedata=(unsigned char*)malloc(768);
	count=0;
	done=FALSE;
	while(!done)
	{
		unsigned char data,run;

		data=getc(imagefile);
		if(data>192)
		{
			run=data-192;
			data=getc(imagefile);
			for(count2=0;count2<run;count2++)
				if(count<64000)
					imagedata[count++]=data;
		}
		else
			if(count<64000)
				imagedata[count++]=data;
		if(count>63999)
			done=TRUE;
	}
	fseek(imagefile,SEEK_END,-768);
	for(count=0;count<768;count++)
		palettedata[count]=getc(imagefile);
	nv=0;
	for(count=0;count<64000;count++)
		if(imagedata[count])
			nv++;
	sprintf(string,"  Number of vertices: <%d>",nv);
	WriteStringToFile(modelfile,string);
	WriteStringToFile(modelfile,"    Vertices:");
	vn=0;
	if(mirrorflag)
		face=-1;
	else
		face=1;
	for(y=0;y<200;y++)
		for(x=0;x<320;x++)
			if(imagedata[x+(y*320)])
			{
				sprintf(string,"        %d: <%d,%d,%d>",vn++,(x-160)*SF,(y-100)*SF,(imagedata[x+(y*320)]-1)*SF*face);
				WriteStringToFile(modelfile,string);
			}
	WriteStringToFile(modelfile,"");
	free(palettedata);
	free(imagedata);
	fclose(imagefile);
}

void EncodeAnchoredBumpMap(char *filename,long num,long *anchors)
{
	char string[256];
	long nv,vn,x,y;
	FILE *imagefile;
	unsigned char *imagedata;
	unsigned char *palettedata;
	char done;
	long count,count2;
	long face;
	
	numanc[objnum]=num;
	sprintf(string,"Object %d:",objnum++);
	WriteStringToFile(modelfile,string);
	imagefile=fopen(filename,"rb");
	for(count=0;count<128;count++)
		getc(imagefile);
	imagedata=(unsigned char*)malloc(64000);
	palettedata=(unsigned char*)malloc(768);
	count=0;
	done=FALSE;
	while(!done)
	{
		unsigned char data,run;

		data=getc(imagefile);
		if(data>192)
		{
			run=data-192;
			data=getc(imagefile);
			for(count2=0;count2<run;count2++)
				if(count<64000)
					imagedata[count++]=data;
		}
		else
			if(count<64000)
				imagedata[count++]=data;
		if(count>63999)
			done=TRUE;
	}
	fseek(imagefile,SEEK_END,-768);
	for(count=0;count<768;count++)
		palettedata[count]=getc(imagefile);
	nv=num;
	for(count=0;count<64000;count++)
		if(imagedata[count])
			nv++;
	sprintf(string,"  Number of vertices: <%d>",nv);
	WriteStringToFile(modelfile,string);
	sprintf(string,"    Anchors: %d",num);
	WriteStringToFile(modelfile,string);
	for(count=0;count<num;count++)
	{
		sprintf(string,"        %d: <%d,%d,%d>",count,anchors[(count*3)+0]*SF,anchors[(count*3)+1]*SF,anchors[(count*3)+2]*SF);
		WriteStringToFile(modelfile,string);
	}
	WriteStringToFile(modelfile,"    Vertices:");
	vn=num;
	face=1;
	for(y=0;y<200;y++)
		for(x=0;x<320;x++)
			if(imagedata[x+(y*320)])
			{
				sprintf(string,"        %d: <%d,%d,%d>",vn++,(x-160)*SF,(y-100)*SF,(imagedata[x+(y*320)]-1)*SF*face);
				WriteStringToFile(modelfile,string);
			}
	WriteStringToFile(modelfile,"");
	free(palettedata);
	free(imagedata);
	fclose(imagefile);
}

void BeginRender(void)
{
	WriteStringToFile(modelfile,"Pre-render instructions:");
	WriteStringToFile(modelfile,"clr: Set pallete entry");
	WriteStringToFile(modelfile,"");
	WriteStringToFile(modelfile,"<clr><1:255,255,255>");
	WriteStringToFile(modelfile,"");
	WriteStringToFile(modelfile,"Render instructions:");
	WriteStringToFile(modelfile,"render: Begin render");
	WriteStringToFile(modelfile,"yprw: Add object's ypr to world ypr");
	WriteStringToFile(modelfile,"trns: Transform points");
	WriteStringToFile(modelfile,"movw: Move object according to world coordinates");
	WriteStringToFile(modelfile,"line: Draw line from point1 to point2 with color 3");
	WriteStringToFile(modelfile,"ypr: Add object1's ypr to object2");
	WriteStringToFile(modelfile,"move: Move object to point");
	WriteStringToFile(modelfile,"roty: Rotate yaw");
	WriteStringToFile(modelfile,"rotp: Rotate pitch");
	WriteStringToFile(modelfile,"rotr: Rotate roll");
	WriteStringToFile(modelfile,"sety: Set yaw");
	WriteStringToFile(modelfile,"setp: Set pitch");
	WriteStringToFile(modelfile,"setr: Set roll");
	WriteStringToFile(modelfile,"poly: Draw polygon");
	WriteStringToFile(modelfile,"prgb: Draw polygon with RGB color");
	WriteStringToFile(modelfile,"pers: Add perspective");
	WriteStringToFile(modelfile,"key: Process commands only if key is pressed");
	WriteStringToFile(modelfile,"endkey: Process all commands");
	WriteStringToFile(modelfile,"end: End render");
	WriteStringToFile(modelfile,"");
	WriteStringToFile(modelfile,"<render>");
	WriteStringToFile(modelfile,"");
	objnum=0;
}

void EncodeTexture(char *filename)
{
	char string[256];
	static long objnum=0;
	FILE *imagefile;
	unsigned char *imagedata;
	unsigned char *palettedata;
	char done;
	long count,count2;
	long vn,x,y,x1,nv;

	imagefile=fopen(filename,"rb");
	for(count=0;count<128;count++)
		getc(imagefile);
	imagedata=(unsigned char*)malloc(64000);
	palettedata=(unsigned char*)malloc(768);
	count=0;
	done=FALSE;
	while(!done)
	{
		unsigned char data,run;

		data=getc(imagefile);
		if(data>192)
		{
			run=data-192;
			data=getc(imagefile);
			for(count2=0;count2<run;count2++)
				if(count<64000)
					imagedata[count++]=data;
		}
		else
			if(count<64000)
				imagedata[count++]=data;
		if(count>63999)
			done=TRUE;
	}
	getc(imagefile);
	fseek(imagefile,SEEK_END,-768);
	for(count=0;count<768;count++)
		palettedata[count]=getc(imagefile);
	vn=numanc[objnum];
	for(y=0;y<199;y++)
		for(x=0;x<319;x++)
		{
			long vhome;
			long veast;
			long vsouth;
			long vsoutheast;

			nv=0;
			vhome=vn;
			if(imagedata[(x+1)+(y*320)])
			{
				veast=vhome+1;
				nv++;
			}
			else
				veast=-1;
			if(imagedata[x+((y+1)*320)])
			{
				vsouth=vhome;
				for(x1=x+1;x1<320;x1++)
					if(imagedata[x1+(y*320)])
						vsouth++;
				for(x1=0;x1<x+1;x1++)
					if(imagedata[x1+((y+1)*320)])
						vsouth++;
				nv++;
			}
			else
				vsouth=-1;
			vsoutheast=-1;
			if(imagedata[(x+1)+((y+1)*320)])
			{
				vsoutheast=vhome;
				for(x1=x+1;x1<320;x1++)
					if(imagedata[x1+(y*320)])
						vsoutheast++;
				for(x1=0;x1<x+2;x1++)
					if(imagedata[x1+((y+1)*320)])
						vsoutheast++;
				nv++;
			}
			else
				vsoutheast=-1;
			if(!imagedata[x+(y*320)])
				vhome=-1;
			else
			{
				vn++;
				nv++;
			}
			if(nv==3)
			{
				long v[3];
				long c[3];
				long index;
				long r1,r2,r3;
				long g1,g2,g3;
				long b1,b2,b3;				
				long r,g,b;

				index=0;
				if(vhome>0)
				{
					v[index]=vhome;
					c[index++]=imagedata[x+(y*320)];
				}
				if(vsouth>0)
				{
					v[index]=vsouth;
					c[index++]=imagedata[x+((y+1)*320)];
				}
				if(vsoutheast>0)
				{
					v[index]=vsoutheast;
					c[index++]=imagedata[(x+1)+((y+1)*320)];
				}
				if(veast>0)
				{
					v[index]=veast;
					c[index++]=imagedata[(x+1)+(y*320)];
				}
				r1=palettedata[c[0]*3];
				r2=palettedata[c[1]*3];
				r3=palettedata[c[2]*3];
				g1=palettedata[(c[0]*3)+1];
				g2=palettedata[(c[1]*3)+1];
				g3=palettedata[(c[2]*3)+1];
				b1=palettedata[(c[0]*3)+2];
				b2=palettedata[(c[1]*3)+2];
				b3=palettedata[(c[2]*3)+2];
				r=(long)((r1+r2+r3)/3);
				g=(long)((g1+g2+g3)/3);
				b=(long)((b1+b2+b3)/3);
				sprintf(string,"<prgb><%d:%d><%d:%d><%d:%d><%d,%d,%d>",objnum,v[0],objnum,v[1],objnum,v[2],r,g,b);
				WriteStringToFile(modelfile,string);
			}
			else if(nv==4)
			{
				long v[3];
				long c[3];
				long r1,r2,r3;
				long g1,g2,g3;
				long b1,b2,b3;				
				long r,g,b;

				v[0]=vhome;
				c[0]=imagedata[x+(y*320)];
				v[1]=vsouth;
				c[1]=imagedata[x+((y+1)*320)];
				v[2]=veast;
				c[2]=imagedata[(x+1)+(y*320)];
				r1=palettedata[c[0]*3];
				r2=palettedata[c[1]*3];
				r3=palettedata[c[2]*3];
				g1=palettedata[(c[0]*3)+1];
				g2=palettedata[(c[1]*3)+1];
				g3=palettedata[(c[2]*3)+1];
				b1=palettedata[(c[0]*3)+2];
				b2=palettedata[(c[1]*3)+2];
				b3=palettedata[(c[2]*3)+2];
				r=(long)((r1+r2+r3)/3);
				g=(long)((g1+g2+g3)/3);
				b=(long)((b1+b2+b3)/3);
				sprintf(string,"<prgb><%d:%d><%d:%d><%d:%d><%d,%d,%d>",objnum,v[0],objnum,v[1],objnum,v[2],r,g,b);
				WriteStringToFile(modelfile,string);
				v[0]=vsouth;
				c[0]=imagedata[x+((y+1)*320)];
				v[1]=vsoutheast;
				c[1]=imagedata[(x+1)+((y+1)*320)];
				v[2]=veast;
				c[2]=imagedata[(x+1)+(y*320)];
				r1=palettedata[c[0]*3];
				r2=palettedata[c[1]*3];
				r3=palettedata[c[2]*3];
				g1=palettedata[(c[0]*3)+1];
				g2=palettedata[(c[1]*3)+1];
				g3=palettedata[(c[2]*3)+1];
				b1=palettedata[(c[0]*3)+2];
				b2=palettedata[(c[1]*3)+2];
				b3=palettedata[(c[2]*3)+2];
				r=(long)((r1+r2+r3)/3);
				g=(long)((g1+g2+g3)/3);
				b=(long)((b1+b2+b3)/3);
				sprintf(string,"<prgb><%d:%d><%d:%d><%d:%d><%d,%d,%d>",objnum,v[0],objnum,v[1],objnum,v[2],r,g,b);
				WriteStringToFile(modelfile,string);
			}
		}
	free(palettedata);
	free(imagedata);
	fclose(imagefile);
	objnum++;
}

void WriteStringToFile(FILE *file,char *string)
{
	unsigned long count;

	for(count=0;count<strlen(string);count++)
		putc(string[count],file);
	putc(13,file);
	putc(10,file);
}

void CloseModelFile(void)
{
	WriteStringToFile(modelfile,"<end>");
	MessageBox(NULL,"Scripting complete","Info",MB_OK);
	fclose(modelfile);
}

void OpenInputFile(char *filename)
{
	inputfile=fopen(filename,"rb");
}

void GetNextCommand(void)
{
	char hit10=0;
	char hit13=0;
	long pos=0;
	char done=0;

	while(!done)
	{
		char c=getc(inputfile);
		if(feof(inputfile))
		{
			MessageBox(NULL,"End of File","Info",MB_OK);
			return;
		}
		if(c==10)
		{
			hit10=1;
			c=0;
		}
		if(c==13)
		{
			hit13=1;
			c=0;
		}
		istring[pos++]=c;
		if(hit10&&hit13)
			done=1;
	}
}

void GetNextNumber(long *num)
{
	char hit10=0;
	char hit13=0;
	long pos=0;
	long val=0;
	char done=0;
	char negflag;

	while(!done)
	{
		char c=getc(inputfile);
		if(feof(inputfile))
		{
			MessageBox(NULL,"End of File","Info",MB_OK);
			return;
		}
		if(c==10)
		{
			hit10=1;
			c=0;
		}
		if(c==13)
		{
			hit13=1;
			c=0;
		}
		istring[pos++]=c;
		if(hit10&&hit13)
			done=1;
	}
	done=0;
	pos=0;
	negflag=0;
	while(!done)
	{
		if(istring[pos]==0)
			done=1;
		else if(istring[pos]=='-')
			negflag=1;
		else if(istring[pos]>='0'&&istring[pos]<='9')
				val=(val*10)+istring[pos]-'0';
		pos++;
	}
	if(negflag)
		val=-val;
	*num=val;
}

void ParseIString(void)
{
	unsigned long count,count2;

	for(count=0;count<strlen(istring)+1;count++)
		if(istring[count]=='"')
		{
			if(count==0)
				for(count2=1;count2<strlen(istring)+1;count2++)
					istring[count2-1]=istring[count2];
			else if(count==strlen(istring)-1)
				istring[count]=0;
			else
				for(count2=count;count2<strlen(istring)+1;count2++)
					istring[count2-1]=istring[count2];
		}
}

void CloseInputFile(void)
{
	fclose(inputfile);	
}
