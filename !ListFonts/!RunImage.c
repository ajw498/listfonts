/*
	ListFonts
	©Alex Waugh 1999
*/

/*	Includes  */

#include "DeskLib:Window.h"
#include "DeskLib:Error.h"
#include "DeskLib:Event.h"
#include "DeskLib:EventMsg.h"
#include "DeskLib:Handler.h"
#include "DeskLib:Hourglass.h"
#include "DeskLib:Icon.h"
#include "DeskLib:Menu.h"
#include "DeskLib:Msgs.h"
#include "DeskLib:Resource.h"
#include "DeskLib:Screen.h"
#include "DeskLib:Template.h"
#include "DeskLib:File.h"
#include "DeskLib:Filing.h"
#include "DeskLib:Drag.h"
#include "DeskLib:Keycodes.h"

#include "AJWLib:Window.h"
#include "AJWLib:Menu.h"
#include "AJWLib:Msgs.h"
#include "AJWLib:Misc.h"
#include "AJWLib:Handler.h"
#include "AJWLib:Error.h"
#include "AJWLib:Flex.h"
#include "AJWLib:DrawFile.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*	Macros  */

#define VERSION "1.00 (7-Jan-99)"
#define DIRPREFIX "ListFonts"

#define icon_SAVE   0
#define icon_CANCEL 1
#define icon_WRITE  2
#define icon_FILE   3

#define menuitem_INFO    0
#define menuitem_ONEFONT 1
#define menuitem_SPLIT   2
#define menuitem_QUIT    3

#define SAVEFILETYPE 0xFFF

/*	Variables  */

window_handle mainwin,info;
menu_ptr menu,submenu;
char splitnumber[5]="249";

/*	Functions  */

void SubMenuClick(int entry, void *r)
{
	Menu_SetFlags(menu,menuitem_SPLIT,TRUE,FALSE);
	Icon_SetText(mainwin,icon_FILE,"directory");
}

void MenuClick(int entry, void *r)
{
	int tick;
	switch (entry) {
		case menuitem_ONEFONT:
			Menu_ToggleTick(menu,entry);
			break;
		case menuitem_SPLIT:
			Menu_ToggleTick(menu,entry);
			Menu_GetFlags(menu,entry,&tick,NULL);
			if (tick) Icon_SetText(mainwin,icon_FILE,"directory"); else Icon_SetText(mainwin,icon_FILE,"file_fff");
			break;
		case menuitem_QUIT:
			Event_CloseDown();
			break;
	}
}

void Error(void)
{
	Hourglass_Off();
	Error_Report(1,"Unable to open file: %s",_kernel_last_oserror()->errmess);
	Event_CloseDown();
}
void SaveFile(char *filename)
{
	FILE *file;
	char fontname[256],filename2[300],oldfontname[255];
	int counter,count=0,max,current=1,tick,onefont;
	Hourglass_On();
	max=atoi(splitnumber);
	Menu_GetFlags(menu,menuitem_SPLIT,&tick,NULL);
	Menu_GetFlags(menu,menuitem_ONEFONT,&onefont,NULL);
	if (tick) {
		SWI(5,0,SWI_OS_File,8,filename,NULL,NULL,0);
		strcpy(filename2,filename);
		strcat(filename2,".1");
		if ((file=fopen(filename2,"w"))==NULL) Error();
	} else {
    	if ((file=fopen(filename,"w"))==NULL) Error();
    }
	SWI(7,3,0x40091,NULL,fontname,0x20000,256,fontname,256,0,NULL,NULL,&counter); /*Font_ListFonts*/
	while ((counter)!=-1) {
		if (onefont) {
			char *end;
			char newfontname[255];
			strcpy(newfontname,fontname);
			end=strchr(newfontname,'.');
			if (end!=NULL) *end='\0';
			if (strcmp(oldfontname,newfontname)!=0) {
				count++;
				strcpy(oldfontname,newfontname);
				fprintf(file,"{font %s}%s{font}\n",fontname,newfontname);
			}
		} else {
			fprintf(file,"{font %s}%s{font}\n",fontname,fontname);
			count++;
		}
		SWI(7,3,0x40091,NULL,fontname,counter,256,fontname,256,0,NULL,NULL,&counter); /*Font_ListFonts*/
		if (count>=max && tick) {
			count=0;
			fclose(file);
			sprintf(filename2,"%s.%d",filename,++current);
			if ((file=fopen(filename2,"w"))==NULL) Error();
		}

	}
	fclose(file);
	Hourglass_Off();
}

void DragEnded(void *r)
{
	mouse_block mouseblk;
	message_block msgblk;
	char leafname[256];
	Wimp_GetPointerInfo(&mouseblk);
	msgblk.header.size=56;
	msgblk.header.yourref=0;
	msgblk.header.action=message_DATASAVE;
	msgblk.data.datasave.window=mouseblk.window;
	msgblk.data.datasave.icon=mouseblk.icon;
	msgblk.data.datasave.pos=mouseblk.pos;
	msgblk.data.datasave.estsize=1024;
	msgblk.data.datasave.filetype=SAVEFILETYPE;
	Filing_GetLeafname(Icon_GetTextPtr(mainwin,icon_WRITE),leafname);
	if (strlen(leafname)>10) leafname[10]='\0';
	strcpy(msgblk.data.datasave.leafname,leafname);
	Wimp_SendMessage(event_USERMESSAGERECORDED,&msgblk,mouseblk.window,mouseblk.icon);
}

