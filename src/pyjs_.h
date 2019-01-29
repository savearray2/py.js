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

#ifndef HEADER_FILES_WERE_A_TERRIBLE_IDEA
#define HEADER_FILES_WERE_A_TERRIBLE_IDEA

#include <Python.h>
#include <uv.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <iomanip>
#include <ctime>
#include <napi.h>
#include <functional>
#include <future>
#include <string>
#include <chrono>

//////////////////////////////////////////
// Hardcoded Options
//////////////////////////////////////////

//#define __PY__DEBUG

//////////////////////////////////////////
// Forward Declarations
//////////////////////////////////////////

class NapiPyObject;
namespace napi_ext { class ThreadSafeCallback; }

//////////////////////////////////////////
// Namespaces & Enums
//////////////////////////////////////////

enum PyObjectType
{
	Object = 0,
	None,
	Integer,
	Bool,
	Float,
	Complex,
	Bytes,
	ByteArray,
	Unicode,
	Tuple,
	List,
	Dictionary,
	Set,
	Function,
	Method,
	Type,
	_JS_Function,
	_JS_Wrap,
	Unsupported
};

enum PyObjectTarget
{
	Python = 0,
	Javascript
};

namespace pyjs
{
	struct MarshallingOptions
	{
		bool rawReference = false;
	};

	std::pair<PyObject*, PyObjectType> Js_ConvertToPython(const Napi::Env env,
		const Napi::Value val, const std::unique_ptr<const std::vector<Napi::Function>> &filters);
	Napi::Value Py_ConvertToJavascript(const Napi::Env env, PyObject* obj,
		const std::unique_ptr<const std::vector<Napi::Function>> &filters,
		std::unique_ptr<std::unordered_map<PyObject*,napi_value>>& python_to_javascript_map,
		const MarshallingOptions& marshalling_options);
	Napi::Value Import(const Napi::CallbackInfo &info);

	PyObject* FunctionBridgeRegisterCallback(const Napi::Env& env, Napi::Function f);

	class PyjsConfigurationOptions
	{
		private:
			static Napi::FunctionReference serialization_filter_callback_;
			static Napi::FunctionReference unmarshalling_filter_callback_;
			static std::unique_ptr<napi_ext::ThreadSafeCallback> debug_messaging_callback_;

		public:
			static Napi::Object Init(const Napi::Env env, const Napi::Object exports);
			static Napi::Value SetSerializationFiltersConstructor(const Napi::CallbackInfo &info);
			static const std::unique_ptr<const std::vector<Napi::Function>> GetSerializationFilters();
			static Napi::Value SetUnmarshallingFilter(const Napi::CallbackInfo &info);
			static NapiPyObject* AttemptNapiObjectUnmarshalling(Napi::Env env, Napi::Value obj);
			static Napi::Value SetDebugMessagingCallback(const Napi::CallbackInfo &info);
			static void SendDebugMessage(const std::vector<std::string> msg);
			static bool IsDebugEnabled();
	};
}

namespace pyjs_async
{
	struct python_loop {
		uv_loop_t loop{};
		uv_loop_t switching_loop{};
		uv_async_t ptn_async_handler{};
		uv_async_t ntp_async_handler{};
		uv_timer_t switching_loop_timer{};
		std::vector<std::string> v{};
	};

	void StartMainPythonLoop(const Napi::CallbackInfo &info);

	void InitAll(Napi::Env env, Napi::Object exports);
	Napi::Object InitThreading(Napi::Env env, Napi::Object exports);

	static std::mutex _async_cb_mutex;

	void InitializeAsyncMessageSystem(const Napi::CallbackInfo &info);
	void DestroyAsyncHandlers();
}

//////////////////////////////////////////
// Helper Classes/Structs
//////////////////////////////////////////

class NapiPyObjectContainer
{
	public:
		NapiPyObjectContainer();
		~NapiPyObjectContainer();
		void set_pyObject(PyObject* pyObject);
		PyObject* get_pyObject();
	private:
		PyObject* pyObject_;
};

