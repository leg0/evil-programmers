/*
    Replace plugin for DialogManager
    Copyright (C) 2005 Shynkarenko Ivan aka 4ekucT

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#ifndef __CM_HPP__
#define __CM_HPP__

#include "../../dm_module.hpp"

enum
{
  mName,
  mOK,
  mCancel,
  mReplaceCaption,
  mSearch,
  mReplace,
  mCase,
  mCurPos,
  mSelect,
};

struct InitDialogItem
{
 int Type;
 int X1,Y1,X2,Y2;
 int Focus;
 int Selected;
 DWORD Flags;
 int DefaultButton;
 const char *Data;
 const char *History;
};

void InitDialogItems(const struct InitDialogItem *Init,struct FarDialogItem *Item,int ItemsNumber);

extern PluginStartupInfo FarInfo;
extern FARSTANDARDFUNCTIONS FSF;
extern DialogPluginStartupInfo DialogInfo;
extern char *GetMsg(int MsgNum,char *Str);

#endif
