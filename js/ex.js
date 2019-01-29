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
'use strict'

module.exports = (p, _etc) => {
	let base = p.base()
	let isinstance = base.isinstance
	let exceptions = getExceptions(base)

	class PythonException extends Error {
		constructor(message, python_exception) {
			super(message)
			
			let obj = _etc.get_raw_object(python_exception)

			try {
				let n = obj.GetAttribute('__class__', null)
				n = n.GetAttribute('__name__', null)
				this.name = `PythonException [#${n}]`
				this.py_name = n
			}
			catch {
				this.name = "PythonException"
				this.py_name = undefined
			}

			this.pythonException = python_exception
		}

		IsStopIterationException() {
			return isinstance(this.pythonException, exceptions.StopIteration)
		}
	}

	return () => {
		return {
			PythonException
		}
	}
}

let getExceptions = (base) => {
	return {
		Exception: base.Exception,
		AssertionError: base.AssertionError,
		AttributeError: base.AttributeError,
		EOFError: base.EOFError,
		FloatingPointError: base.FloatingPointError,
		GeneratorExit: base.GeneratorExit,
		ImportError: base.ImportError,
		ModuleNotFoundError: base.ModuleNotFoundError,
		IndexError: base.IndexError,
		KeyError: base.KeyError,
		KeyboardInterrupt: base.KeyboardInterrupt,
		MemoryError: base.MemoryError,
		NameError: base.NameError,
		NotImplementedError: base.NotImplementedError,
		OSError: base.OSError,
		OverflowError: base.OverflowError,
		RecursionError: base.RecursionError,
		ReferenceError: base.ReferenceError,
		RuntimeError: base.RuntimeError,
		StopIteration: base.StopIteration,
		StopAsyncIteration: base.StopAsyncIteration,
		SyntaxError: base.SyntaxError,
		IndentationError: base.IndentationError,
		TabError: base.TabError,
		SystemError: base.SystemError,
		SystemExit: base.SystemExit,
		TypeError: base.TypeError,
		UnboundLocalError: base.UnboundLocalError,
		UnicodeError: base.UnicodeError,
		UnicodeEncodeError: base.UnicodeEncodeError,
		UnicodeDecodeError: base.UnicodeDecodeError,
		UnicodeTranslateError: base.UnicodeTranslateError,
		ValueError: base.ValueError,
		ZeroDivisionError: base.ZeroDivisionError
	}
}