#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Media.h>
#include <winrt/Windows.Media.Control.h>

#include <evglib>

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


class GroupSignal
{
public:
	bool ready = false;
	std::mutex m;
	std::condition_variable c;

	void signal() noexcept
	{
		{
			std::unique_lock<std::mutex> lock(m);
			ready = true;
		}
		c.notify_one();
	}

	void watch()
	{
		std::unique_lock<std::mutex> lock(m);
		while (!ready)
		{
			c.wait(lock);
			if (!ready)
			{
				std::cout << "UNLOCK WITHOUT READY\n";
			}
		}
	}

	~GroupSignal()
	{
		std::unique_lock<std::mutex> lock(m);
	}
};




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







struct AsyncSleeper : std::suspend_always
{
public:
	boost::asio::high_resolution_timer timer;

	AsyncSleeper(std::chrono::milliseconds _time) : timer(threads, _time) {}

	void await_suspend(std::coroutine_handle<> handle)
	{
		timer.async_wait([handle](const std::error_code e) 
			{ 
				if (e) { throw std::runtime_error(e.message()); }
				handle(); 
			});
	}
};


auto AsyncSleep(std::chrono::milliseconds time)
{
	return AsyncSleeper(time);
}

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
			//std::copy(std::begin(in), std::end(in), std::begin(title));
			//std::cout << adapter.get_u8() << '\n';

			//auto f = std::ofstream("C:\\Users\\nickk\\source\\repos\\AdvancedDiscordStatus\\x64\\Debug");
			//f << adapter.get_u8();
		}
		else
		{
			std::cout << "Last error: " << GetLastError() << '\n';
		}





		Windows::Media::Control::GlobalSystemMediaTransportControlsSessionManager manager(nullptr);
		auto sessions = co_await manager.RequestAsync();
		auto currentSession = sessions.GetCurrentSession();
		assert(currentSession);
		auto info = co_await currentSession.TryGetMediaPropertiesAsync();

		audioTitle = "Listening to ";
		utf16ToUtf8(info.Artist(), audioTitle);
		audioTitle += " - ";
		utf16ToUtf8(info.Title(), audioTitle);
		std::cout << windowTitle.get_u8() << '\n';
		std::cout << audioTitle << '\n';




		
		memset(&activity, 0, sizeof(activity));
		activity.type = DiscordActivityType_Playing;
		//activity.application_id = 684941486873378867;
		memcpy(&activity.details, windowTitle.get_u8().data(), windowTitle.get_u8().size());
		memcpy(&activity.state, audioTitle, audioTitle.size());
		memcpy(&activity.assets.large_image, "chrome", 7);

		IDiscordActivityManager* dmanager = app.core->get_activity_manager(app.core);
		dmanager->update_activity(dmanager, &activity, nullptr, nullptr);

		app.core->run_callbacks(app.core);


		co_await AsyncSleep(1s);
	}
}



int main(int argc, char** argv)
{
	threads.init();
	//thisProgram.setArgs(argc, argv);

	std::cout << "AdvancedDiscordStatus by VCInventerman\n";

	Task p = getCurrentMedia();
	p.signal.watch();
	

	std::cout << "Hello World!\n";

	threads.stop();
}