//////////////////////////////////////////////////////////////////////
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
//////////////////////////////////////////////////////////////////////

#include "pyjs_.h"
#include "napi_callback.hpp"

//////////////////////////////////////////////////////////////////////

NapiPyObjectContainer::NapiPyObjectContainer() { }

void NapiPyObjectContainer::set_pyObject(PyObject* pyObject)
{
	this->pyObject_ = pyObject;
}

PyObject* NapiPyObjectContainer::get_pyObject()
{
	return this->pyObject_;
}

NapiPyObjectContainer::~NapiPyObjectContainer()
{
	Py_DECREF(this->pyObject_);
}

//////////////////////////////////////////////////////////////////////

Napi::FunctionReference NapiPyObject::constructor_;
Napi::FunctionReference NapiPyObject::serialization_callback_;

void NapiPyObject::SetSerializationCallBackConstructor(const Napi::CallbackInfo &info)
{
	serialization_callback_.Reset();
	serialization_callback_ = Napi::Persistent(info[0].As<Napi::Function>());
	serialization_callback_.SuppressDestruct();
}

Napi::Object NapiPyObject::Init(Napi::Env env, Napi::Object exports)
{
	Napi::HandleScope scope(env);

	Napi::Function func = NapiPyObject::DefineClass(env, "PyObject", {
		NapiPyObject::InstanceMethod("GetObjectType", &NapiPyObject::GetObjectType),
		NapiPyObject::InstanceMethod("GetPythonTypeObject", &NapiPyObject::GetPythonTypeObject),
		NapiPyObject::InstanceMethod("GetAttributeList", &NapiPyObject::GetAttributeList),
		NapiPyObject::InstanceMethod("GetAttribute", &NapiPyObject::GetAttribute),
		NapiPyObject::InstanceMethod("SetAttribute", &NapiPyObject::SetAttribute),
		NapiPyObject::InstanceMethod("IsCallable", &NapiPyObject::IsCallable),
		NapiPyObject::InstanceMethod("FunctionCallAsync", &NapiPyObject::FunctionCallAsync),
		NapiPyObject::InstanceMethod("FunctionCall", &NapiPyObject::FunctionCall),
		NapiPyObject::InstanceMethod("CloneReference", &NapiPyObject::CloneReference)
	});

	NapiPyObject::constructor_ = Napi::Persistent(func);
	NapiPyObject::constructor_.SuppressDestruct();

	//exports.Set("NapiPyObject", func);
	return exports;
}

Napi::Value NapiPyObject::GetObjectType(const Napi::CallbackInfo &info)
{
	Napi::Env env = info.Env();
	Napi::Number num = Napi::Number::New(env, (double)this->type_);

	return num;
}

Napi::Value NapiPyObject::GetPythonTypeObject(const Napi::CallbackInfo &info)
{
	Napi::Env env = info.Env();

	PyObject* itm = this->container_->get_pyObject();
	PyObject* type = PyObject_Type(itm);
	
	auto map = std::unique_ptr<std::unordered_map<PyObject*,napi_value>>
		(new std::unordered_map<PyObject*,napi_value>());
	auto res = pyjs::Py_ConvertToJavascript(env, type,
		pyjs::PyjsConfigurationOptions::GetSerializationFilters(),
		map, pyjs::MarshallingOptions());

	Py_XDECREF(type);

	return res;
}

