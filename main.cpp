#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Media.h>
#include <winrt/Windows.Media.Control.h>

#define EVG_LIB_DISCORD

#include <evglib>
#include <evergreen/WideStringIntake.h>

using namespace evg;
using namespace winrt;

using namespace std::literals::chrono_literals;




struct Application {
	struct IDiscordCore* core = nullptr;
	struct IDiscordUsers* users = nullptr;
};




#define CHECK_HR(fun) fun
#define SAFE_RELEASE(ptr) if (ptr) \
							{ptr->Release(); ptr = nullptr;}
HRESULT EnumSessions(IAudioSessionManager2* pSessionManager)
{
	if (!pSessionManager)
	{
		return E_INVALIDARG;
	}

	HRESULT hr = S_OK;

	int cbSessionCount = 0;
	LPWSTR pswSession = NULL;

	IAudioSessionEnumerator* pSessionList = NULL;
	IAudioSessionControl* pSessionControl = NULL;

	// Get the current list of sessions.
	CHECK_HR(hr = pSessionManager->GetSessionEnumerator(&pSessionList));

	// Get the session count.
	CHECK_HR(hr = pSessionList->GetCount(&cbSessionCount));

	for (int index = 0; index < cbSessionCount; index++)
	{
		CoTaskMemFree(pswSession);
		SAFE_RELEASE(pSessionControl);

		// Get the <n>th session.
		CHECK_HR(hr = pSessionList->GetSession(index, &pSessionControl));

		CHECK_HR(hr = pSessionControl->GetDisplayName(&pswSession));

		wprintf_s(L"Session Name: %s\n", pswSession);
	}

	CoTaskMemFree(pswSession);
	SAFE_RELEASE(pSessionControl);
	SAFE_RELEASE(pSessionList);

	return hr;

}






template<typename RetT = void>
struct Task
{

	struct VoidResultHolder
	{
		

		void return_void()
		{
		}
	};

	struct ResultHolder
	{
		
		RetT result;

		void return_value(RetT val)
		{
			result = val;
		}
	};

	struct promise_type : public std::conditional<std::is_same<RetT, void>::value, VoidResultHolder, ResultHolder>::type
	{
		GroupSignal* signal = nullptr;

		Task get_return_object() { return Task(*this); }
		std::suspend_never initial_suspend() { return {}; }
		std::suspend_never final_suspend() noexcept 
		{ 
			signal->signal();
			return {}; 
		}
		void unhandled_exception() {}
	};


	GroupSignal signal;

	Task(promise_type& _promise)
	{
		_promise.signal = &signal;
	}
	Task(const Task&) = delete;
	Task& operator= (const Task& rhs)
	{
		return std::move(rhs);
	}

	promise_type promise;
};


template<typename T>
class Awaitable
{
public:

};









Task<int> it()
{
	for (int i = 0;i < 5;++i)
	{
		std::cout << "Hey " << i << '\n';
		co_await AsyncSleep(1s);
	}
	
	co_return 6;
}


Task<void> getCurrentMedia()
{
	WStringAdapter windowTitle;
	StringBuilder audioTitle;

	Application app;
	IDiscordCoreEvents events;
	memset(&events, 0, sizeof(events));
	DiscordCreateParams params;
	params.client_id = 845330151029211207;
	params.flags = DiscordCreateFlags_Default;
	params.events = &events;
	params.event_data = &app;

	DiscordActivity activity;

	DiscordCreate(DISCORD_VERSION, &params, &app.core);

	for (Size i = 0;; ++i)
	{
		HWND currentWindow = GetForegroundWindow();

		if (GetWindowTextW(currentWindow, windowTitle.in_u16(), windowTitle.size_u16() - 1))
		{
			Windows::Media::Control::GlobalSystemMediaTransportControlsSessionManager manager(nullptr);
			auto sessions = co_await manager.RequestAsync();
			auto currentSession = sessions.GetCurrentSession();
			assert(currentSession);
			auto info = co_await currentSession.TryGetMediaPropertiesAsync();

			audioTitle = "Listening to ";
			utf16ToUtf8(info.Artist(), audioTitle);
			audioTitle += " - ";
			utf16ToUtf8(info.Title(), audioTitle);

			StringBuilderBase<WChar> audioTitleTerminal;
			utf8ToUtf16(audioTitle, audioTitleTerminal);


			//std::wcout << windowTitle.get_u8() << '\n';
			//std::wcout << audioTitle << '\n';

			std::wcout << windowTitle.get_u16().data() << '\n';
			std::wcout << audioTitleTerminal.data() << '\n';





			memset(&activity, 0, sizeof(activity));
			activity.type = DiscordActivityType_Playing;
			//activity.application_id = 684941486873378867;
			memcpy(&activity.details, windowTitle.get_u8().data(), windowTitle.get_u8().size());
			memcpy(&activity.state, audioTitle, audioTitle.size());
			memcpy(&activity.assets.large_image, "chrome", 7);

			IDiscordActivityManager* dmanager = app.core->get_activity_manager(app.core);
			dmanager->update_activity(dmanager, &activity, nullptr, nullptr);

			app.core->run_callbacks(app.core);
		}
		else
		{
			std::wcout << "Last error: " << GetLastError() << '\n';
		}

		co_await AsyncSleep(1s);
	}
}



int main(int argc, char** argv)
{
	evgProgramBegin(argc, argv, "AdvancedDiscordStatus", SemVer("1.0.0"));

	

	/*CONSOLE_FONT_INFOEX cfi;
	cfi.cbSize = sizeof cfi;
	cfi.nFont = 0;
	cfi.dwFontSize.X = 0;
	cfi.dwFontSize.Y = 16;
	cfi.FontFamily = FF_DONTCARE;
	cfi.FontWeight = FW_NORMAL;
	wcscpy_s(cfi.FaceName, L"Consolas");
	SetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &cfi);



	_setmode(_fileno(stdout), _O_U16TEXT);
	*///_setmode(_fileno(stdin), _O_U16TEXT);
	//SetConsoleOutputCP(1200);

	_setmode(_fileno(stdout), _O_U16TEXT);
	
	//std::cout << "AdvancedDiscordStatus by VCInventerman\n";

	Task p = getCurrentMedia();
	p.signal.watch();
	

	//std::cout << "Hello World!\n";

	threads.stop();
}

