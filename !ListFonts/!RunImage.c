/*
	ListFonts
	©Alex Waugh 1999
*/

/*	Includes  */

#include "Desk.Window.h"
#include "Desk.Error2.h"
#include "Desk.Event.h"
#include "Desk.EventMsg.h"
#include "Desk.Handler.h"
#include "Desk.Hourglass.h"
#include "Desk.Icon.h"
#include "Desk.Menu.h"
#include "Desk.Msgs.h"
#include "Desk.Resource.h"
#include "Desk.Screen.h"
#include "Desk.Template.h"
#include "Desk.File.h"
#include "Desk.Filing.h"
#include "Desk.Drag.h"
#include "Desk.Keycodes.h"

#include "AJWLib.Window.h"
#include "AJWLib.Menu.h"
#include "AJWLib.Msgs.h"
#include "AJWLib.Error2.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <kernel.h>


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

Desk_window_handle mainwin,info;
Desk_menu_ptr menu,submenu;
char splitnumber[5]="249";

/*	Functions  */

void SubMenuClick(int entry, void *r)
{
	Desk_Menu_SetFlags(menu,menuitem_SPLIT,Desk_TRUE,Desk_FALSE);
	Desk_Icon_SetText(mainwin,icon_FILE,"directory");
}

void MenuClick(int entry, void *r)
{
	int tick;
	switch (entry) {
		case menuitem_ONEFONT:
			AJWLib_Menu_ToggleTick(menu,entry);
			break;
		case menuitem_SPLIT:
			AJWLib_Menu_ToggleTick(menu,entry);
			Desk_Menu_GetFlags(menu,entry,&tick,NULL);
			if (tick) Desk_Icon_SetText(mainwin,icon_FILE,"directory"); else Desk_Icon_SetText(mainwin,icon_FILE,"file_fff");
			break;
		case menuitem_QUIT:
			Desk_Event_CloseDown();
			break;
	}
}

void Error(void)
{
	Desk_Hourglass_Off();
	Desk_Error_Report(1,"Unable to open file: %s",((Desk_os_error *)_kernel_last_oserror())->errmess);
	Desk_Event_CloseDown();
}

void SaveFile(char *filename)
{
	FILE *file;
	char fontname[256],filename2[300],oldfontname[255];
	int counter,count=0,max,current=1,tick,onefont;
	Desk_Hourglass_On();
	max=atoi(splitnumber);
	Desk_Menu_GetFlags(menu,menuitem_SPLIT,&tick,NULL);
	Desk_Menu_GetFlags(menu,menuitem_ONEFONT,&onefont,NULL);
	if (tick) {
		Desk_SWI(5,0,Desk_SWI_OS_File,8,filename,NULL,NULL,0);
		strcpy(filename2,filename);
		strcat(filename2,".1");
		if ((file=fopen(filename2,"w"))==NULL) Error();
	} else {
    	if ((file=fopen(filename,"w"))==NULL) Error();
    }
	Desk_SWI(7,3,0x40091,NULL,fontname,0x20000,256,fontname,256,0,NULL,NULL,&counter); /*Font_ListFonts*/
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
		Desk_SWI(7,3,0x40091,NULL,fontname,counter,256,fontname,256,0,NULL,NULL,&counter); /*Font_ListFonts*/
		if (count>=max && tick) {
			count=0;
			fclose(file);
			sprintf(filename2,"%s.%d",filename,++current);
			if ((file=fopen(filename2,"w"))==NULL) Error();
		}

	}
	fclose(file);
	Desk_Hourglass_Off();
}

void DragEnded(void *r)
{
	Desk_mouse_block mouseblk;
	Desk_message_block msgblk;
	char leafname[256];
	Desk_Wimp_GetPointerInfo(&mouseblk);
	msgblk.header.size=56;
	msgblk.header.yourref=0;
	msgblk.header.action=Desk_message_DATASAVE;
	msgblk.data.datasave.window=mouseblk.window;
	msgblk.data.datasave.icon=mouseblk.icon;
	msgblk.data.datasave.pos=mouseblk.pos;
	msgblk.data.datasave.estsize=1024;
	msgblk.data.datasave.filetype=SAVEFILETYPE;
	Desk_Filing_GetLeafname(Desk_Icon_GetTextPtr(mainwin,icon_WRITE),leafname);
	if (strlen(leafname)>10) leafname[10]='\0';
	strcpy(msgblk.data.datasave.leafname,leafname);
	Desk_Wimp_SendMessage(Desk_event_USERMESSAGERECORDED,&msgblk,mouseblk.window,mouseblk.icon);
}

Desk_bool DataSaveAck(Desk_event_pollblock *block,void *r)
{
	SaveFile(block->data.message.data.datasaveack.filename);
	block->data.message.header.yourref=block->data.message.header.myref;
	block->data.message.header.action=Desk_message_DATALOAD;
	Desk_Wimp_SendMessage(Desk_event_USERMESSAGERECORDED,&block->data.message,block->data.message.header.sender,0);
	return Desk_TRUE;
}

Desk_bool DataLoadAck(Desk_event_pollblock *block,void *r)
{
	Desk_Event_CloseDown();
	return Desk_TRUE;
}

Desk_bool MessageAck(Desk_event_pollblock *block,void *r)
{
	switch (block->data.message.header.action) {
		case Desk_message_DATASAVE:
			/*Do nothing*/
			break;
		case Desk_message_DATALOAD:
			Desk_Error_Report(1,"Data transfer failed: Reciever died");
			break;
		default:
			return Desk_FALSE;
	}
	return Desk_TRUE;
}

