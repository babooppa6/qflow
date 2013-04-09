// BDnet2Verilog
//
// Revision 0, 2006-11-11: First release by R. Timothy Edwards.
// Revision 1, 2009-07-13: Minor cleanups by Philipp Klaus Krause.
// Revision 2, 2011-11-7: Added flag "-c" to maintain character case
//
// This program is written in ISO C99.

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <float.h>

#define	EXIT_SUCCESS	0
#define	EXIT_FAILURE	1
#define	EXIT_HELP	2
#define TRUE 		1
#define FALSE		0
#define NMOS		1
#define PMOS		0
#define LengthOfNodeName  100
#define LengthOfLine    200
#define MaxNumberOfInputs 100
#define MaxNumberOfOutputs 100
#define LengthOfInOutString 10000
#define NumberOfUniqueConnections 6

/* getopt stuff */
extern	int	optind, getopt();
extern	char	*optarg;

struct Vect {
	struct Vect *next;
	char name[LengthOfLine];
	char direction[LengthOfNodeName];
	int Max;
};

void ReadNetlistAndConvert(FILE *, FILE *, int, int);
void CleanupString(char text[]);
void ToLowerCase( char *text);
float getnumber(char *strpntbegin);
int loc_getline( char s[], int lim, FILE *fp);
void helpmessage();
int ParseNumber( char *test);
struct Vect *VectorAlloc(void);

int main ( int argc, char *argv[])
{

	FILE *NET1, *NET2, *OUT;
	struct Resistor *ResistorData;
	int i,AllMatched,NetsEqual,ImplicitPower,MaintainCase;

	char Net1name[LengthOfNodeName];
	






	ImplicitPower=TRUE;
	MaintainCase=FALSE;
        while( (i = getopt( argc, argv, "pchH" )) != EOF ) {
	   switch( i ) {
	   case 'p':
	       ImplicitPower=FALSE;
	       break;
	   case 'c':
	       MaintainCase=TRUE;
	       break;
	   case 'h':
	   case 'H':
	       helpmessage();
	       break;
	   default:
	       fprintf(stderr,"\nbad switch %d\n", i );
	       helpmessage();
	       break;
	   }
        }

        if( optind < argc )	{
           strcpy(Net1name,argv[optind]);
	   optind++;
	}
	else	{
	   fprintf(stderr,"Couldn't find a filename as input\n");
	   exit(EXIT_FAILURE);
	}
        optind++;
	NET1=fopen(Net1name,"r");
	if (NET1 == NULL ) {
		fprintf(stderr,"Couldn't open %s for read\n",Net1name);
		exit(EXIT_FAILURE);
	}
	OUT=stdout;
	ReadNetlistAndConvert(NET1,OUT,ImplicitPower,MaintainCase);
        return 0;


}





