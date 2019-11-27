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
// pyjs static references
////////////////////////////////////////////

static PyObject* __pyjs_module_;
static PyObject* __py__main__module_;

////////////////////////////////////////////
// Javascript -> Python Marshalling
////////////////////////////////////////////

std::pair<PyObject*,PyObjectType> pyjs::Js_ConvertToPython(const Napi::Env env,
	const Napi::Value val, const std::unique_ptr<const std::vector<Napi::Function>>& filters)
{
	PyObject* obj = nullptr;
	PyObjectType pot = PyObjectType::Unsupported;

	//If marshalling an undefined or null type, return immediately.
	if (val.IsUndefined() || val.IsNull())
	{
		obj = Py_None; //Py_None (Static)
		pot = PyObjectType::None;
		Py_INCREF(obj);
		return std::make_pair(obj, pot);
	}

	//Check to see if we have a JS proxy obj or Napi wrapper, if so return immediately.
	NapiPyObject* _napi_obj_tmp;
	if ((_napi_obj_tmp = pyjs::PyjsConfigurationOptions::AttemptNapiObjectUnmarshalling(env, val)))
	{
		PyObject* obj = _napi_obj_tmp->GetPyObject(env);
		Py_INCREF(obj); //Clone
		return std::make_pair(obj,
			_napi_obj_tmp->GetObjectTypeUnwrapped());
	}
	else if (NapiPyObject::IsInstanceOfNative(env, val))
	{
		_napi_obj_tmp = Napi::ObjectWrap<NapiPyObject>::Unwrap(val.As<Napi::Object>());
		PyObject* obj = _napi_obj_tmp->GetPyObject(env);
		Py_INCREF(obj); //Clone
		return std::make_pair(obj,
			_napi_obj_tmp->GetObjectTypeUnwrapped());
	}

	//Further JS -> Python marshalling performed after this point.
	obj = nullptr;

	if (val.IsBoolean())
	{
		if (val.ToBoolean())
		{
			obj = Py_True;
			Py_INCREF(obj);
			pot = PyObjectType::Bool;
		}
		else
		{
			obj = Py_False;
			Py_INCREF(obj);
			pot = PyObjectType::Bool;		
		}
	}
	else if (val.Type() == napi_valuetype::napi_bigint)
	{
		//TODO: write a better conversion at some point here
		obj = PyLong_FromString(val.ToString().Utf8Value().c_str(), NULL, 10);
		pot = PyObjectType::Integer;
	}
	else if (val.IsNumber())
	{
		//This should work, but Python seems to be platform specific.
		obj = PyFloat_FromDouble(val.As<Napi::Number>().DoubleValue()); //PyFloat_FromDouble (New)
		pot = PyObjectType::Float;
	}
	else if (val.IsString())
	{
		obj = PyUnicode_FromString(val.As<Napi::String>().Utf8Value().c_str()); //PyUnicode_FromString (New)
		pot = PyObjectType::Unicode;
	}
	else if (val.IsArray())
	{
		napi_value napi_array = Napi::Array(val.As<Napi::Array>());
		NAPI_DIRECT_START(env);
		//NAPI_SERIALIZATION_FILTER(filter, napi_array, obj);
		uint32_t size;
		pot = PyObjectType::List;

		if (!filters->empty())
		{
			//Sequence Test
			Napi::Value _filter_result = filters->operator[](0).Call({ napi_array });
			if (!_filter_result.IsUndefined())
			{
				Napi::Object _napi_pyObject = _filter_result.As<Napi::Object>();
				NapiPyObject* _npo = Napi::ObjectWrap<NapiPyObject>::Unwrap(_napi_pyObject);
				obj = _npo->GetPyObject(env);
				pot = _npo->GetObjectTypeUnwrapped();
				Py_INCREF(obj); //Clone.

				return std::make_pair(obj, pot);
			}
			//Sequence Register
			else
			{
				NAPI_DIRECT_FUNC(napi_get_array_length, napi_array, &size);
				obj = PyList_New(size); //PyList_New (New)
				auto temp = NapiPyObject::NewInstance(env, {});
				NapiPyObject* _npo = Napi::ObjectWrap<NapiPyObject>::Unwrap(temp);
				_npo->SetPyObject(env, obj);
				_npo->SetObjectType(pot);
				Py_INCREF(obj); //NapiPyObject (Future Delete)
				filters->operator[](1).Call({ napi_array, temp });
			}
		}
		else
		{
			NAPI_DIRECT_FUNC(napi_get_array_length, napi_array, &size);
			obj = PyList_New(size); //PyList_New (New)
		}

		for (uint32_t i = 0; i < size; i++)
		{
			napi_value napi_ele;
			NAPI_DIRECT_FUNC(napi_get_element, napi_array, i, &napi_ele);
			PyObject* ca = pyjs::Js_ConvertToPython(env, Napi::Value(env, napi_ele), filters).first;
			PyList_SET_ITEM(obj, i, ca); //PyList_SET_ITEM (Steals)
		}
	}
	else if (val.IsFunction())
	{
		//NAPI_ERROR(env, "Unable to marshal type: Function is currently unsupported.");
		//return env.Undefined();
		pot = PyObjectType::Function;

		PyObject* ptr = pyjs::FunctionBridgeRegisterCallback(env, val.As<Napi::Function>());
		Napi::Value tmp_val = NapiPyObject::NewInstance(env, {});
		NapiPyObject* tmp_obj = Napi::ObjectWrap<NapiPyObject>::Unwrap(tmp_val.As<Napi::Object>());
		tmp_obj->SetPyObject(env, ptr);
		tmp_obj->SetObjectType(PyObjectType::Integer);

		auto callback = NapiPyObject::serialization_callback_.Call(
			{
				Napi::Number::New(env, PyObjectType::_JS_Function),
				tmp_val
			}
		);

		Napi::Object _napi_pyObject = callback.As<Napi::Object>();
		NapiPyObject* _npo = Napi::ObjectWrap<NapiPyObject>::Unwrap(_napi_pyObject);
		PyObject* obj = _npo->GetPyObject(env);
		Py_INCREF(obj); //Clone.

		return std::make_pair(obj, pot);
	}
	else if (val.IsBuffer() || val.IsTypedArray())
	{
		auto data = Napi::Buffer<const char>(env, val);
		const char* bytes = data.Data();
		//Return a new bytes object with a copy of the string v
		obj = PyBytes_FromStringAndSize(bytes, data.Length()); //PyBytes_FromStringAndSize (New)
		pot = PyObjectType::Bytes;
	}
	else if (val.IsObject())
	{
		napi_value napi_obj = Napi::Object(val.As<Napi::Object>());
		NAPI_DIRECT_START(env);
		pot = PyObjectType::Object;

		if (!filters->empty())
		{
			//Sequence Test
			Napi::Value _filter_result = filters->operator[](0).Call({ napi_obj });
			if (!_filter_result.IsUndefined())
			{
				Napi::Object _napi_pyObject = _filter_result.As<Napi::Object>();
				NapiPyObject* _npo = Napi::ObjectWrap<NapiPyObject>::Unwrap(_napi_pyObject);
				obj = _npo->GetPyObject(env);
				pot = _npo->GetObjectTypeUnwrapped();
				Py_INCREF(obj); //Clone.

				return std::make_pair(obj, pot);
			}
			//Sequence Register
			else
			{
				obj = PyDict_New(); //PyDict_New (New)
				auto temp = NapiPyObject::NewInstance(env, {});
				NapiPyObject* _npo = Napi::ObjectWrap<NapiPyObject>::Unwrap(temp);
				_npo->SetPyObject(env, obj);
				_npo->SetObjectType(pot);
				Py_INCREF(obj); //Register new, but allow NapiPiObject to handle memory.
				filters->operator[](1).Call({ napi_obj, temp });
			}
		}
		else
		{
			obj = PyDict_New(); //PyDict_New (New)
		}

		napi_value napi_arr;
		NAPI_DIRECT_FUNC(napi_get_property_names, napi_obj, &napi_arr);
		uint32_t size;
		NAPI_DIRECT_FUNC(napi_get_array_length, napi_arr, &size);
		for (uint32_t i = 0; i < size; i++)
		{
			napi_value napi_ele;
			NAPI_DIRECT_FUNC(napi_get_element, napi_arr, i, &napi_ele);
			Napi::Value napiVal = Napi::Value(env, napi_ele);

			PyObject* key;		

			if (napiVal.IsNumber())
			{
				key = PyFloat_FromDouble(napiVal.ToNumber().DoubleValue()); //PyFloat_FromDouble (New)
			}
			else
			{
				key = PyUnicode_FromString(napiVal.As<Napi::String>().Utf8Value().c_str()); //PyUnicode_FromString (New)	
			}

			napi_value napi_val;
			NAPI_DIRECT_FUNC(napi_get_property, napi_obj, napi_ele, &napi_val);
			PyObject* res = pyjs::Js_ConvertToPython(env, Napi::Value(env, napi_val), filters).first;
			PyDict_SetItem(obj, key, res); //PyDict_SetItem (Nothing)
			Py_XDECREF(key);
			Py_XDECREF(res);
		}
	}

	if (val.IsSymbol())
	{
		NAPI_ERROR(env, "Unable to marshal Javascript type 'Symbol' to Python.");
	}

	//Napi has no good way of returning if something is a Date type yet...
	//instanceof is unreliable in some cases
	PyJsSpecialObjectType special_type = 
		pyjs::PyjsConfigurationOptions::CheckJSSpecialType(val); 

	if (special_type == PyJsSpecialObjectType::JSDateTime)
	{
		auto callback = NapiPyObject::serialization_callback_.Call(
			{
				Napi::Number::New(env, PyObjectType::_JS_DateTime),
				val
			}
		);

		NapiPyObject* _npo = Napi::ObjectWrap<NapiPyObject>::Unwrap(callback.As<Napi::Object>());
		obj = _npo->GetPyObject(env);
		Py_INCREF(obj); //Clone.

		pot = PyObjectType::DateTime;
	}
	else if (obj == nullptr) //This shouldn't happen.
	{
		NAPI_ERROR(env, "Unable to marshal unknown Javascript type.");
	}

	if (env.IsExceptionPending())
	{
		Py_XDECREF(obj);
	}

	return std::make_pair(obj, pot);
}

