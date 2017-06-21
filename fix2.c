/* program to extract individual email messages from a Thunderbird */
/* message-store, found at various locations therein.              */
/*                                                                 */
/* THIS SOFTWARE IS PRESENTED "AS-IS", AND MAKES NO  CLAIM  OF     */
/* ACCURATE OPERATION, APPLICABILITY TO YOUR NEEDS, SUCCESSFUL     */
/* OUTCOME, SAFE MANIPULATION OF DATA.  -> USE AT YOUR OWN RISK <- */
/*                                                                 */
/* The author is in no way responsible for what happens when you   */
/* use this program or apply its concepts.                         */
/*                                                                 */
/* FULL DOCUMENTATION IS CONTAINED IN THE 'README' file            */
/*                                                                 */
/* June 2017  R. Magnuson   rjm@gandsnut.net                       */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#define CMDSTRLEN 256
#define INBOXFILE 1
#define SENTFILE  2
#define PROCGOOD  0
#define PROCERROR -1
#define OUTEXTEN  ".eml"
#define GREPINBOX "grep -b \"From -\" Inbox > "
#define GREPSENT  "grep -b \"From -\" Sent > "
#define STAGE2    "/tmp/stage2"
#define STAGE3    "/tmp/stage3"

int main(int, char *[]);
int create_grep_out(int);
int process_stage2(int);
int gen_emails(int);

int main(int argc, char *argv[])
{
    printf("\nProgram begin.\n\n");

    create_grep_out(SENTFILE);
    process_stage2(SENTFILE);
    gen_emails(SENTFILE);

    create_grep_out(INBOXFILE);
    process_stage2(INBOXFILE);
    gen_emails(INBOXFILE);

    printf("Done.\n\n");
    return(0);
}

int create_grep_out(int whichfile)
{
char cmdstr[CMDSTRLEN];
int  retval;

    if (whichfile == INBOXFILE)
    	sprintf(cmdstr, "%s%s.inbox", GREPINBOX, STAGE2); 
    else
    	sprintf(cmdstr, "%s%s.sent", GREPSENT, STAGE2); 

    printf("\nExecuting \"%s\"\n", cmdstr);

    retval = system(cmdstr);
    return(retval);
}

int process_stage2(int whichfile)
{
char         *cptr;
char         i_filestr[CMDSTRLEN];
char         o_filestr[CMDSTRLEN];
char         instr[128];
int          retval;
unsigned int q_strfound;
FILE         *ifp;
FILE         *ofp;

    if (whichfile == INBOXFILE)
    	sprintf(i_filestr, "%s.inbox", STAGE2); 
    else
    	sprintf(i_filestr, "%s.sent", STAGE2); 

    if (whichfile == INBOXFILE)
    	sprintf(o_filestr, "%s.inbox", STAGE3); 
    else
    	sprintf(o_filestr, "%s.sent", STAGE3); 

    printf("\nFiltering \"%s\" into \"%s\"\n\n", i_filestr, o_filestr);

    ifp = fopen(i_filestr, "r");
    ofp = fopen(o_filestr, "w");

    if (ifp == (FILE *) NULL)
    {
    printf("\nFile \"%s\" open for read, error: %s\n\n", i_filestr, strerror(errno));
    return(-1);
    }

    if (ofp == (FILE *) NULL)
    {
    printf("\nFile \"%s\" open for write, error: %s\n\n", o_filestr, strerror(errno));
    return(-1);
    }

   q_strfound = 0;
   while ((fgets(instr, 128, ifp)) != (char *) NULL)
   {
   if (ferror(ifp))
       puts("I/O error when reading");
   else if (feof(ifp) != 0)
       break;

   cptr  = strchr(instr, (int) ':');
   *cptr = (char) '\0';

   fprintf(ofp, "%s\n", instr);
   q_strfound++;
   }

   fclose(ifp);
   fflush(ofp);
   fclose(ofp);

   printf("Lines processed: %u\n\n", q_strfound);
   return(0);
}

