//////////////////////////////////////////////////////////////////////////
//	py.js - Node.js/Python Bridge; Node.js-hosted Python.
//	Copyright (C) 2019  Michael Brown
//
//	This program is free software: you can redistribute it and/or modify
//	it under the terms of the GNU Affero General Public License as
//	published by the Free Software Foundation, either version 3 of the
//	License, or (at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU Affero General Public License for more details.
//
//	You should have received a copy of the GNU Affero General Public License
//	along with this program.  If not, see <https://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////////

#include "pyjs_.h"
#include "structmember.h"
#include "napi_callback.hpp"

//////////////////////////////////////////////////////////////////////////

static uv_loop_t* _node_event_loop;

////////////////////////////////////////////
// Python Loop Controller
////////////////////////////////////////////

static std::atomic<bool> exiting(false);
static pyjs_async::python_loop ploop{};
static std::mutex _python_message_queue_mutex;
static std::vector<pyjs_async::PythonNodeAsyncMessage> python_message_queue{};

static void python_switching_handler(uv_timer_t* _handle)
{
	//Make Python do any work it has outstanding on other threads.
	Py_BEGIN_ALLOW_THREADS
	Py_END_ALLOW_THREADS
}

static void node_to_python_message_handler(uv_async_t* _handle)
{
	{
		std::lock_guard<std::mutex> lock(_python_message_queue_mutex);

		if (python_message_queue.size() < 1)
		{
			return;
		}
	}

	std::vector<pyjs_async::PythonNodeAsyncMessage> _queue;

	{
		std::lock_guard<std::mutex> lock(_python_message_queue_mutex);
		_queue.swap(python_message_queue);
	}

	for (const auto& ele : _queue)
	{
		//One time async invocation of Python function.
		//Return value is sent to node, if requested, through async callback.
		if (ele.msg_type == pyjs_async::PythonNodeAsyncMessageType::FunctionCall)
		{

			PyObject* pyObject = ele.arguments[0]; //remote function object
			PyObject* args = ele.arguments[1];
			PyObject* dict = ele.arguments[2];
			PyObject* ret;

			{
				lock_gil lock_me;
				ret = PyObject_Call(pyObject, args, dict); //PyObject_Call (New)

				Py_DECREF(pyObject); //cloned for async in (FunctionCallAsync)
				Py_DECREF(args); //new from (ProcessFunctionCallArguments)
				Py_DECREF(dict); //new from (ProcessFunctionCallArguments)
			}

			if (ele.callback != nullptr)
			{
				ele.callback->call([ret](Napi::Env env, std::vector<napi_value>& args)
				{
					if (ret == NULL)
					{
						throw std::runtime_error("Error in async callback:\n" );
							//+ pyjs_utils::GetPythonException());
					}

					auto map = std::unique_ptr<std::unordered_map<PyObject*,napi_value>>
						(new std::unordered_map<PyObject*,napi_value>());

					auto js = pyjs::Py_ConvertToJavascript(env, ret,
						pyjs::PyjsConfigurationOptions::GetSerializationFilters(),
						map, pyjs::MarshallingOptions());

					Py_DECREF(ret);

					args = { js };

				});
			}
			else 
			{
				if (ret == NULL)
				{
					//TODO
					//throw exception here
					//decrement refs
					//communicate with node?
				}
				else
				{
					lock_gil lock_me;
					Py_DECREF(ret);
				}
			}
		}
	}
}

//Does nothing for now.
static void python_to_node_message_handler(uv_async_t* _handle) {}

static void python_loop_thread(pyjs_async::python_loop* _ploop)
{
	
	PY_DEBUG("('python_loop' thread started)");
	UV_CHECK_START();
	UV_CHECK_VOID(uv_run, &ploop.loop, UV_RUN_DEFAULT);
}

void pyjs_async::PythonLoopMessageNotify(pyjs_async::PythonNodeAsyncMessage&& msg)
{
	{
		std::lock_guard<std::mutex> lock(_python_message_queue_mutex);
		python_message_queue.push_back(std::move(msg));
	}

	uv_async_send(&ploop.ntp_async_handler);
}

void pyjs_async::StartMainPythonLoop(const Napi::CallbackInfo &info)
{
	PY_DEBUG("(starting python event loop)");

	Napi::Env env = info.Env();
	NAPI_DIRECT_START(env);
	NAPI_DIRECT_FUNC(napi_get_uv_event_loop, &_node_event_loop);

	UV_CHECK_START();

	//Grab node's loop handler info.
	UV_CHECK_VOID(uv_async_init, _node_event_loop, &ploop.ptn_async_handler, python_to_node_message_handler);

	//The main loop handler for Python async events and tasks. We do our messaging here.
	UV_CHECK_VOID(uv_loop_init, &ploop.loop);
	UV_CHECK_VOID(uv_async_init, &ploop.loop, &ploop.ntp_async_handler, node_to_python_message_handler);

	//A timer that runs on node's main loop and forces node to give up its GIL so other Python tasks can run (e.g. async).
	UV_CHECK_VOID(uv_timer_init, _node_event_loop, &ploop.switching_loop_timer);
	UV_CHECK_VOID(uv_timer_start, &ploop.switching_loop_timer, python_switching_handler, 500, 10);

	//Start new loops using C++11 threading implementation (cross-platform)
	std::thread py_thread([] {
		python_loop_thread(&ploop);
	});
	py_thread.detach();
}

////////////////////////////////////////////
// Main Node/Python Async Controller
////////////////////////////////////////////

void pyjs_async::DestroyAsyncHandlers()
{
	if (!exiting)
	{
		exiting = true;

		//Stop handlers.
		uv_unref((uv_handle_t*)&ploop.ntp_async_handler);
		uv_unref((uv_handle_t*)&ploop.ptn_async_handler);
		uv_timer_stop(&ploop.switching_loop_timer);
	}
}

Napi::Value GetCurrentThreadID(const Napi::CallbackInfo &info)
{
	Napi::Env env = info.Env();

	std::thread::id this_id = std::this_thread::get_id();
	std::stringstream ss;
	ss << this_id;

	return Napi::String::New(env, ss.str());
}

Napi::Object pyjs_async::InitThreading(Napi::Env env, Napi::Object exports)
{
	exports.Set("$GetCurrentThreadID", Napi::Function::New(env, GetCurrentThreadID));	
	return exports;
}

////////////////////////////////////////////
// Async Setup
////////////////////////////////////////////

void pyjs_async::InitAll(Napi::Env env, Napi::Object exports)
{
	pyjs_async::InitThreading(env, exports);
}