////////////////////////////////////////////
// Python -> Javascript Marshalling
////////////////////////////////////////////

Napi::Value pyjs::Py_ConvertToJavascript(const Napi::Env env, PyObject* obj,
	const std::unique_ptr<const std::vector<Napi::Function>>& filters,
	std::unique_ptr<std::unordered_map<PyObject*,napi_value>>& python_to_javascript_map,
	const pyjs::MarshallingOptions& marshalling_options)
{
	PY_CHECK_START();
	Napi::Value napiValue = env.Null();

	//None
	if (obj == NULL)
	{
		napiValue = env.Undefined();
	}
	else if (obj == Py_None)
	{
		napiValue = env.Null();
	}
	//Integer
	else if (PyLong_CheckExact(obj))
	{
		//TODO: Create a faster/better conversion in the future.
		PyObject* str = PyObject_Str(obj); //PyObject_Str (New)
		std::string strs = PyUnicode_AsUTF8(str);
		napiValue = NapiPyObject::serialization_callback_.Call(
			{
				Napi::Number::New(env, PyObjectType::Integer),
				Napi::String::New(env, strs.c_str())
			}
		);
		Py_DECREF(str);
	}
	//Boolean
	else if (PyBool_Check(obj))
	{
		if (obj == Py_False)
		{
			napiValue = Napi::Boolean::New(env, false);
		}
		else
		{
			napiValue = Napi::Boolean::New(env, true);
		}
	}
	//Floating Point
	else if (PyFloat_CheckExact(obj))
	{
		//This should work, but Python seems to be platform specific.
		napiValue = Napi::Number::New(env, PyFloat_AsDouble(obj));
	}
	//Complex Number
	else if (PyComplex_CheckExact(obj))
	{
		napi_value real = Napi::Value(Napi::Number::New(env, PyComplex_RealAsDouble(obj)));
		napi_value imag = Napi::Value(Napi::Number::New(env, PyComplex_ImagAsDouble(obj)));

		NAPI_DIRECT_START(env);
		napi_value napi_array;
		NAPI_DIRECT_FUNC(napi_create_array_with_length, 2, &napi_array);
		NAPI_DIRECT_FUNC(napi_set_element, napi_array, 0, real);
		NAPI_DIRECT_FUNC(napi_set_element, napi_array, 1, imag);

		napiValue = NapiPyObject::serialization_callback_.Call(
			{
				Napi::Number::New(env, PyObjectType::Complex),
				napi_array
			}
		);
	}
	//Bytes
	else if (PyBytes_CheckExact(obj))
	{
		//Copy these by default, and then let the user decide if they want to get the raw pointers, no copy.
		/*char* bytes = PyBytes_AsString(obj);
		NapiPyObjectHelperContainer<char>* container = new NapiPyObjectHelperContainer<char>(obj);
		auto finalizer = [container](Napi::Env e, char* b) {
			container->Finalize(e,b);
		};
		napiValue = Napi::Buffer<char>::New(env, bytes, PyBytes_Size(obj), finalizer);*/

		char* bytes = PyBytes_AS_STRING(obj); //TODO: can this fail? check above too
		napiValue = Napi::Buffer<char>::Copy(env, bytes, PyBytes_Size(obj));
	}
	//Byte Array
	else if (PyByteArray_CheckExact(obj))
	{
		char* bytes = PyByteArray_AS_STRING(obj); //TODO: can this fail? check above too
		napiValue = Napi::Buffer<char>::Copy(env, bytes, PyByteArray_Size(obj));
	}
	//Unicode
	else if (PyUnicode_CheckExact(obj))
	{
		std::string str = PyUnicode_AsUTF8(obj);
		napiValue = Napi::String::New(env, str.c_str());
	}
	//Tuple
	else if (PyTuple_CheckExact(obj))
	{
		auto it = python_to_javascript_map->find(obj);
		if (it != python_to_javascript_map->end())
		{
			return Napi::Value(env, it->second);
		}

		NAPI_DIRECT_START(env);
		napi_value napi_array;
		Py_ssize_t size = PyTuple_GET_SIZE(obj);
		NAPI_DIRECT_FUNC(napi_create_array_with_length, size, &napi_array);

		python_to_javascript_map->insert(std::make_pair(obj, napi_array));

		for (Py_ssize_t i = 0; i < size; i++)
		{
			PyObject* itm = PyTuple_GET_ITEM(obj, i); //PyTuple_GET_ITEM (Borrowed)
			Napi::Value val;
			val = Py_ConvertToJavascript(env, itm, filters, 
				python_to_javascript_map, marshalling_options);

			val = NapiPyObject::serialization_callback_.Call(
			{
				Napi::Number::New(env, PyObjectType::_JS_Wrap),
				val
			});

			NAPI_DIRECT_FUNC(napi_set_element, napi_array, i, val);
		}

		napiValue = Napi::Value(env, napi_array);
	}
	//List
	else if (PyList_CheckExact(obj))
	{
		auto it = python_to_javascript_map->find(obj);
		if (it != python_to_javascript_map->end())
		{
			return Napi::Value(env, it->second);
		}

		NAPI_DIRECT_START(env);
		napi_value napi_array;
		Py_ssize_t size = PyList_Size(obj);
		NAPI_DIRECT_FUNC(napi_create_array_with_length, size, &napi_array);

		python_to_javascript_map->insert(std::make_pair(obj, napi_array));

		for (Py_ssize_t i = 0; i < size; i++)
		{
			PyObject* itm = PyList_GET_ITEM(obj, i); //PyList_GET_ITEM (Borrowed)
			Napi::Value val = Py_ConvertToJavascript(env, itm, filters,
				python_to_javascript_map, marshalling_options);

			val = NapiPyObject::serialization_callback_.Call(
			{
				Napi::Number::New(env, PyObjectType::_JS_Wrap),
				val
			});

			NAPI_DIRECT_FUNC(napi_set_element, napi_array, i, val);
		}

		napiValue = Napi::Value(env, napi_array);
	}
	//Dictionary
	else if (PyDict_CheckExact(obj))
	{
		auto it = python_to_javascript_map->find(obj);
		if (it != python_to_javascript_map->end())
		{
			return Napi::Value(env, it->second);
		}

		Napi::Value map = NapiPyObject::serialization_callback_.Call(
		{
			Napi::Number::New(env, PyObjectType::Dictionary),
			env.Null()
		});

		python_to_javascript_map->insert(std::make_pair(obj, map));

		Py_ssize_t pos = 0;
		PyObject *key, *val;

		while (PyDict_Next(obj, &pos, &key, &val)) //PyDict_Next (Borrow)
		{
			Napi::Value n_key = Py_ConvertToJavascript(env, key, filters, 
				python_to_javascript_map, marshalling_options);
			Napi::Value n_val = Py_ConvertToJavascript(env, val, filters, 
				python_to_javascript_map, marshalling_options);

			NapiPyObject::serialization_callback_.Call(
			{
				Napi::Number::New(env, PyObjectType::Dictionary),
				map,
				n_key,
				n_val
			});
		}

		napiValue = map;
	}
	//Set
	else if (PyAnySet_CheckExact(obj))
	{
		auto it = python_to_javascript_map->find(obj);
		if (it != python_to_javascript_map->end())
		{
			return Napi::Value(env, it->second);
		}

		auto size = PySet_GET_SIZE(obj);
		auto list = PySequence_List(obj); //PySequence_List (New)
		NAPI_DIRECT_START(env);
		napi_value napi_array;
		NAPI_DIRECT_FUNC(napi_create_array_with_length, size, &napi_array);

		python_to_javascript_map->insert(std::make_pair(obj, napi_array));

		for (Py_ssize_t i = 0; i < size; i++)
		{
			PyObject* itm = PySequence_Fast_GET_ITEM(list, i); //PySequence_Fast_GET_ITEM (Borrowed)
			Napi::Value val = Py_ConvertToJavascript(env, itm, filters,
				python_to_javascript_map, marshalling_options);

			val = NapiPyObject::serialization_callback_.Call(
			{
				Napi::Number::New(env, PyObjectType::_JS_Wrap),
				val
			});

			NAPI_DIRECT_FUNC(napi_set_element, napi_array, i, val);
		}

		Py_DECREF(list);

		napiValue = NapiPyObject::serialization_callback_.Call(
		{
			Napi::Number::New(env, PyObjectType::Set),
			napi_array
		});
	}
	//Instance Method
	else if (PyInstanceMethod_Check(obj))
	{
		NAPI_ERROR(env, "Python Type (<class 'instance method'>) is not currently supported.");
	}
	//Cell
	else if (PyCell_Check(obj))
	{
		NAPI_ERROR(env, "Python Type (<class 'cell'>) is not currently supported.");
	}
	//Code
	else if (PyCode_Check(obj))
	{
		NAPI_ERROR(env, "Python Type (<class 'code'>) is not currently supported.");
	}
	//Send to marshaller as object
	else
	{
		auto it = python_to_javascript_map->find(obj);
		if (it != python_to_javascript_map->end())
		{
			return Napi::Value(env, it->second);
		}

		napiValue = NapiPyObject::NewInstance(env, {});
		NapiPyObject* npo = Napi::ObjectWrap<NapiPyObject>::Unwrap(napiValue.As<Napi::Object>());
		npo->SetPyObject(env, obj);

		if (PyType_Check(obj))
			npo->SetObjectType(PyObjectType::Type);
		else if (PyFunction_Check(obj))
			npo->SetObjectType(PyObjectType::Function);
		else if (PyMethod_Check(obj))
			npo->SetObjectType(PyObjectType::Method);
		else
			npo->SetObjectType(PyObjectType::Object);

		Py_INCREF(obj); //NapiPyObject (Future Delete)

		python_to_javascript_map->insert(std::make_pair(obj, napiValue));
	}

	return napiValue;
}

