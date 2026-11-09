#include "stdafx.h"
#include "MediaTransportControls.h"
#include <mutex>
#include <roapi.h>
#include <wrl/client.h>
#include <wrl/event.h>
#include <wrl/wrappers/corewrappers.h>
#include <windows.foundation.h>
#include <windows.media.h>
#include <windows.storage.h>
#include <windows.storage.streams.h>

//From SystemMediaTransportControlsInterop.h, which only declares it when targeting
//Windows 10 (the program targets Windows 7 and checks for it at run time).
MIDL_INTERFACE("ddb0472d-c911-4a1f-86d9-dc3d71a95f5a")
ISystemMediaTransportControlsInterop : public IInspectable
{
public:
	virtual HRESULT STDMETHODCALLTYPE GetForWindow(HWND appWindow, REFIID riid, void** mediaTransportControl) = 0;
};

//The WinRT DLLs are delay-loaded (see TeenSpirit.vcxproj), so nothing here may run
//before WinRTAvailable() has confirmed they exist: on Windows 7 they don't.

namespace WRL = Microsoft::WRL;
namespace AF = ABI::Windows::Foundation;
namespace AM = ABI::Windows::Media;
namespace AS = ABI::Windows::Storage;
namespace ASS = ABI::Windows::Storage::Streams;
using Microsoft::WRL::ComPtr;
using Microsoft::WRL::Wrappers::HStringReference;

namespace
{
	BOOL WinRTAvailable()
	{
		static INT s_available = -1;
		if (s_available == -1)
		{
			HMODULE h = LoadLibraryW(L"combase.dll");
			s_available = (h != NULL && GetProcAddress(h, "RoGetActivationFactory") != NULL &&
				GetProcAddress(h, "WindowsCreateStringReference") != NULL) ? 1 : 0;
		}
		return s_available == 1;
	}

	template<class T>
	HRESULT GetFactory(LPCWSTR runtimeClass, ComPtr<T>& factory)
	{
		return RoGetActivationFactory(HStringReference(runtimeClass).Get(), IID_PPV_ARGS(&factory));
	}

	HStringReference Str(LPCWSTR s)
	{
		return HStringReference(s != NULL ? s : L"");
	}

	typedef AF::ITypedEventHandler<AM::SystemMediaTransportControls*,
		AM::SystemMediaTransportControlsButtonPressedEventArgs*> ButtonPressedHandler;
	typedef AF::IAsyncOperationCompletedHandler<AS::StorageFile*> FileOpenedHandler;
}

struct MediaTransportControls::State
{
	State(): hwnd(NULL), generation(0)	{buttonToken.value = 0;}
	std::mutex mx;	//the thumbnail is set from a thread-pool thread
	HWND hwnd;
	ComPtr<AM::ISystemMediaTransportControls> smtc;
	ComPtr<AM::ISystemMediaTransportControlsDisplayUpdater> updater;
	EventRegistrationToken buttonToken;
	UINT generation;	//bumped by every SetTrack; late thumbnails of older tracks are dropped
};

MediaTransportControls::MediaTransportControls()
{
}

MediaTransportControls::~MediaTransportControls()
{
	Close();
}

BOOL MediaTransportControls::IsSupported()
{
	static INT s_supported = -1;
	if (s_supported == -1)
	{
		s_supported = 0;
		if (WinRTAvailable())
		{
			ComPtr<ISystemMediaTransportControlsInterop> interop;
			if (SUCCEEDED(GetFactory(RuntimeClass_Windows_Media_SystemMediaTransportControls, interop)))
				s_supported = 1;
		}
	}
	return s_supported == 1;
}

BOOL MediaTransportControls::Init(HWND hwnd)
{
	Close();
	if (hwnd == NULL || !IsSupported())
		return FALSE;
	ComPtr<ISystemMediaTransportControlsInterop> interop;
	if (FAILED(GetFactory(RuntimeClass_Windows_Media_SystemMediaTransportControls, interop)))
		return FALSE;
	std::shared_ptr<State> st = std::make_shared<State>();
	st->hwnd = hwnd;
	if (FAILED(interop->GetForWindow(hwnd, IID_PPV_ARGS(&st->smtc))) ||
		FAILED(st->smtc->get_DisplayUpdater(&st->updater)))
	{
		TRACE(_T("@1 MediaTransportControls::Init. Media controls unavailable\r\n"));
		return FALSE;
	}

	//Raised on a background thread: just forward the button to the UI thread.
	ComPtr<ButtonPressedHandler> handler = WRL::Callback<ButtonPressedHandler>(
		[hwnd](AM::ISystemMediaTransportControls*, AM::ISystemMediaTransportControlsButtonPressedEventArgs* args) -> HRESULT
	{
		AM::SystemMediaTransportControlsButton button;
		if (args == NULL || FAILED(args->get_Button(&button)))
			return S_OK;
		WPARAM vk = 0;
		switch (button)
		{
		case AM::SystemMediaTransportControlsButton_Play:
		case AM::SystemMediaTransportControlsButton_Pause:
			vk = VK_MEDIA_PLAY_PAUSE;
			break;
		case AM::SystemMediaTransportControlsButton_Stop:
			vk = VK_MEDIA_STOP;
			break;
		case AM::SystemMediaTransportControlsButton_Next:
			vk = VK_MEDIA_NEXT_TRACK;
			break;
		case AM::SystemMediaTransportControlsButton_Previous:
			vk = VK_MEDIA_PREV_TRACK;
			break;
		}
		if (vk != 0)
			::PostMessage(hwnd, WM_HOTKEY, vk, 0);
		return S_OK;
	});
	if (handler == nullptr || FAILED(st->smtc->add_ButtonPressed(handler.Get(), &st->buttonToken)))
		return FALSE;

	st->smtc->put_IsPlayEnabled(true);
	st->smtc->put_IsPauseEnabled(true);
	st->smtc->put_IsStopEnabled(true);
	st->smtc->put_IsNextEnabled(true);
	st->smtc->put_IsPreviousEnabled(true);
	st->smtc->put_PlaybackStatus(AM::MediaPlaybackStatus_Closed);
	st->updater->put_Type(AM::MediaPlaybackType_Music);
	st->smtc->put_IsEnabled(true);
	m_state = st;
	return TRUE;
}