BOOL DataSaveAck(event_pollblock *block,void *r)
{
	SaveFile(block->data.message.data.datasaveack.filename);
	block->data.message.header.yourref=block->data.message.header.myref;
	block->data.message.header.action=message_DATALOAD;
	Wimp_SendMessage(event_USERMESSAGERECORDED,&block->data.message,block->data.message.header.sender,0);
	return TRUE;
}

BOOL DataLoadAck(event_pollblock *block,void *r)
{
	Event_CloseDown();
	return TRUE;
}

BOOL MessageAck(event_pollblock *block,void *r)
{
	switch (block->data.message.header.action) {
		case message_DATASAVE:
			/*Do nothing*/
			break;
		case message_DATALOAD:
			Error_Report(1,"Data transfer failed: Reciever died");
			break;
		default:
			return FALSE;
	}
	return TRUE;
}

BOOL FileDragged(event_pollblock *block,void *r)
{
	if (block->data.mouse.button.data.dragselect==FALSE && block->data.mouse.button.data.dragadjust==FALSE) return FALSE;
	Icon_StartSolidDrag(block->data.mouse.window,icon_FILE);
	Drag_SetHandlers(NULL,DragEnded,r);
	return TRUE;
}

BOOL OkPressed(event_pollblock *block,void *r)
{
	char *filename;
	if (block->data.mouse.button.data.menu) return FALSE;
	filename=Icon_GetTextPtr(mainwin,icon_WRITE);
	if (strlen(filename)<1 || strpbrk(filename,".:")==NULL) {
		Error_Report(1,"Enter a name for the file, then drag the icon to a directory display");
		return TRUE;
	}
	SaveFile(filename);
	Event_CloseDown();
	return TRUE;
}

BOOL CancelPressed(event_pollblock *block,void *r)
{
	if (block->data.mouse.button.data.menu) return FALSE;
	Event_CloseDown();
	return TRUE;
}

BOOL KeyPressed(event_pollblock *block,void *r)
{
	char *filename;
	switch (block->data.key.code) {
		case keycode_RETURN:
			filename=Icon_GetTextPtr(mainwin,icon_WRITE);
			if (strlen(filename)<1 || strpbrk(filename,".:")==NULL) {
				Error_Report(1,"Enter a name for the file, then drag the icon to a directory display");
				return TRUE;
			}
            Icon_SetSelect(mainwin,icon_SAVE,TRUE);
			SaveFile(filename);
			Event_CloseDown();
			break;
		case keycode_ESCAPE:
			Icon_SetSelect(mainwin,icon_CANCEL,TRUE);
			Event_CloseDown();
			break;
		default:
			return FALSE;
	}
	return TRUE;
}

int main(void)
{
	Error_RegisterSignals();
	Resource_Initialise(DIRPREFIX);
	Msgs_LoadFile("Messages");
	Event_Initialise(Msgs_TempLookup("Task.Name:"));
	EventMsg_Initialise();
	Screen_CacheModeInfo();
	Template_Initialise();
	EventMsg_Claim(message_MODECHANGE,event_ANY,Handler_ModeChange,NULL);
	Event_Claim(event_CLOSE,event_ANY,event_ANY,Handler_CloseWindow,NULL);
	Event_Claim(event_OPEN,event_ANY,event_ANY,Handler_OpenWindow,NULL);
	Event_Claim(event_KEY,event_ANY,event_ANY,Handler_KeyPress,NULL);
	Event_Claim(event_REDRAW,event_ANY,event_ANY,Handler_HatchRedraw,NULL);
	Template_LoadFile("Templates");
	mainwin=Window_CreateAndShow("Main",template_TITLEMIN,open_CENTERED);
	Msgs_SetTitle(mainwin,"Win.Title:");
	Msgs_SetText(mainwin,icon_CANCEL,"Icon.Cancel");
	Msgs_SetText(mainwin,icon_SAVE,"Icon.Save");
	Msgs_SetText(mainwin,icon_WRITE,"Icon.Write");
	Icon_SetCaret(mainwin,icon_WRITE);
	info=Window_CreateInfoWindowFromMsgs("Task.Name:","Task.Purpose:","©Alex Waugh 1999",VERSION);
	menu=Menu_CreateFromMsgs("Menu.Title:","Menu.Body:",MenuClick,NULL);
	submenu=Menu_CreateFromMsgs("Sub.Title:","Sub.Body:",SubMenuClick,NULL);
	Menu_MakeWritable(submenu,0,splitnumber,5,"A0-9");
	Menu_AddSubMenu(menu,menuitem_INFO,(menu_ptr)info);
	Menu_AddSubMenu(menu,menuitem_SPLIT,submenu);
	Menu_Attach(mainwin,event_ANY,menu,button_MENU);
	Menu_SetFlags(menu,menuitem_ONEFONT,TRUE,FALSE);
	Drag_Initialise(FALSE);
	Event_Claim(event_CLICK,mainwin,icon_FILE,FileDragged,NULL);
	Event_Claim(event_CLICK,mainwin,icon_SAVE,OkPressed,NULL);
	Event_Claim(event_CLICK,mainwin,icon_CANCEL,CancelPressed,NULL);
	Event_Claim(event_KEY,mainwin,event_ANY,KeyPressed,NULL);
	Event_Claim(event_USERMESSAGEACK,event_ANY,event_ANY,MessageAck,NULL);
	EventMsg_Claim(message_DATASAVEACK,event_ANY,DataSaveAck,NULL);
	EventMsg_Claim(message_DATALOADACK,event_ANY,DataLoadAck,NULL);
	while (TRUE) Event_Poll();
	return 0;
}