////////////////////////////////////////////
// Configuration Options & Filters
////////////////////////////////////////////

Napi::FunctionReference
	pyjs::PyjsConfigurationOptions::js_type_checking_callback_;

Napi::FunctionReference
	pyjs::PyjsConfigurationOptions::serialization_filter_callback_;

Napi::FunctionReference
	pyjs::PyjsConfigurationOptions::unmarshalling_filter_callback_;

std::unique_ptr<napi_ext::ThreadSafeCallback>
	pyjs::PyjsConfigurationOptions::debug_messaging_callback_;

const std::unique_ptr<const std::vector<Napi::Function>>
	pyjs::PyjsConfigurationOptions::GetSerializationFilters()
{
	std::unique_ptr<std::vector<Napi::Function>> filters(new std::vector<Napi::Function>());
	Napi::Value filter_object_ = pyjs::PyjsConfigurationOptions::serialization_filter_callback_
		.Call({ });

	if (!filter_object_.IsUndefined())
	{
		Napi::Object obj = filter_object_.As<Napi::Object>();
		filters->emplace_back(obj.Get("SequenceTest").As<Napi::Function>());
		filters->emplace_back(obj.Get("SequenceRegister").As<Napi::Function>());
		filters->emplace_back(obj.Get("Finalize").As<Napi::Function>());
	}

	return filters;
}