int gen_emails(int whichfile)
{
char         *cptr;
char         in_fname[CMDSTRLEN];
char         out_fname[CMDSTRLEN];
char         data_fname[CMDSTRLEN];
char         instr[128];
int          ich;
int          retval;
int          filecntr;
unsigned int endloc;
unsigned int curloc;
FILE         *ifp;
FILE         *datainfp;
FILE         *emloutfp;

    if (whichfile == INBOXFILE)
        {
    	sprintf(in_fname, "%s.inbox", STAGE3); 
        strcpy(data_fname, "Inbox");
        }
    else
        {
    	sprintf(in_fname, "%s.sent", STAGE3); 
        strcpy(data_fname, "Sent");
        }

   printf("\nBegin email files extraction\n\n");

   ifp = fopen(in_fname, "r");    /* open file of byte offsets */
   if (ifp == (FILE *) NULL)
   {
   printf("\nFile \"%s\" open for read, error: %s\n\n", in_fname, strerror(errno));
   fclose(ifp);
   return(-1);
   }

   fgets(instr, 128, ifp);     /* all stage3 files will have first line with "0", step past it */
   curloc = 0;

   datainfp = fopen(data_fname, "r");    /* open big data file to read through, byte-at-a-time */
   if (datainfp == (FILE *) NULL)
   {
   printf("\nFile \"%s\" open for read, error: %s\n\n", data_fname, strerror(errno));
   fclose(ifp);
   return(-2);
   }

   filecntr = 1;
   while ((fgets(instr, 128, ifp)) != (char *) NULL)
   {
   if (ferror(ifp))
       puts("I/O error when reading");
   else if (feof(ifp) != 0)
       break;

   cptr   = strchr(instr, (int) '\n');
   *cptr  = (char) '\0';
   endloc = (unsigned int) atol(instr);

   /* scan through the chosen one, and write out bytes to multiple .eml files                 */

   if (whichfile == INBOXFILE)
      sprintf(out_fname, "inbox%d%s", filecntr, OUTEXTEN);
   else
      sprintf(out_fname, "sent%d%s", filecntr, OUTEXTEN);

   emloutfp = fopen(out_fname, "w");
   if (emloutfp == (FILE *) NULL)
   {
   printf("\nFile \"%s\" open for write, error: %s\n\n", out_fname, strerror(errno));
   fclose(ifp);
   fclose(datainfp);
   return(-3);
   }

   while ((ich = fgetc(datainfp)) != EOF)
      {
      if (ferror(datainfp))	
          puts("I/O error when reading");
      else if (feof(datainfp) != 0)
      break;

      if (ich != 0x0d)
          fputc(ich, emloutfp);

      if (++curloc == endloc - 1)
          break;
      }

   fflush(emloutfp);
   fclose(emloutfp);

   filecntr++;
   }

   /* one more file to be written out, but we don't know its endloc, because that endloc = EOF */

   if (whichfile == INBOXFILE)
      sprintf(out_fname, "inbox%d%s", filecntr, OUTEXTEN);
   else
      sprintf(out_fname, "sent%d%s", filecntr, OUTEXTEN);

   emloutfp = fopen(out_fname, "w");
   if (emloutfp == (FILE *) NULL)
   {
   printf("\nFile \"%s\" open for write, error: %s\n\n", out_fname, strerror(errno));
   fclose(ifp);
   fclose(datainfp);
   return(-4);
   }

   while ((ich = fgetc(datainfp)) != EOF)
      {
      if (ferror(datainfp))	
          puts("I/O error when reading");
      else if (feof(datainfp) != 0)
          break;

      if (ich != 0x0d)
          fputc(ich, emloutfp);
      }

   fflush(emloutfp);
   fclose(emloutfp);

   fclose(datainfp);
   fclose(ifp);
   printf("\nCreated %d email files\n\n", filecntr);
   return(0);
}