Napi::Value NapiPyObject::GetAttributeList(const Napi::CallbackInfo &info)
{
	Napi::Env env = info.Env();
	PY_CHECK_START();

	PyObject* pyObject = this->container_->get_pyObject();
	PyObject* dir = PyObject_Dir(pyObject); //PyObject_Dir (New)
	if (dir == NULL)
	{
		//Not all types have a full object definition, and as such, PyObject_Dir will fail.
		//In specific, PyMethodDef *tp_methods is init'd to NULL when defining dynamically
		//generated custom types, and the current API isn't set up to handle that, it seems.

		PyErr_Clear();

		PyObject* dir_name = PyUnicode_FromString("__dir__"); //PyUnicode_FromString (New)
		PyObject* dir_func = PyObject_GetAttr(pyObject, dir_name); //PyObject_GetItem (New)
		Py_DECREF(dir_name);
		PY_CHECK(env, dir_func, NULL, env.Undefined());
		PY_CHECK_INCLUDE(dir_func);
		PyObject* dir_args = PyTuple_New(0); //PyTuple_New (New)
		//Py_INCREF(pyObject);
		//PyTuple_SET_ITEM(dir_args, 0, pyObject); //PyTuple_SET_ITEM (Steals)
		dir = PyObject_Call(dir_func, dir_args, NULL); //PyObject_Call (New)
		Py_DECREF(dir_args);
	}

	PY_CHECK(env, dir, NULL, env.Undefined());
	PY_CHECK_INCLUDE(dir);

	Py_ssize_t size = PyList_Size(dir);

	NAPI_DIRECT_START(env);
	napi_value napi_array;
	NAPI_DIRECT_FUNC(napi_create_array_with_length, size, &napi_array);

	for (Py_ssize_t i = 0; i < size; i++)
	{
		PyObject* name = PyList_GetItem(dir, i); //PyList_GetItem (Borrowed)
		PY_CHECK(env, name, NULL, env.Undefined());
		napi_value napi_name;
		NAPI_DIRECT_FUNC(napi_create_string_utf8, PyUnicode_AsUTF8(name), NAPI_AUTO_LENGTH, &napi_name);
		NAPI_DIRECT_FUNC(napi_set_element, napi_array, i, napi_name);
	}

	Napi::Value nArray;
	NAPI_VALUE_WRAP(env, nArray, napi_array);

	Py_DECREF(dir);

	return nArray;
}

Napi::Value NapiPyObject::GetAttribute(const Napi::CallbackInfo &info)
{
	Napi::Env env = info.Env();

	PyObject* pyObject = this->container_->get_pyObject();
	PyObject* attr_name = PyUnicode_FromString(info[0].ToString().Utf8Value().c_str()); //PyUnicode_FromString (New)
	PyObject* attr = PyObject_GetAttr(pyObject, attr_name); //PyObject_GetItem (New)
	Py_XDECREF(attr_name);

	auto map = std::unique_ptr<std::unordered_map<PyObject*,napi_value>>
		(new std::unordered_map<PyObject*,napi_value>());
	
	//Use info[1] for marshalling options
	auto res = pyjs::Py_ConvertToJavascript(env, attr,
		pyjs::PyjsConfigurationOptions::GetSerializationFilters(),
		map, NapiPyObject::ProcessMarshallingOptions(info[1]));
	
	Py_XDECREF(attr);

	return res;
}

Napi::Value NapiPyObject::SetAttribute(const Napi::CallbackInfo &info)
{
	Napi::Env env = info.Env();
	PyObject* pyObject = this->container_->get_pyObject();
	
	Napi::Value napiVal = info[1];
	NapiPyObject* npo = Napi::ObjectWrap<NapiPyObject>::Unwrap(napiVal.As<Napi::Object>());
	PyObject* itm = npo->GetPyObject(env);
	Py_INCREF(itm); // NapiPyObject(Future Delete)

	if (PyObject_SetAttrString(pyObject, info[0].ToString().Utf8Value().c_str(), itm) < 0)
		NAPI_ERROR(env, "Error setting attribute '" + info[0].ToString().Utf8Value() + "' on object.");

	return env.Undefined();
}

Napi::Value NapiPyObject::IsCallable(const Napi::CallbackInfo &info)
{
	Napi::Env env = info.Env();
	PyObject* pyObject = this->container_->get_pyObject();

	if (PyCallable_Check(pyObject) && !PyType_Check(pyObject)) //Always succeeds
	{
		return Napi::Boolean::New(env, true);
	}
	else
	{
		return Napi::Boolean::New(env, false);
	}
}