Napi::Value pyjs::PyjsConfigurationOptions::SetJSTypeCheckingCallback(const Napi::CallbackInfo &info)
{
	Napi::Env env = info.Env();
	js_type_checking_callback_ = Napi::Persistent(info[0].As<Napi::Function>());
	js_type_checking_callback_.SuppressDestruct();
	return env.Undefined();
}

Napi::Value pyjs::PyjsConfigurationOptions::SetSerializationFiltersConstructor(const Napi::CallbackInfo &info)
{
	Napi::Env env = info.Env();
	//Don't set this more than once...
	serialization_filter_callback_ = Napi::Persistent(info[0].As<Napi::Function>());
	serialization_filter_callback_.SuppressDestruct();
	return env.Undefined();
}

Napi::Object pyjs::PyjsConfigurationOptions::Init(Napi::Env env, Napi::Object exports)
{
	exports.Set("$SetJSTypeCheckingCallback", 
		Napi::Function::New(env, pyjs::PyjsConfigurationOptions::SetJSTypeCheckingCallback));
	exports.Set("$SetUnmarshallingFilter",
		Napi::Function::New(env, pyjs::PyjsConfigurationOptions::SetUnmarshallingFilter));
	exports.Set("$SetSerializationCallbackConstructor",
		Napi::Function::New(env, NapiPyObject::SetSerializationCallBackConstructor));
	exports.Set("$SetSerializationFilterConstructor",
		Napi::Function::New(env, pyjs::PyjsConfigurationOptions::SetSerializationFiltersConstructor));
	exports.Set("$SetDebugMessagingCallback",
		Napi::Function::New(env, pyjs::PyjsConfigurationOptions::SetDebugMessagingCallback));

	return exports;
}

