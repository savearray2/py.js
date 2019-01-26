//////////////////////////////////////////////////////////////////////////
//	py.js - Node.js/Python Bridge; Node.js-hosted Python.
//	Copyright (C) 2019  Michael Brown
//
//	This program is free software: you can redistribute it and/or modify
//	it under the terms of the GNU Affero General Public License as
//	published by the Free Software Foundation, either version 3 of the
//  License, or (at your option) any later version.
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

//////////////////////////////////////////////////////////////////////////
// The following "GetPythonException" function was originally published
// by Chris Dickinson:
//
// Copyright (c) 2010, Chris Dickinson
//
// Please see the file "contrib/LICENSE1" for more details.
// Any modifications of this code are to fall under the overarching
// license of this project. Please see the header of this file, or
// "LICENSE" for more details.
//////////////////////////////////////////////////////////////////////////

std::string pyjs_utils::GetPythonException()
{
	PyObject *ptype, *pvalue, *ptraceback;
	PyErr_Fetch(&ptype, &pvalue, &ptraceback);
	PyErr_NormalizeException(&ptype, &pvalue, &ptraceback);

	if(pvalue == NULL)
	{
		return "Exception thrown, but no symbolic data found.";
	}

	std::string msg = "[Python Error] => ";

	if (ptype != NULL)
	{
		PyObject* attrString = PyObject_GetAttrString(ptype, "__name__"); //PyObject_GetAttrString (New)
		PyObject* pyStr = PyObject_Str(attrString); //PyObject_Str (New)
		msg += PyUnicode_AsUTF8(pyStr);
		msg += ": ";
		Py_DECREF(attrString);
		Py_DECREF(pyStr);

		if (pvalue != NULL)
		{
			pyStr = PyObject_Str(pvalue); //PyObject_Str (New)
			msg += PyUnicode_AsUTF8(pyStr);
			Py_DECREF(pyStr);
		}

		msg += "\n";
	}

	if (ptraceback != NULL)
	{
		PyObject *module_name, *pyth_module, *pyth_func;
		module_name = PyUnicode_FromString("traceback"); //PyUnicode_FromString (New)
		pyth_module = PyImport_Import(module_name); //PyImport_Import (New)
		Py_DECREF(module_name);

		pyth_func = PyObject_GetAttrString(pyth_module, "format_exception"); //PyObject_GetAttrString (New)
		Py_DECREF(pyth_func);
		Py_DECREF(pyth_module);

		if (pyth_func)
		{
			PyObject *pyth_val, *pystr, *ret;
			const char *str;

			char *full_backtrace;

			pyth_val = PyObject_CallFunctionObjArgs(pyth_func, ptype, pvalue, ptraceback, NULL); //PyObject_CallFunctionObjArgs (New)
			pystr = PyUnicode_FromString(""); //PyUnicode_FromString (New)
			ret = PyUnicode_Join(pystr, pyth_val); //PyUnicode_Join (New)
			Py_DECREF(pystr);
			pystr = PyObject_Str(ret); //PyObject_Str (New)
			str = PyUnicode_AsUTF8(pystr);
			full_backtrace = strdup(str);

			Py_DECREF(pyth_func);
			Py_DECREF(pyth_val);
			Py_DECREF(pystr);
			Py_DECREF(str);
			Py_DECREF(ret);

			msg += "\n";
			msg += full_backtrace;
		}
		else
		{
			PyObject *pystr;
			const char *str;

			msg += "\n";
			pystr = PyObject_Str(ptraceback); //PyObject_Str (New)
			str = PyUnicode_AsUTF8(pystr);
			msg += str;
			Py_DECREF(pystr);
		}
	}

	return msg;
}

void pyjs_utils::ThrowPythonException(Napi::Env env)
{
	NAPI_ERROR(env, pyjs_utils::GetPythonException());
}