void MediaTransportControls::Close()
{
	if (m_state == nullptr)
		return;
	std::shared_ptr<State> st = m_state;
	m_state.reset();
	std::lock_guard<std::mutex> lock(st->mx);
	st->generation++;
	if (st->buttonToken.value != 0)
		st->smtc->remove_ButtonPressed(st->buttonToken);
	st->updater->ClearAll();
	st->updater->Update();
	st->smtc->put_IsEnabled(false);
	st->updater.Reset();
	st->smtc.Reset();
}

void MediaTransportControls::SetPlayState(PlayState ps)
{
	if (m_state == nullptr)
		return;
	AM::MediaPlaybackStatus status;
	switch (ps)
	{
	case PS_Playing:	status = AM::MediaPlaybackStatus_Playing;	break;
	case PS_Paused:		status = AM::MediaPlaybackStatus_Paused;	break;
	case PS_Stopped:	status = AM::MediaPlaybackStatus_Stopped;	break;
	case PS_Preparing:
	case PS_Stalled:	status = AM::MediaPlaybackStatus_Changing;	break;
	default:			status = AM::MediaPlaybackStatus_Closed;	break;
	}
	m_state->smtc->put_PlaybackStatus(status);
}

void MediaTransportControls::SetTrack(LPCWSTR title, LPCWSTR artist, LPCWSTR album, LPCWSTR picturePath)
{
	if (m_state == nullptr)
		return;
	std::shared_ptr<State> st = m_state;
	UINT generation = 0;
	{
		std::lock_guard<std::mutex> lock(st->mx);
		generation = ++st->generation;
		st->updater->ClearAll();
		st->updater->put_Type(AM::MediaPlaybackType_Music);
		ComPtr<AM::IMusicDisplayProperties> music;
		if (SUCCEEDED(st->updater->get_MusicProperties(&music)))
		{
			music->put_Title(Str(title).Get());
			music->put_Artist(Str(artist).Get());
			ComPtr<AM::IMusicDisplayProperties2> music2;
			if (album != NULL && album[0] != 0 && SUCCEEDED(music.As(&music2)))
				music2->put_AlbumTitle(Str(album).Get());
		}
		st->updater->Update();
	}
	if (picturePath == NULL || picturePath[0] == 0)
		return;

	//The cover goes in when the file has been opened (asynchronously), if the track hasn't changed since.
	ComPtr<AS::IStorageFileStatics> files;
	ComPtr<AF::IAsyncOperation<AS::StorageFile*>> op;
	if (FAILED(GetFactory(RuntimeClass_Windows_Storage_StorageFile, files)) ||
		FAILED(files->GetFileFromPathAsync(Str(picturePath).Get(), &op)))
		return;
	std::weak_ptr<State> weak = st;
	ComPtr<FileOpenedHandler> opened = WRL::Callback<FileOpenedHandler>(
		[weak, generation](AF::IAsyncOperation<AS::StorageFile*>* op, AsyncStatus status) -> HRESULT
	{
		ComPtr<AS::IStorageFile> file;
		if (status != AsyncStatus::Completed || FAILED(op->GetResults(&file)))
			return S_OK;
		ComPtr<ASS::IRandomAccessStreamReferenceStatics> refs;
		ComPtr<ASS::IRandomAccessStreamReference> thumbnail;
		if (FAILED(GetFactory(RuntimeClass_Windows_Storage_Streams_RandomAccessStreamReference, refs)) ||
			FAILED(refs->CreateFromFile(file.Get(), &thumbnail)))
			return S_OK;
		std::shared_ptr<State> st = weak.lock();
		if (st == nullptr)
			return S_OK;
		std::lock_guard<std::mutex> lock(st->mx);
		if (st->generation == generation && st->updater != nullptr)
		{
			st->updater->put_Thumbnail(thumbnail.Get());
			st->updater->Update();
		}
		return S_OK;
	});
	if (opened != nullptr)
		op->put_Completed(opened.Get());
}

void MediaTransportControls::ClearTrack()
{
	if (m_state == nullptr)
		return;
	std::lock_guard<std::mutex> lock(m_state->mx);
	m_state->generation++;
	m_state->updater->ClearAll();
	m_state->updater->Update();
	m_state->smtc->put_PlaybackStatus(AM::MediaPlaybackStatus_Closed);
}