Napi::Value pyjs::PyjsConfigurationOptions::SetUnmarshallingFilter(const Napi::CallbackInfo &info)
{
	Napi::Env env = info.Env();
	unmarshalling_filter_callback_ = Napi::Persistent(info[0].As<Napi::Function>());
	unmarshalling_filter_callback_.SuppressDestruct();
	return env.Undefined();
}

NapiPyObject* pyjs::PyjsConfigurationOptions::AttemptNapiObjectUnmarshalling(Napi::Env env, Napi::Value obj)
{
	Napi::Value filter_output_ = pyjs::PyjsConfigurationOptions::unmarshalling_filter_callback_
		.Call({ obj });

	if (!filter_output_.IsUndefined())
	{
		return Napi::ObjectWrap<NapiPyObject>::Unwrap(filter_output_.As<Napi::Object>());
	}
	else
		return NULL;
}

PyJsSpecialObjectType pyjs::PyjsConfigurationOptions::CheckJSSpecialType(const Napi::Value val)
{
	return (PyJsSpecialObjectType)js_type_checking_callback_.Call({ val }).ToNumber().Int32Value();
}

Napi::Value pyjs::PyjsConfigurationOptions::SetDebugMessagingCallback(const Napi::CallbackInfo &info)
{
	Napi::Env env = info.Env();
	pyjs::PyjsConfigurationOptions::debug_messaging_callback_ = 
		std::make_unique<napi_ext::ThreadSafeCallback>(info[0].As<Napi::Function>());
	debug_messaging_callback_->unref();

	return env.Undefined();
}

