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
//
//	Additional permission under the GNU Affero GPL version 3 section 7:
//
//	If you modify this Program, or any covered work, by linking or
//	combining it with other code, such other code is not for that reason
//	alone subject to any of the requirements of the GNU Affero GPL
//	version 3.
//////////////////////////////////////////////////////////////////////////

#include "pyjs_.h"
#include "napi_callback.hpp"


inline Napi::Value CoerceAsInteger(const Napi::CallbackInfo &info)
{
	Napi::Env env = info.Env();

	PyObject* obj;
	if (info[0].IsString())
		obj = PyLong_FromString(info[0].ToString().Utf8Value().c_str(),
			NULL, info[1].ToNumber().operator int());
	else if (info[0].IsNumber())
		obj = PyLong_FromLongLong(info[0].ToNumber().operator uint32_t());
	else
	{
		NAPI_ERROR(env, "Unknown type. Unable to coerce as integer.");
		return env.Undefined();
	}

	Napi::Value napiValue = NapiPyObject::NewInstance(env, {});
	NapiPyObject* npo = Napi::ObjectWrap<NapiPyObject>::Unwrap(napiValue.As<Napi::Object>());
	npo->SetPyObject(env, obj);
	npo->SetObjectType(PyObjectType::Integer);

	return napiValue;
};

inline Napi::Value CoerceAsTuple(const Napi::CallbackInfo &info)
{
	Napi::Env env = info.Env();
	Napi::EscapableHandleScope scope(env);

	napi_value napi_array = info[0];
	uint32_t size;

	NAPI_DIRECT_START(env);
	NAPI_DIRECT_FUNC(napi_get_array_length, napi_array, &size);

	PyObject* obj = PyTuple_New(size); //PyTuple_New (New)

	for (uint32_t i = 0; i < size; i++)
	{
		napi_value napi_ele;
		NAPI_DIRECT_FUNC(napi_get_element, napi_array, i, &napi_ele);
		NapiPyObject* _npo = Napi::ObjectWrap<NapiPyObject>::Unwrap(Napi::Object(env, napi_ele).As<Napi::Object>());
		PyObject* py_ele = _npo->GetPyObject(env);
		PyTuple_SET_ITEM(obj, i, py_ele);
	}

	Napi::Value napiValue = NapiPyObject::NewInstance(env, {});
	NapiPyObject* npo = Napi::ObjectWrap<NapiPyObject>::Unwrap(napiValue.As<Napi::Object>());
	npo->SetPyObject(env, obj);
	npo->SetObjectType(PyObjectType::Tuple);

	return scope.Escape(napi_value(napiValue));
}

inline Napi::Object coerceAs(Napi::Env env)
{
	Napi::Object obj = Napi::Object::New(env);
	obj.Set("Integer", Napi::Function::New(env, CoerceAsInteger));
	obj.Set("Tuple", Napi::Function::New(env, CoerceAsTuple));

	return obj;
};

Napi::Object pyjs_utils::InitAll(Napi::Env env, Napi::Object exports)
{
	exports.Set("$coerceAs", coerceAs(env));
	return exports;
}