/*--------------------------------------------------------------*/
/*C *Alloc - Allocates memory for linked list elements		*/
/*								*/
/*         ARGS: 
        RETURNS: 1 to OS
   SIDE EFFECTS: 
\*--------------------------------------------------------------*/
void ReadNetlistAndConvert(FILE *NETFILE, FILE *OUT, int ImplicitPower, int MaintainCase)
{

	struct Vect *Vector,*VectorPresent;
	int i,Found,NumberOfInputs,NumberOfOutputs,NumberOfInstances;
	int First,VectorIndex,ItIsAnInput,ItIsAnOutput,PrintIt;
	int breakCond;

	float length, width, Value;


	char *node2pnt,*lengthpnt,*widthpnt,*FirstblankafterNode2, *Weirdpnt;
        char line[LengthOfLine];
	char allinputs[LengthOfInOutString];
	char alloutputs[LengthOfInOutString];
	char InputName[LengthOfNodeName],InputEquivalent[LengthOfNodeName];
	char OutputName[LengthOfNodeName],OutputEquivalent[LengthOfNodeName];
	char InputNodes[MaxNumberOfInputs][2][LengthOfNodeName];
	char OutputNodes[MaxNumberOfOutputs][2][LengthOfNodeName];
        char MainSubcktName[LengthOfNodeName],node1[LengthOfNodeName];
	char InstanceName[LengthOfNodeName],InstancePortName[LengthOfNodeName];
	char InstancePortWire[LengthOfNodeName];
        char node2[LengthOfNodeName],nodelabel[LengthOfNodeName];
        char body[LengthOfNodeName],model[LengthOfNodeName];
	char dum[LengthOfNodeName];

/* Read in line by line */

	NumberOfInstances=0;
	NumberOfOutputs=0;
	NumberOfInputs=0;
	First=TRUE;
	Vector=VectorAlloc();
	Vector->next=NULL;
        while(loc_getline(line, sizeof(line), NETFILE)>0 ) {
	   if(strstr(line,"MODEL") != NULL ) {
              if(sscanf(line,"MODEL %s",MainSubcktName)==1) {
	         CleanupString(MainSubcktName);
	         fprintf(OUT,"module %s (",MainSubcktName);
	         if(ImplicitPower) fprintf(OUT," VSS, VDD, "); 
	      }
	      else if(strstr(line,"ENDMODEL") != NULL) {
                 fprintf(OUT,"endmodule\n");
              }
	   }
	   if(strstr(line,"INPUT") != NULL ) {
	      breakCond = FALSE;
	      strcpy(allinputs,"");
	      while(loc_getline(line, sizeof(line), NETFILE)>1 ) {
                 if(sscanf(line,"%s :  %s",InputName,InputEquivalent)==2) {
		    if (InputEquivalent[strlen(InputEquivalent) - 1] == ';')
		       breakCond = TRUE;
	            PrintIt=TRUE;
	            CleanupString(InputName);
	            CleanupString(InputEquivalent);
	            strcpy(InputNodes[NumberOfInputs][0],InputName);
	            strcpy(InputNodes[NumberOfInputs][1],InputEquivalent);
	            if( (Weirdpnt=strchr(InputName,'[')) != NULL) {
	               PrintIt=FALSE;
	               VectorIndex=ParseNumber(Weirdpnt);// This one needs to cut off [..]
	               VectorPresent=Vector;
	               Found=FALSE;
	               while(VectorPresent->next !=NULL && !Found) {
	                  if(strcmp(VectorPresent->name,InputName)==0) {
	                     VectorPresent->Max= VectorPresent->Max > VectorIndex ? VectorPresent->Max : VectorIndex;
	                     Found=TRUE;
	                  }
	                  VectorPresent=VectorPresent->next; 
	               }
	               if(!Found) {
	                  strcpy(VectorPresent->name,InputName);
	                  strcpy(VectorPresent->direction,"input");
	                  VectorPresent->Max=VectorIndex;
	                  VectorPresent->next=VectorAlloc();
	                  VectorPresent->next->next=NULL;
	               }
	            }
	            if(PrintIt || ! Found) { // Should print vectors in module statement
	               if(First) {
	                  fprintf(OUT,"%s",InputName);
	                  First=FALSE;
	               }
	               else fprintf(OUT,", %s",InputName);
	            }
	            if(PrintIt) {//Should not print vectors now
	               strcat(allinputs,"input ");
	               strcat(allinputs,InputName);
	               strcat(allinputs,";\n");
	            }
	            NumberOfInputs+=1; 
	         }
		 if (breakCond) break;
	      }
	   }
	   if(strstr(line,"OUTPUT") != NULL ) {
	      breakCond = FALSE;
	      strcpy(alloutputs,"");
	      while(loc_getline(line, sizeof(line), NETFILE)>1 ) {
                 if(sscanf(line,"%s :  %s",OutputName,OutputEquivalent)==2) {
		    if (OutputEquivalent[strlen(OutputEquivalent) - 1] == ';')
			breakCond = TRUE;
	            PrintIt=TRUE;
	            CleanupString(OutputName);
	            CleanupString(OutputEquivalent);
	            strcpy(OutputNodes[NumberOfOutputs][0],OutputName);
	            strcpy(OutputNodes[NumberOfOutputs][1],OutputEquivalent);
	            if( (Weirdpnt=strchr(OutputName,'[')) != NULL) {
	               PrintIt=FALSE;
	               VectorIndex=ParseNumber(Weirdpnt);// This one needs to cut off [..]
	               VectorPresent=Vector;
	               Found=FALSE;
	               while(VectorPresent->next !=NULL && !Found) {
	                  if(strcmp(VectorPresent->name,OutputName)==0) {
	                     VectorPresent->Max= VectorPresent->Max > VectorIndex ? VectorPresent->Max : VectorIndex;
	                     Found=TRUE;
	                  }
	                  VectorPresent=VectorPresent->next; 
	               }
	               if(!Found) {
	                  strcpy(VectorPresent->name,OutputName);
	                  strcpy(VectorPresent->direction,"output");
	                  VectorPresent->Max=VectorIndex;
	                  VectorPresent->next=VectorAlloc();
	                  VectorPresent->next->next=NULL;
	               }
	            }
	            if(PrintIt || ! Found) {
	               if(First) {
	                  fprintf(OUT,"%s",OutputName);
	                  First=FALSE;
	               }
	               else fprintf(OUT,", %s",OutputName);
	            }
	            if(PrintIt) {
	               strcat(alloutputs,"output ");
	               strcat(alloutputs,OutputName);
	               strcat(alloutputs,";\n");
	            }
	            NumberOfOutputs+=1; 
	         }
		 if (breakCond) break;
	      }
	      fprintf(OUT,");\n");
	      if(ImplicitPower) fprintf(OUT,"input VSS, VDD; ");
	      fprintf(OUT,"%s",allinputs);
	      fprintf(OUT,"%s",alloutputs);
	      VectorPresent=Vector;
	      while (VectorPresent->next != NULL) {
	         fprintf(OUT,"%s [%d:0] %s;\n",VectorPresent->direction,VectorPresent->Max,VectorPresent->name);
	         VectorPresent=VectorPresent->next;
	      }
	   }
	   if(strstr(line,"INSTANCE") != NULL ) {
	      NumberOfInstances+=1;
	      if(sscanf(line,"INSTANCE %s:",InstanceName)==1) {
	         CleanupString(InstanceName);
	         if (!MaintainCase) ToLowerCase(InstanceName);
	         fprintf(OUT,"\t%s u%d ( ",InstanceName,NumberOfInstances);
	         First=TRUE;
	         if(ImplicitPower) fprintf(OUT,".VSS(VSS), .VDD(VDD), "); 
	         while(loc_getline(line, sizeof(line), NETFILE)>1 ) {
	            if(sscanf(line,"%s :  %s",InstancePortName,InstancePortWire)==2) {
	               CleanupString(InstancePortName);
	               CleanupString(InstancePortWire);
	               ItIsAnInput=FALSE;
	               ItIsAnOutput=FALSE;
	               for(i=0;i<NumberOfInputs;i++) {
	                  if(strcmp(InstancePortWire,InputNodes[i][1]) == 0) {
	                     ItIsAnInput=TRUE;
	                     strcpy(InstancePortWire,InputNodes[i][0]);
	                     Weirdpnt=strchr(InstancePortWire,']');
	                     if(Weirdpnt != NULL) *(Weirdpnt+1)='\0';
	                  }
	               }
	               for(i=0;i<NumberOfOutputs;i++) {
	                  if(strcmp(InstancePortWire,OutputNodes[i][1]) == 0) {
	                     ItIsAnOutput=TRUE;
	                     strcpy(InstancePortWire,OutputNodes[i][0]);
	                     Weirdpnt=strchr(InstancePortWire,']');
	                     if(Weirdpnt != NULL) *(Weirdpnt+1)='\0';
	                  }
	               }
	               if(!ItIsAnInput && !ItIsAnOutput) {
	                  if( (Weirdpnt=strchr(InstancePortWire,'['))!=NULL) {
	                     *Weirdpnt='_';
	                     if( (Weirdpnt=strchr(InstancePortWire,']'))!=NULL) 
	                        *Weirdpnt='_';
	                  }
	                  while( (Weirdpnt=strchr(InstancePortWire,'$'))!=NULL) {
	                     *Weirdpnt='_';
	                     Weirdpnt++;
	                  }
	               }

	                 
	               if(InstancePortWire[0] <= '9' && InstancePortWire[0] >='0') {
	                  strcpy(dum,"N_");
	                  strcat(dum,InstancePortWire);
	                  strcpy(InstancePortWire,dum);
	               }
	               if(First) {
	                  fprintf(OUT,".%s(%s)",InstancePortName,InstancePortWire);
	                  First=FALSE;
	               }
	               else fprintf(OUT,", .%s(%s)",InstancePortName,InstancePortWire);
	            }
	         }
	         fprintf(OUT," );\n");
	      }
	   } 
	}
}