std::pair<PyObject*,PyObject*> NapiPyObject::ProcessFunctionCallArguments(const Napi::CallbackInfo &info)
{
	Napi::Env env = info.Env();

	napi_value napi_arr = Napi::Array(info[0].As<Napi::Array>());
	napi_value napi_dict = NULL;
	if (!info[1].IsUndefined())
		napi_dict = info[1].ToObject();

	uint32_t size;
	PY_CHECK_START();
	NAPI_DIRECT_START(env);
	NAPI_DIRECT_FUNC(napi_get_array_length, napi_arr, &size);

	PyObject* args = PyTuple_New(size); //PyTuple_New (New)
	PY_CHECK(env, args, NULL, std::make_pair(nullptr, nullptr));
	PY_CHECK_INCLUDE(args);

	for (uint32_t i = 0; i < size; i++)
	{
		napi_value napi_ele;
		NAPI_DIRECT_FUNC(napi_get_element, napi_arr, i, &napi_ele);
		Napi::Value napiVal = Napi::Value(env, napi_ele);
		NapiPyObject* npo = Napi::ObjectWrap<NapiPyObject>::Unwrap(napiVal.As<Napi::Object>());
		PyObject* itm = npo->GetPyObject(env);
		Py_INCREF(itm); // NapiPyObject(Future Delete)
		PyTuple_SET_ITEM(args, i, itm); //PyTuple_SET_ITEM (Steals)
	}

	PyObject* dict = PyDict_New(); //PyDict_New (New)
	PY_CHECK(env, dict, NULL, std::make_pair(nullptr, nullptr));
	PY_CHECK_INCLUDE(dict);

	if (napi_dict != NULL)
	{
		napi_value napi_arr_2;
		NAPI_DIRECT_FUNC(napi_get_property_names, napi_dict, &napi_arr_2);
		NAPI_DIRECT_FUNC(napi_get_array_length, napi_arr_2, &size);
		for (uint32_t i = 0; i < size; i++)
		{
			napi_value napi_ele_key;
			napi_value napi_ele_val;

			NAPI_DIRECT_FUNC(napi_get_element, napi_arr_2, i, &napi_ele_key);
			NAPI_DIRECT_FUNC(napi_get_property, napi_dict, napi_ele_key, &napi_ele_val);

			Napi::Value ele_key = Napi::Value(env, napi_ele_key);
			Napi::Value ele_val = Napi::Value(env, napi_ele_val);

			NapiPyObject* npo = Napi::ObjectWrap<NapiPyObject>::Unwrap(ele_val.As<Napi::Object>());

			PyObject* key_1 = PyUnicode_FromString(ele_key.As<Napi::String>().Utf8Value().c_str()); //PyUnicode_FromString (New)
			PyObject* val_2 = npo->GetPyObject(env);

			PyDict_SetItem(dict, key_1, val_2); //PyDict_SetItem (Neutral)
			Py_DECREF(key_1); //val_2 is being handled by NapiPyObject
		}
	}
	
	return std::make_pair(args, dict);
}

pyjs::MarshallingOptions NapiPyObject::ProcessMarshallingOptions(const Napi::Value val)
{
	if (val.IsNull())
	{
		//Default options
		return pyjs::MarshallingOptions();
	}

	Napi::Object obj = val.ToObject();
	pyjs::MarshallingOptions mo = pyjs::MarshallingOptions(
		obj.Get("getReference").ToBoolean().Value());

	return mo;
}

Napi::Value NapiPyObject::FunctionCallAsync(const Napi::CallbackInfo &info)
{
	Napi::Env env = info.Env();
	Napi::EscapableHandleScope scope(env);

	//info[0] & info[1] are used for (args, kwargs)
	auto pair = NapiPyObject::ProcessFunctionCallArguments(info);
	if (!pair.first)
		return env.Undefined();

	PyObject* pyObject = this->container_->get_pyObject();
	Py_INCREF(pyObject); //Keep during async

	if (info.Length() > 2)
	{
		std::unique_ptr<napi_ext::ThreadSafeCallback> ptr 
			= std::make_unique<napi_ext::ThreadSafeCallback>(
				info[2].As<Napi::Function>());

		pyjs_async::PythonLoopMessageNotify({
			pyjs_async::PythonNodeAsyncMessageType::FunctionCall,
			{ pyObject, pair.first, pair.second },
			std::move(ptr)
		});
	}
	else
	{
		pyjs_async::PythonLoopMessageNotify({
			pyjs_async::PythonNodeAsyncMessageType::FunctionCall,
			{ pyObject, pair.first, pair.second },
			nullptr
		});
	}

	return scope.Escape(napi_value(env.Undefined()));
}

Napi::Value NapiPyObject::FunctionCall(const Napi::CallbackInfo &info)
{
	Napi::Env env = info.Env();
	Napi::EscapableHandleScope scope(env);

	//info[0] & info[1] are used for (args, kwargs)
	auto pair = NapiPyObject::ProcessFunctionCallArguments(info);
	if (!pair.first)
		return env.Undefined();

	PyObject* args = pair.first;
	PyObject* dict = pair.second;
	PyObject* pyObject = this->container_->get_pyObject();

	PY_CHECK_START();
	PyObject* retValue = PyObject_Call(pyObject, args, dict); //PyObject_Call (New)
	PY_CHECK(env, retValue, NULL, env.Undefined());
	Py_DECREF(args);
	Py_DECREF(dict);

	pyjs::MarshallingOptions mo = 
		NapiPyObject::ProcessMarshallingOptions(info[2]);

	if (mo.rawReference)
	{
		//Should we send this as an object type?
		Napi::Value napiValue = NapiPyObject::NewInstance(env, {});
		NapiPyObject* npo = Napi::ObjectWrap<NapiPyObject>::Unwrap(napiValue.As<Napi::Object>());
		npo->SetPyObject(env, retValue); //NapiPyObject now managing memory for retValue
		return scope.Escape(napi_value(napiValue));
	}

	auto map = std::unique_ptr<std::unordered_map<PyObject*,napi_value>>
		(new std::unordered_map<PyObject*,napi_value>());

	//Use info[2] for marshalling options.
	auto napiValue = pyjs::Py_ConvertToJavascript(env, retValue,
		pyjs::PyjsConfigurationOptions::GetSerializationFilters(),
		map, mo);

	Py_DECREF(retValue);

	return scope.Escape(napi_value(napiValue));
}