void pyjs::PyjsConfigurationOptions::SendDebugMessage(std::vector<std::string> msg)
{
	#ifdef __PY__DEBUG
	pyjs::PyjsConfigurationOptions::debug_messaging_callback_->call(
		[msg](Napi::Env env, std::vector<napi_value>& args)
	{
		std::vector<napi_value> nv (msg.size());
		std::transform(msg.begin(), msg.end(), nv.begin(), [env](std::string s) {
			return Napi::String::New(env, s);
		});

		args = nv;
	});
	#endif
}

#ifdef __PY__DEBUG
bool pyjs::PyjsConfigurationOptions::IsDebugEnabled()
{
	return true;
}
#else
bool pyjs::PyjsConfigurationOptions::IsDebugEnabled()
{
	return false;
}
#endif

////////////////////////////////////////////
// Base Module Setup & Functions
////////////////////////////////////////////

static Napi::Value Import(const Napi::CallbackInfo &info)
{
	Napi::Env env = info.Env();

	PyObject* module_name;
	PyObject* module;

	PY_CHECK_START();
	//module_name = PyUnicode_DecodeFSDefault(info[0].ToString().Utf8Value().c_str());
	module_name = PyUnicode_FromString(info[0].ToString().Utf8Value().c_str()); //PyUnicode_FromString (New)
	module = PyImport_Import(module_name); //PyImport_Import (New)
	Py_XDECREF(module_name);

	PY_CHECK(env, module, NULL, env.Undefined());

	auto map = std::unique_ptr<std::unordered_map<PyObject*,napi_value>>
		(new std::unordered_map<PyObject*,napi_value>());

	Napi::Value napiValue = pyjs::Py_ConvertToJavascript(env, module,
		pyjs::PyjsConfigurationOptions::GetSerializationFilters(), map,
		pyjs::MarshallingOptions());

	Py_XDECREF(module);

	return napiValue;
}

static Napi::Value EvalHelper(const Napi::CallbackInfo &info, int type)
{
	Napi::Env env = info.Env();

	PY_CHECK_START()

	PyObject* code = Py_CompileString( //Py_CompileString (New)
		info[0].As<Napi::String>().Utf8Value().c_str(), 
		info[1].As<Napi::String>().Utf8Value().c_str(), type);
	PY_CHECK(env, code, NULL, env.Undefined());

	PyObject* main = PyImport_AddModule("__main__"); //PyImport_AddModule (Borrow)
	PY_CHECK(env, main, NULL, env.Undefined());

	PyObject* global = PyModule_GetDict(main); //PyModule_GetDict (Borrow)
	PY_CHECK(env, global, NULL, env.Undefined());

	PyObject* local = PyDict_New(); //PyDict_New (New)
	PY_CHECK(env, local, NULL, env.Undefined());

	PyObject* obj = PyEval_EvalCode(code, global, local); //PyEval_EvalCode (New)
	PY_CHECK(env, obj, NULL, env.Undefined());

	Py_XDECREF(code);
	Py_XDECREF(local);

	auto map = std::unique_ptr<std::unordered_map<PyObject*,napi_value>>
		(new std::unordered_map<PyObject*,napi_value>());
	auto res = pyjs::Py_ConvertToJavascript(env, obj,
		pyjs::PyjsConfigurationOptions::GetSerializationFilters(), 
		map, pyjs::MarshallingOptions());

	Py_XDECREF(obj);

	return res;
}

static Napi::Value Eval(const Napi::CallbackInfo &info)
{
	return EvalHelper(info, Py_eval_input);
}

static Napi::Value EvalAsFile(const Napi::CallbackInfo &info)
{
	return EvalHelper(info, Py_file_input);
}

static Napi::Value Global(const Napi::CallbackInfo &info)
{
	Napi::Env env = info.Env();

	//Check if static reference is available
	if (__py__main__module_ == NULL)
		return env.Undefined();

	auto map = std::unique_ptr<std::unordered_map<PyObject*,napi_value>>
		(new std::unordered_map<PyObject*,napi_value>());	

	return pyjs::Py_ConvertToJavascript(env, __py__main__module_,
		pyjs::PyjsConfigurationOptions::GetSerializationFilters(), 
		map, pyjs::MarshallingOptions());	
}

Napi::Object InstanceInformation(const Napi::CallbackInfo &info)
{
	Napi::Env env = info.Env();

	Napi::Object obj = Napi::Object::New(env);
	obj.Set("python_version", Napi::String::New(env, Py_GetVersion()));

	if (Py_GetPythonHome() == NULL)
	{
		obj.Set("python_home", env.Null());
	}
	else
	{
		PyObject* pythonHome = PyUnicode_FromWideChar(Py_GetPythonHome(), -1);
		std::string pythonHomeStr = PyUnicode_AsUTF8(pythonHome);
		Napi::String pythonHomeNapiStr = Napi::String::New(env, pythonHomeStr.c_str());
		obj.Set("python_home", pythonHomeNapiStr);
		Py_DECREF(pythonHome);
	}

	return obj;
}