Desk_bool FileDragged(Desk_event_pollblock *block,void *r)
{
	if (block->data.mouse.button.data.dragselect==Desk_FALSE && block->data.mouse.button.data.dragadjust==Desk_FALSE) return Desk_FALSE;
	Desk_Icon_StartSolidDrag(block->data.mouse.window,icon_FILE);
	Desk_Drag_SetHandlers(NULL,DragEnded,r);
	return Desk_TRUE;
}

Desk_bool OkPressed(Desk_event_pollblock *block,void *r)
{
	char *filename;
	if (block->data.mouse.button.data.menu) return Desk_FALSE;
	filename=Desk_Icon_GetTextPtr(mainwin,icon_WRITE);
	if (strlen(filename)<1 || strpbrk(filename,".:")==NULL) {
		Desk_Error_Report(1,"Enter a name for the file, then drag the icon to a directory display");
		return Desk_TRUE;
	}
	SaveFile(filename);
	Desk_Event_CloseDown();
	return Desk_TRUE;
}

Desk_bool CancelPressed(Desk_event_pollblock *block,void *r)
{
	if (block->data.mouse.button.data.menu) return Desk_FALSE;
	Desk_Event_CloseDown();
	return Desk_TRUE;
}

Desk_bool KeyPressed(Desk_event_pollblock *block,void *r)
{
	char *filename;
	switch (block->data.key.code) {
		case Desk_keycode_RETURN:
			filename=Desk_Icon_GetTextPtr(mainwin,icon_WRITE);
			if (strlen(filename)<1 || strpbrk(filename,".:")==NULL) {
				Desk_Error_Report(1,"Enter a name for the file, then drag the icon to a directory display");
				return Desk_TRUE;
			}
            Desk_Icon_SetSelect(mainwin,icon_SAVE,Desk_TRUE);
			SaveFile(filename);
			Desk_Event_CloseDown();
			break;
		case Desk_keycode_ESCAPE:
			Desk_Icon_SetSelect(mainwin,icon_CANCEL,Desk_TRUE);
			Desk_Event_CloseDown();
			break;
		default:
			return Desk_FALSE;
	}
	return Desk_TRUE;
}

int main(void)
{
	AJWLib_Error2_Init();
	Desk_Resource_Initialise(DIRPREFIX);
	Desk_Msgs_LoadFile("Messages");
	Desk_Event_Initialise(AJWLib_Msgs_TempLookup("Task.Name:"));
	Desk_EventMsg_Initialise();
	Desk_Screen_CacheModeInfo();
	Desk_Template_Initialise();
	Desk_EventMsg_Claim(Desk_message_MODECHANGE,Desk_event_ANY,Desk_Handler_ModeChange,NULL);
	Desk_Event_Claim(Desk_event_CLOSE,Desk_event_ANY,Desk_event_ANY,Desk_Handler_CloseWindow,NULL);
	Desk_Event_Claim(Desk_event_OPEN,Desk_event_ANY,Desk_event_ANY,Desk_Handler_OpenWindow,NULL);
	Desk_Event_Claim(Desk_event_KEY,Desk_event_ANY,Desk_event_ANY,Desk_Handler_Key,NULL);
	Desk_Event_Claim(Desk_event_REDRAW,Desk_event_ANY,Desk_event_ANY,Desk_Handler_HatchRedraw,NULL);
	Desk_Template_LoadFile("Templates");
	mainwin=Desk_Window_CreateAndShow("Main",Desk_template_TITLEMIN,Desk_open_CENTERED);
	AJWLib_Msgs_SetTitle(mainwin,"Win.Title:");
	AJWLib_Msgs_SetText(mainwin,icon_CANCEL,"Icon.Cancel");
	AJWLib_Msgs_SetText(mainwin,icon_SAVE,"Icon.Save");
	AJWLib_Msgs_SetText(mainwin,icon_WRITE,"Icon.Write");
	Desk_Icon_SetCaret(mainwin,icon_WRITE);
	info=AJWLib_Window_CreateInfoWindowFromMsgs("Task.Name:","Task.Purpose:","© Alex Waugh 1999",VERSION);
	menu=AJWLib_Menu_CreateFromMsgs("Menu.Title:","Menu.Body:",MenuClick,NULL);
	submenu=AJWLib_Menu_CreateFromMsgs("Sub.Title:","Sub.Body:",SubMenuClick,NULL);
	Desk_Menu_MakeWritable(submenu,0,splitnumber,5,"A0-9");
	Desk_Menu_AddSubMenu(menu,menuitem_INFO,(Desk_menu_ptr)info);
	Desk_Menu_AddSubMenu(menu,menuitem_SPLIT,submenu);
	AJWLib_Menu_Attach(mainwin,Desk_event_ANY,menu,Desk_button_MENU);
	Desk_Menu_SetFlags(menu,menuitem_ONEFONT,Desk_TRUE,Desk_FALSE);
	Desk_Drag_Initialise(Desk_FALSE);
	Desk_Event_Claim(Desk_event_CLICK,mainwin,icon_FILE,FileDragged,NULL);
	Desk_Event_Claim(Desk_event_CLICK,mainwin,icon_SAVE,OkPressed,NULL);
	Desk_Event_Claim(Desk_event_CLICK,mainwin,icon_CANCEL,CancelPressed,NULL);
	Desk_Event_Claim(Desk_event_KEY,mainwin,Desk_event_ANY,KeyPressed,NULL);
	Desk_Event_Claim(Desk_event_USERMESSAGEACK,Desk_event_ANY,Desk_event_ANY,MessageAck,NULL);
	Desk_EventMsg_Claim(Desk_message_DATASAVEACK,Desk_event_ANY,DataSaveAck,NULL);
	Desk_EventMsg_Claim(Desk_message_DATALOADACK,Desk_event_ANY,DataLoadAck,NULL);
	while (Desk_TRUE) Desk_Event_Poll();
	return 0;
}