int ParseNumber( char *text)
{
	char *begin, *end;
// Assumes *text is a '['
	begin=(text+1);
	end=strchr(begin,']');
	*end='\0';
	*text='\0';
	return atoi(begin);
}
	

	

void ToLowerCase( char *text)
{
        int i=0;

        while( text[i] != '\0' ) {
           text[i]=tolower(text[i]);
           i++;
        }
}


void CleanupString(char text[LengthOfNodeName])
{
	int i;
	char *CitationPnt, *Weirdpnt;
	
	CitationPnt=strchr(text,'"');
	if( CitationPnt != NULL) {
	   i=0;
	   while( CitationPnt[i+1] != '"' ) {
	      CitationPnt[i]=CitationPnt[i+1];
	      i+=1;
	   }
	   CitationPnt[i]='\0';
           CitationPnt=strchr(text,'[');
	   if(CitationPnt != NULL) {
              i=0;
              while( CitationPnt[i+1] != ']' ) {
                 CitationPnt[i]=CitationPnt[i+1];
                 i+=1;
              }
              CitationPnt[i]='\0';
	   }
	}
	Weirdpnt=strchr(text,'<');
	if(Weirdpnt != NULL) *Weirdpnt='[';
	Weirdpnt=strchr(text,'>');
	if(Weirdpnt != NULL) *Weirdpnt=']';
}