Napi::Value NapiPyObject::CloneReference(const Napi::CallbackInfo &info)
{
	Napi::Env env = info.Env();
	PyObject* pyObject = this->container_->get_pyObject();
	Napi::Value napiValue = NapiPyObject::NewInstance(env, {});
	NapiPyObject* npo = Napi::ObjectWrap<NapiPyObject>::Unwrap(napiValue.As<Napi::Object>());
	npo->SetPyObject(env, pyObject);
	npo->SetObjectType(type_);
	Py_INCREF(pyObject);

	return napiValue;
}

Napi::Value NapiPyObject::GetMarshaledObject(const Napi::CallbackInfo &info)
{
	Napi::Env env = info.Env();
	Napi::EscapableHandleScope scope(env);

	const std::unique_ptr<const std::vector<Napi::Function>>&
		serialization_filters = pyjs::PyjsConfigurationOptions::GetSerializationFilters();
	std::pair<PyObject*,PyObjectType> conv = pyjs::Js_ConvertToPython(env, info[0],
		serialization_filters);

	if (env.IsExceptionPending())
		return env.Undefined();

	PyObject* obj = conv.first;
	Napi::Value napiValue = NapiPyObject::NewInstance(env, {});
	NapiPyObject* npo = Napi::ObjectWrap<NapiPyObject>::Unwrap(napiValue.As<Napi::Object>());
	npo->SetPyObject(env, obj);
	npo->SetObjectType(conv.second);

	//Finalize
	if (serialization_filters->size() > 0)
		serialization_filters->operator[](2).Call({ });

	return scope.Escape(napi_value(napiValue));
}

NapiPyObject::NapiPyObject(const Napi::CallbackInfo &info) : Napi::ObjectWrap<NapiPyObject>(info)
{
	container_ = new NapiPyObjectContainer();
}

Napi::Object NapiPyObject::NewInstance(Napi::Env env, const std::vector<napi_value>& args)
{
	Napi::Object obj = NapiPyObject::constructor_.New(args);
	return obj;
}

Napi::Value NapiPyObject::IsInstanceOf(const Napi::CallbackInfo &info)
{
	Napi::Env env = info.Env();

	if (info.Length() < 1)
	{
		NAPI_ERROR(env, "Invalid Parameters. Expecting more than one argument.");
	}

	napi_value napi_val = Napi::Value(info[0]);
	napi_value constructor_ref;
	NAPI_DIRECT_START(env);
	NAPI_DIRECT_FUNC(napi_get_reference_value, NapiPyObject::constructor_, &constructor_ref);
	bool instance_of = false;
	NAPI_DIRECT_FUNC(napi_instanceof, napi_val, constructor_ref, &instance_of);

	return Napi::Boolean::New(info.Env(), instance_of);
}

bool NapiPyObject::IsInstanceOfNative(Napi::Env env, Napi::Value val)
{
	napi_value napi_val = val;
	napi_value constructor_ref;
	NAPI_DIRECT_START(env);
	NAPI_DIRECT_FUNC(napi_get_reference_value, NapiPyObject::constructor_, &constructor_ref);
	bool instance_of = false;
	NAPI_DIRECT_FUNC(napi_instanceof, napi_val, constructor_ref, &instance_of);
	return instance_of;
}

void NapiPyObject::SetPyObject(Napi::Env env, PyObject* pyObject)
{
	this->container_->set_pyObject(pyObject);
}

PyObject* NapiPyObject::GetPyObject(Napi::Env env)
{
	return this->container_->get_pyObject();
}

void NapiPyObject::SetObjectType(PyObjectType type)
{
	this->type_ = type;
}

PyObjectType NapiPyObject::GetObjectTypeUnwrapped()
{
	return this->type_;
}

NapiPyObject::~NapiPyObject()
{
	delete this->container_;
}