////////////////////////////////////////////
// Python Capsule Bridge & Module Setup
////////////////////////////////////////////

PyObject* pyjs::FunctionBridgeRegisterCallback(const Napi::Env& env, Napi::Function f)
{
	lock_gil lock_me;

	napi_ext::ThreadSafeCallback* callback = new napi_ext::ThreadSafeCallback(f);
	callback->unref();
	//and into integer for long-term Python storage...
	auto tmp = PyLong_FromSsize_t((Py_ssize_t)callback);
	return tmp;
}

static PyObject* PyCapsuleNodeJSInterfaceFunctionCall(PyObject* self, PyObject* args)
{
	PyObject* ptr_obj;
	PyObject* cb_args;
	napi_ext::ThreadSafeCallback* callback;

	{
		lock_gil lock_me;

		ptr_obj = PyTuple_GET_ITEM(args, 0); //PyTuple_GET_ITEM (Borrowed)
		cb_args = PyTuple_GET_ITEM(args, 1); //PyTuple_GET_ITEM (Borrowed)
		Py_INCREF(cb_args); //Keep until we're done with async callback.

		Py_ssize_t ptr = PyLong_AsSsize_t(ptr_obj);
		callback = (napi_ext::ThreadSafeCallback*)ptr;
	}

	PyObject* res;

	{
		//Release GIL so we don't deadlock during async.
		Py_BEGIN_ALLOW_THREADS

		//The following lambdas run on the main loop.
		auto future =
			callback->call<PyObject*>([cb_args](Napi::Env env, std::vector<napi_value>& args)
			{
				auto map = std::unique_ptr<std::unordered_map<PyObject*,napi_value>>
					(new std::unordered_map<PyObject*,napi_value>());

				auto js = pyjs::Py_ConvertToJavascript(env, cb_args,
					pyjs::PyjsConfigurationOptions::GetSerializationFilters(),
					map, pyjs::MarshallingOptions());

				Py_DECREF(cb_args); //We can get rid of args now.

				args = { js };
			}, [](const Napi::Value &napiValue)
			{
				Napi::Env env = napiValue.Env();
				Napi::HandleScope scope(env);

				const std::unique_ptr<const std::vector<Napi::Function>>&
					serialization_filters = pyjs::PyjsConfigurationOptions::GetSerializationFilters();
				std::pair<PyObject*,PyObjectType> conv = pyjs::Js_ConvertToPython(env, napiValue,
					serialization_filters);

				return conv.first;
			});


		//Block Python async thread (python_loop_thread) while waiting for result.
		res = future.get();

		Py_END_ALLOW_THREADS
	}

	return res;
}

static PyObject* PyCapsuleNodeJSInterfaceDestroyFunction(PyObject* self, PyObject* args)
{
	Py_ssize_t ptr;

	{
		lock_gil lock_me;
		PyObject* ptr_obj = PyTuple_GET_ITEM(args, 0); //PyTuple_GET_ITEM (Borrowed)

		//TODO queue this, might be in the middle of processing, will have to tag
		ptr = PyLong_AsSsize_t(ptr_obj);
	}

	napi_ext::ThreadSafeCallback* callback = (napi_ext::ThreadSafeCallback*)ptr;
	delete callback;


	Py_RETURN_TRUE;
}

static PyObject* PyCapsuleNodeJSInterfaceGetCurrentThreadID(PyObject* self, PyObject* args)
{
	PyObject* ret;

	{
		lock_gil lock_me;
		std::thread::id this_id = std::this_thread::get_id();
		std::stringstream ss;
		ss << this_id;

		//PyUnicode_FromString (New)
		ret = PyUnicode_FromString(ss.str().c_str());
	}

	return ret;
}

static PyObject* PyCapsuleNodeJSInterfaceDebugEnabled(PyObject* self, PyObject* args)
{
	if (pyjs::PyjsConfigurationOptions::IsDebugEnabled())
	{
		Py_RETURN_TRUE;
	}
	else
	{
		Py_RETURN_FALSE;
	}
}

static PyObject* PyCapsuleNodeJSInterfaceSendDebugMessage(PyObject* self, PyObject* args)
{
	std::vector<std::string> d;
	Py_ssize_t length = PyTuple_GET_SIZE(args);
	for (Py_ssize_t i = 0; i < length; i++)
	{
		PyObject* obj = PyTuple_GET_ITEM(args, i);
		if (PyUnicode_CheckExact(obj))
		{
			d.emplace_back(PyUnicode_AsUTF8(obj));
		}
	}

	if (d.size() > 0)
	{
		d.emplace_back(pyjs_utils::GetCurrentThreadID());
		d.emplace_back(std::to_string(pyjs_utils::GetCurrentTimeTicks()));
		pyjs::PyjsConfigurationOptions::SendDebugMessage(d);
	}

	Py_RETURN_NONE;
}

