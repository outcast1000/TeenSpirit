//	/*
// 	*
// 	* Copyright (C) 2003-2010 Alexandros Economou
//	*
//	* This file is part of Jaangle (http://www.jaangle.com)
// 	*
// 	* This Program is free software; you can redistribute it and/or modify
// 	* it under the terms of the GNU General Public License as published by
// 	* the Free Software Foundation; either version 2, or (at your option)
// 	* any later version.
// 	*
// 	* This Program is distributed in the hope that it will be useful,
// 	* but WITHOUT ANY WARRANTY; without even the implied warranty of
// 	* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// 	* GNU General Public License for more details.
// 	*
// 	* You should have received a copy of the GNU General Public License
// 	* along with GNU Make; see the file COPYING. If not, write to
// 	* the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
// 	* http://www.gnu.org/copyleft/gpl.html
// 	*
//	*/ 
#ifndef IStateChangeListener_h_
#define IStateChangeListener_h_

//=== The Interface implementation that must inherit all the controls that listen to state changes
class IStateChangeListener
{
public:
	IStateChangeListener()												{}
	virtual ~IStateChangeListener()										{}
	virtual BOOL OnStateChanged(UINT stateMessage)						= 0;
	virtual LPCTSTR GetStateListenerName()								= 0;

};

//=== The application's event bus (StateManager)
class IStateManager
{
public:
	virtual ~IStateManager()											{}
	virtual void RegisterStateChangeListener(IStateChangeListener& listener)	= 0;
	virtual void UnRegisterStateChangeListener(IStateChangeListener& listener)	= 0;
	//Broadcasts now on the main thread; from another thread it is posted.
	virtual void SendMessage(UINT stateMessage)							= 0;
	//Broadcast on the main thread's next HeartBeat (duplicates are merged).
	virtual void PostMessage(UINT stateMessage)							= 0;
};

#endif
