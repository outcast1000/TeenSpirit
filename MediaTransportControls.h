#pragma once

//	Windows media controls (System Media Transport Controls, Windows 10+): the media
//	overlay, lock screen and taskbar show the current track, and the keyboard media
//	keys reach the program through Windows (instead of a global hotkey).
//
//	All calls are made from the UI thread. Button presses are posted to the window
//	given to Init as WM_HOTKEY with wParam = VK_MEDIA_PLAY_PAUSE / _STOP / _NEXT_TRACK /
//	_PREV_TRACK, the same message the multimedia hotkeys produce.
//	On older Windows Init fails and every other call does nothing.

#include <memory>
#include "MediaPlayerEngine.h"	//PlayState

class MediaTransportControls
{
public:
	MediaTransportControls();
	~MediaTransportControls();

	//TRUE when Windows has media controls for desktop programs (Windows 10+).
	static BOOL IsSupported();

	BOOL Init(HWND hwnd);
	void Close();
	BOOL IsActive() const	{return m_state != nullptr;}

	void SetPlayState(PlayState ps);
	//Any argument can be NULL. The picture (a local image file) is loaded in the background.
	void SetTrack(LPCWSTR title, LPCWSTR artist, LPCWSTR album, LPCWSTR picturePath);
	void ClearTrack();

private:
	MediaTransportControls(const MediaTransportControls&);
	MediaTransportControls& operator=(const MediaTransportControls&);

	struct State;
	std::shared_ptr<State> m_state;
};
