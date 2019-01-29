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
#include "napi_callback.hpp"

////////////////////////////////////////////
// Exceptions
////////////////////////////////////////////

void pyjs_utils::ThrowPythonException(Napi::Env env)
{
	auto p_ex = pyjs_utils::GetPythonException();
	auto napiEx = NapiPyObject::NewInstance(env, {});
	NapiPyObject* npo = Napi::ObjectWrap<NapiPyObject>::Unwrap(napiEx.As<Napi::Object>());
	npo->SetPyObject(env, p_ex.second);
	npo->SetObjectType(PyObjectType::Python_Exception);
	Py_INCREF(p_ex.second); //Clone.

	Napi::Object exObj = Napi::Object::New(env);
	exObj.Set("message", p_ex.first);
	exObj.Set("exception", napiEx);

	Napi::Value ex = NapiPyObject::serialization_callback_.Call(
	{
		Napi::Number::New(env, PyObjectType::Python_Exception),
		exObj
	});


	NAPI_DIRECT_START(env);
	NAPI_DIRECT_FUNC(napi_throw, ex);
}

////////////////////////////////////////////
// Debug?
////////////////////////////////////////////

std::string pyjs_utils::GetCurrentThreadID()
{
	std::thread::id this_id = std::this_thread::get_id();
	std::stringstream ss;
	ss << this_id;
	return ss.str();
}

unsigned long pyjs_utils::GetCurrentTimeTicks()
{
	std::chrono::milliseconds ms =
		std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch());
	
	return ms.count();
}

/*void py_debug::print(const std::string &msg)
{
	std::time_t t = std::time(nullptr);
	std::cout << std::put_time(std::localtime(&t), "[%H:%M:%S] ") << msg << std::endl;
}*/