class NapiPyObject : public Napi::ObjectWrap<NapiPyObject>
{
	private:
		NapiPyObjectContainer* container_;
		PyObjectType type_;
		static Napi::FunctionReference constructor_;
	public:
		static Napi::FunctionReference serialization_callback_;
		NapiPyObject(const Napi::CallbackInfo &info);
		~NapiPyObject();
		static void SetSerializationCallBackConstructor(const Napi::CallbackInfo &info);
		static Napi::Object Init(const Napi::Env env, const Napi::Object exports);
		static Napi::Object NewInstance(const Napi::Env env, const std::vector<napi_value> &args);
		static Napi::Value GetMarshaledObject(const Napi::CallbackInfo &info);
		static Napi::Value IsInstanceOf(const Napi::CallbackInfo &info);
		static bool IsInstanceOfNative(Napi::Env env, Napi::Value val);
		Napi::Value GetObjectType(const Napi::CallbackInfo &info);
		Napi::Value GetPythonTypeObject(const Napi::CallbackInfo &info);
		Napi::Value GetAttributeList(const Napi::CallbackInfo &info);
		Napi::Value GetAttribute(const Napi::CallbackInfo &info);
		Napi::Value SetAttribute(const Napi::CallbackInfo &info);
		Napi::Value IsCallable(const Napi::CallbackInfo &info);
		static std::pair<PyObject*,PyObject*> ProcessFunctionCallArguments(const Napi::CallbackInfo &info);
		static pyjs::MarshallingOptions ProcessMarshallingOptions(const Napi::Value val);
		Napi::Value FunctionCallAsync(const Napi::CallbackInfo &info);
		Napi::Value FunctionCall(const Napi::CallbackInfo &info);
		Napi::Value CloneReference(const Napi::CallbackInfo &info);
		void SetPyObject(const Napi::Env env, PyObject* pyObject);
		PyObject* GetPyObject(const Napi::Env env);
		void SetObjectType(const PyObjectType type);
		PyObjectType GetObjectTypeUnwrapped();
};

namespace pyjs_async
{
	enum PythonNodeAsyncMessageType
	{
		FunctionCall = 0
	};

	struct PythonNodeAsyncMessage
	{
		PythonNodeAsyncMessageType msg_type;
		std::vector<PyObject*> arguments;
		std::unique_ptr<napi_ext::ThreadSafeCallback> callback;

		//Can't be copied, add default move constructor
		PythonNodeAsyncMessage(const PythonNodeAsyncMessage&) = delete;
		PythonNodeAsyncMessage(PythonNodeAsyncMessage&&) = default;
	};

	void PythonLoopMessageNotify(PythonNodeAsyncMessage&& msg);
}

class lock_gil
{
	public:
		lock_gil() { _state = PyGILState_Ensure(); }
		~lock_gil() { PyGILState_Release(_state); }
	private:
		 PyGILState_STATE _state;
};

//////////////////////////////////////////
// Utils
//////////////////////////////////////////

namespace pyjs_utils
{
	void ThrowPythonException(const Napi::Env env);
	std::string GetPythonException();

	unsigned long GetCurrentTimeTicks();
	std::string GetCurrentThreadID();

	Napi::Object InitAll(Napi::Env env, Napi::Object exports);
}

//////////////////////////////////////////
// Custom Defs
//////////////////////////////////////////

#define NAPI_DIRECT_START(env)								\
	napi_status _napi_status;								\
	napi_env _napi_env = Napi::Env(env);
#if _WIN32
#define NAPI_DIRECT_FUNC(fn, ...)							\
	_napi_status = fn(_napi_env, __VA_ARGS__);				\
	if (_napi_status != napi_ok) {							\
		Napi::Error::New(Napi::Env(_napi_env),				\
		"Error calling NAPI function. This should never happen. You may be out of memory.").ThrowAsJavaScriptException();	}
#else
#define NAPI_DIRECT_FUNC(fn, args...)						\
	_napi_status = fn(_napi_env, args);						\
	if (_napi_status != napi_ok) {							\
		Napi::Error::New(Napi::Env(_napi_env),				\
		"Error calling NAPI function. This should never happen. You may be out of memory.").ThrowAsJavaScriptException();	}
#endif

#define NAPI_VALUE_WRAP(env, wrapper, val)					\
	wrapper = Napi::Value(env, val);