static PyMethodDef PyCapsuleNodeJsMethods[] = 
{
	{
		"_callback",
		PyCapsuleNodeJSInterfaceFunctionCall,
		METH_VARARGS,
		"calls the node.js/python bridge." 
	},
	{
		"_callback_destroy",
		PyCapsuleNodeJSInterfaceDestroyFunction,
		METH_VARARGS,
		"destroys a registered function on the node.js/python bridge." 
	},
	{
		"_get_current_thread_id",
		PyCapsuleNodeJSInterfaceGetCurrentThreadID,
		METH_VARARGS,
		"gets the current thread id from the node.js/python interface." 
	},
	{
		"_debug_enabled",
		PyCapsuleNodeJSInterfaceDebugEnabled,
		METH_VARARGS,
		"gets the debug state of the node.js/python interface."
	},
	{
		"_debug",
		PyCapsuleNodeJSInterfaceSendDebugMessage,
		METH_VARARGS,
		"sends debug information to the node.js/python interface."
	}, { NULL, NULL, 0, NULL }
};

static PyModuleDef PyCapsuleNodeJsModule =
{
	PyModuleDef_HEAD_INIT,
	"__pyjs",
	NULL,
	-1, 
	PyCapsuleNodeJsMethods,
	NULL, NULL, NULL, NULL
};

////////////////////////////////////////////
// Init & Finalize
////////////////////////////////////////////

static PyObject* PyjsModuleInitAll(void)
{
	//Keep static reference
	__pyjs_module_ = PyModule_Create(&PyCapsuleNodeJsModule); //PyModule_Create (New)

	return __pyjs_module_;
}

static Napi::Value Initialize(const Napi::CallbackInfo &info)
{	
	Napi::Env env = info.Env();

	PY_DEBUG("py.js says hello. debugging enabled.");

	pyjs_async::StartMainPythonLoop(info);

	int res = PyImport_AppendInittab("__pyjs", &PyjsModuleInitAll);
	if (res < 0 || PyErr_Occurred())
		pyjs_utils::ThrowPythonException(env);

	Py_InitializeEx(1);
	PyEval_InitThreads(); //compatibility with older versions of Python 3 

	PyImport_ImportModule("__pyjs"); //PyImport_ImportModule (New)
	//Purposely keep one active.

	__py__main__module_ = PyImport_AddModule("__main__"); //PyImport_AddModule (Borrowed)
	if (__py__main__module_ == NULL || PyErr_Occurred())
		pyjs_utils::ThrowPythonException(env);
	Py_INCREF(__py__main__module_); //Keep static reference.

	PY_DEBUG("py.js initialized.");

	return env.Undefined();
}

static Napi::Value BeginFinalize(const Napi::CallbackInfo &info)
{
	Napi::Env env = info.Env();

	PY_DEBUG("py.js says goodbye. finalize called.");
	pyjs_async::DestroyAsyncHandlers();
	Py_XDECREF(__pyjs_module_);

	return env.Undefined();
}

////////////////////////////////////////////
// Pre-Init & Setup
////////////////////////////////////////////

/*static Napi::Value PreInit_SetPythonHome(const Napi::CallbackInfo &info)
{
	Napi::Env env = info.Env();

	PyObject* path = PyUnicode_FromString(info[0].ToString().Utf8Value().c_str());
	Py_ssize_t* size = nullptr;
	Py_SetProgramName(PyUnicode_AsWideCharString(path, size));
	Py_XDECREF(path);
	PyMem_Free(size);

	return env.Null();
}

static Napi::Object PreInit_Settings_Object(const Napi::Env env, Napi::Object exports)
{
	Napi::Object obj = Napi::Object::New(env);
	obj.Set("set_python_home", Napi::Function::New(env, PreInit_SetPythonHome));

	return obj;
}*/

static Napi::Object InitAll(Napi::Env env, Napi::Object exports)
{
	//exports.Set("settings", PreInit_Settings_Object(env, exports));
	exports.Set("init", Napi::Function::New(env, Initialize));
	exports.Set("beginFinalize", Napi::Function::New(env, BeginFinalize));
	exports.Set("eval", Napi::Function::New(env, Eval));
	exports.Set("evalAsFile", Napi::Function::New(env, EvalAsFile));
	exports.Set("global", Napi::Function::New(env, Global));
	exports.Set("import", Napi::Function::New(env, Import));
	exports.Set("instance", Napi::Function::New(env, InstanceInformation));
	exports.Set("$GetMarshaledObject", Napi::Function::New(env, NapiPyObject::GetMarshaledObject));
	exports.Set("$IsInstanceOf", Napi::Function::New(env, NapiPyObject::IsInstanceOf));
	NapiPyObject::Init(env, exports);
	pyjs::PyjsConfigurationOptions::Init(env, exports);
	pyjs_async::InitAll(env, exports);
	pyjs_utils::InitAll(env, exports);
	return exports;
}

NODE_API_MODULE(pyjs, InitAll)
