/*
	ListFonts
	©Alex Waugh 1999

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
	
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

	$Id: !RunImage.c,v 1.3 2000-02-19 13:43:09 uid1 Exp $
	
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

#define VERSION "1.01 (19-Feb-00)"
#define DIRPREFIX "ListFonts"

#define icon_SAVE   0
#define icon_CANCEL 1
#define icon_WRITE  2
#define icon_FILE   3

#define menuitem_INFO    0
#define menuitem_ONEFONT 1
#define menuitem_SPLIT   2
#define menuitem_TEXT    3
#define menuitem_QUIT    4

#define SAVEFILETYPE 0xFFF

#define Font_ListFonts 0x40091

/*	Variables  */

Desk_window_handle mainwin,info;
Desk_menu_ptr menu,submenu,textmenu;
char splitnumber[5]="249";
char textused[256];

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
	char fontname[256],filename2[300],oldfontname[255],*ch;
	int counter,count=0,max,current=1,tick,onefont;
	Desk_Hourglass_On();
	max=atoi(splitnumber);
	if ((ch=strchr(textused,'%'))!=strrchr(textused,'%')) { Desk_Msgs_Report(1,"Error.BadStr:","%%","%%s"); return;}
	if (ch) if (ch[1]!='s') { Desk_Msgs_Report(1,"Error.BadStr:","%%","%%s"); return; }
	Desk_Menu_GetFlags(menu,menuitem_SPLIT,&tick,NULL);
	Desk_Menu_GetFlags(menu,menuitem_ONEFONT,&onefont,NULL);
	if (tick) {
		Desk_SWI(5,0,Desk_SWI_OS_File,8,filename,NULL,NULL,0);
		strcpy(filename2,filename);
		strcat(filename2,".01");
		if ((file=fopen(filename2,"w"))==NULL) Error();
	} else {
    	if ((file=fopen(filename,"w"))==NULL) Error();
    }
	Desk_SWI(7,3,Font_ListFonts,NULL,fontname,0x20000,256,fontname,256,0,NULL,NULL,&counter);
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
				fprintf(file,"{font %s}",fontname);
				fprintf(file,textused,newfontname);
				fprintf(file,"{font}\n");
			}
		} else {
			fprintf(file,"{font %s}",fontname);
			fprintf(file,textused,fontname);
			fprintf(file,"{font}\n");
			count++;
		}
		Desk_SWI(7,3,Font_ListFonts,NULL,fontname,counter,256,fontname,256,0,NULL,NULL,&counter);
		if (count>=max && tick) {
			count=0;
			fclose(file);
			sprintf(filename2,"%s.%.2d",filename,++current);
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
	Desk_Filing_GetLeafname(Desk_Icon_GetTextPtr(mainwin,icon_WRITE),leafname);
	msgblk.header.size=45+strlen(leafname);
	msgblk.header.size=(msgblk.header.size+3) & ~3; /*Word align*/
	msgblk.header.yourref=0;
	msgblk.header.action=Desk_message_DATASAVE;
	msgblk.data.datasave.window=mouseblk.window;
	msgblk.data.datasave.icon=mouseblk.icon;
	msgblk.data.datasave.pos=mouseblk.pos;
	msgblk.data.datasave.estsize=1024;
	msgblk.data.datasave.filetype=SAVEFILETYPE;
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
	textmenu=AJWLib_Menu_CreateFromMsgs("Text.Title:","Text.Default:",NULL,NULL);
	Desk_Menu_MakeWritable(textmenu,0,textused,256,"");
	Desk_Msgs_Lookup("Text.Default:",textused,255);
	Desk_Menu_AddSubMenu(menu,menuitem_INFO,(Desk_menu_ptr)info);
	Desk_Menu_AddSubMenu(menu,menuitem_SPLIT,submenu);
	Desk_Menu_AddSubMenu(menu,menuitem_TEXT,textmenu);
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