#define NAPI_ERROR(env, msg)								\
	Napi::Error::New(env, msg).ThrowAsJavaScriptException();

#define NAPI_FATAL(loc, msg)								\
	Napi::Error::Fatal("", msg);

#define PY_CHECK_START()									\
	std::vector<PyObject*> _pyobj_vector;

#define PY_CHECK_INCLUDE(obj)								\
	_pyobj_vector.push_back(obj);

#define PY_CHECK(env, a, b, c)								\
	if (a == b || PyErr_Occurred())							\
	{														\
		pyjs_utils::ThrowPythonException(env);				\
		for (PyObject* _pyobj : _pyobj_vector)				\
			Py_XDECREF(_pyobj);								\
		return c;											\
	}

#define PY_CHECK_NAPI_ERROR(env)							\
	if (env.IsExceptionPending())							\
		for (PyObject* _pyobj : _pyobj_vector)				\
			Py_XDECREF(_pyobj);								\

#define UV_CHECK_START()									\
	int _uv_err;
#if _WIN32
#define UV_CHECK_VOID(fn, ...)								\
	_uv_err = fn(__VA_ARGS__);								\
	if (_uv_err < 0) {										\
		NAPI_FATAL("",										\
		("Error in libuv initialization. (" +				\
		std::string(__func__) + ")").c_str()); return; }
#else
#define UV_CHECK_VOID(fn, args...)							\
	_uv_err = fn(args);										\
	if (_uv_err < 0) {										\
		NAPI_FATAL("",										\
		("Error in libuv initialization. (" +				\
		std::string(__func__) + ")").c_str()); return; }
#endif		

#define SLEEP(s)											\
	std::this_thread::sleep_for(							\
		std::chrono::milliseconds(s));

//////////////////////////////////////////
// Debug
//////////////////////////////////////////

#ifdef __PY__DEBUG
#define PY_DEBUG(msg)										\
pyjs::PyjsConfigurationOptions::SendDebugMessage({			\
	msg, 													\
	pyjs_utils::GetCurrentThreadID(),						\
	std::to_string(pyjs_utils::GetCurrentTimeTicks())});				
#else
#define PY_DEBUG(msg) (void)0;
#endif

//////////////////////////////////////////
/*
template <typename T>
class NapiPyObjectHelperContainer
{
	private:
		static Napi::FunctionReference constructor_;
		const PyObject* pyObject_;
	public:
		Napi::Object Init(Napi::Env env, Napi::Object exports)
		{
			Napi::Function func = NapiPyObject::DefineClass(env, "PyContainer", {});
			NapiPyObjectHelperContainer::constructor_ = Napi::Persistent(func);
			NapiPyObjectHelperContainer::constructor_.SuppressDestruct();
			return exports;
		}
		NapiPyObjectHelperContainer(const PyObject* pyObject)
		{
			pyObject_ = pyObject;
		}

		const PyObject* GetPyObject()
		{
			return pyObject_;
		}

		void Finalize(Napi::Env env, T* data)
		{
			Py_DECREF(pyObject_);
			PY_DEBUG("NapiPyObjectHelperContainer::~NapiPyObjectHelperContainer");
			delete this;
		}

		static Napi::Value InstantiateContainerAndWrap(const Napi::Env env, const PyObject* pyObject)
		{
			Napi::Object obj = NapiPyObjectHelperContainer<T>::constructor_.New({});
			return obj;
		}
};*/

//////////////////////////////////////////

#ifdef __PY__DEBUG
	template <class T>
	constexpr
	std::string_view
	type_name()
	{
		using namespace std;
	#ifdef __clang__
		string_view p = __PRETTY_FUNCTION__;
		return string_view(p.data() + 34, p.size() - 34 - 1);
	#elif defined(__GNUC__)
		string_view p = __PRETTY_FUNCTION__;
	#  if __cplusplus < 201402
		return string_view(p.data() + 36, p.size() - 36 - 1);
	#  else
		return string_view(p.data() + 49, p.find(';', 49) - 49);
	#  endif
	#elif defined(_MSC_VER)
		string_view p = __FUNCSIG__;
		return string_view(p.data() + 84, p.size() - 84 - 7);
	#endif
	}
#endif

//////////////////////////////////////////

#endif