/*--------------------------------------------------------------*/
/*C getnumber - gets number pointed by strpntbegin		*/
/*								*/
/*         ARGS: strpntbegin - number expected after '='        */ 
/*        RETURNS: 1 to OS
   SIDE EFFECTS: 
\*--------------------------------------------------------------*/
float getnumber(char *strpntbegin)
{       int i;
        char *strpnt,magn1,magn2;
        float number;

        strpnt=strpntbegin;
        strpnt=strchr(strpntbegin,'=');
        if(strpnt == NULL) {
           fprintf(stderr,"Error: getnumber: Didn't find '=' in string "
                               "%s\n",strpntbegin);
           return DBL_MAX;
        }
        strpnt=strpnt+1;
        
	if(sscanf(strpnt,"%f%c%c",&number,&magn1, &magn2)!=3) {
            if(sscanf(strpnt,"%f%c",&number,&magn1)!=2) {
               fprintf(stderr,"Error: getnumber : Couldn't read number in "
                      "%s %s\n",strpntbegin,strpnt);
               return DBL_MAX;
            }
        }
/*if(*strpntbegin =='m') */
        switch( magn1 ) {
        case 'f':
           number *= 1e-15;
           break;          
        case 'p':
           number *= 1e-12;
           break;          
        case 'n':
           number *= 1e-9;
           break;          
        case 'u':
           number *= 1e-6;
           break;          
        case 'm':
           if(magn2='e') number *= 1e6;
           else number *= 1e-3;
           break;          
        case 'k':
           number *= 1e3;
           break;          
        case 'g':
           number *= 1e9;
           break;
        case ' ':
           default:
           return number;
        }
        return number;
}          





















/*--------------------------------------------------------------*/
/*C loc_getline: read a line, return length		*/
/*								*/
/*         ARGS: 
        RETURNS: 1 to OS
   SIDE EFFECTS: 
\*--------------------------------------------------------------*/
int loc_getline( char s[], int lim, FILE *fp)
{
	int c, i;
	
	i=0;
	while(--lim > 0 && (c=getc(fp)) != EOF && c != '\n')
		s[i++] = c;
	if (c == '\n');
		s[i++] = c;
	s[i] = '\0';
	if ( c == EOF ) i=0; 
	return i;
}


struct Vect *VectorAlloc(void)
{
	return (struct Vect *) malloc(sizeof(struct Vect));
}


/*--------------------------------------------------------------*/
/*C helpmessage - tell user how to use the program		*/
/*								*/
/*         ARGS: 
        RETURNS: 1 to OS
   SIDE EFFECTS: 
\*--------------------------------------------------------------*/


void helpmessage()
{

    fprintf(stderr,"BDnet2Verilog [-options] netlist \n");
    fprintf(stderr,"\n");
    fprintf(stderr,"BDnet2Verilog converts a netlist in bdnet format \n");
    fprintf(stderr,"to Verilog format. Output on stdout\n");
    fprintf(stderr,"\n");
    fprintf(stderr,"option, -h this message\n");    
    fprintf(stderr,"option, -p means: don't add power nodes to instances\n");
    fprintf(stderr,"        only nodes present in the INSTANCE statement used\n");

    exit( EXIT_HELP );	
} /* helpmessage() */


