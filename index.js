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
'use strict'

const fs = require('fs')
const path = require('path')
const util = require('util')

const chalk = require('chalk')
const moment = require('moment')


const _pyjs = require('./build/Release/pyjs.node')
const _etc = require('./js/etc.js')
const _pylib = {}

////////////////////////////////////////////
// Debug Handler
////////////////////////////////////////////

const _debug_handler = function() {
	let debug_data = arguments;
	let thread_id = arguments[arguments.length - 2]
	let ticks = arguments[arguments.length - 1]
	ticks = moment(ticks, 'x').format()

	thread_id = thread_id.slice(-9)
	console.log(chalk`[{blue ${ticks}}{gray #}{green ${thread_id}}] {yellow ${debug_data[0]}}`)
}

////////////////////////////////////////////
// Special Type Checker
////////////////////////////////////////////

const _js_type_checking_types = {
	UNKNOWN_JS_TYPE: 0,
	JS_DATETIME: 1
}

const _js_type_checking = (obj) => {
	if (util.isDate(obj))
		return _js_type_checking_types.JS_DATETIME
}

////////////////////////////////////////////
// Setup
////////////////////////////////////////////

_etc.set_pyjs(_pyjs)
_pyjs.$SetDebugMessagingCallback(_debug_handler)
_pyjs.$SetSerializationCallbackConstructor(_etc.default_serializer)
_pyjs.$SetSerializationFilterConstructor(_etc.serializaition_filter)
_pyjs.$SetUnmarshallingFilter(_etc.unmarshalling_filter)
_pyjs.$SetJSTypeCheckingCallback(_js_type_checking)

////////////////////////////////////////////
// Pass-through Functions
////////////////////////////////////////////

const _apply =	(fn) =>
	{ return () => fn.apply(this, arguments) }

let pyjs = {}
let fns = {
	instance: _pyjs.instance,
	global: () => 
		_etc.marshalling_factory(
			_pyjs.global()),
	base: () =>
		_etc.marshalling_factory(
			_pyjs.global()).__builtins__,
	$GetCurrentThreadID: () =>
		_pyjs.$GetCurrentThreadID()
}
for (let f in fns)
	pyjs[f] = _apply(fns[f])

////////////////////////////////////////////
// Library Base Functions
////////////////////////////////////////////

pyjs.import = function () {
	_etc.parameter_check('import', [
			_etc.parameter_check.methods.string ],
			arguments)
	return _etc.marshalling_factory(
		_pyjs.import.apply(this, arguments))
}

pyjs.eval = function (code, {tag} = {}) {
	_etc.parameter_check('eval', [
		_etc.parameter_check.methods.string ],
		arguments)

	code += '\n'
	let s_tag = '=node.js'
	if (_etc.parameter_check.methods.string.f(tag)) {
		s_tag = tag
	}
	else if (!_etc.parameter_check.methods.undefined.f(tag)) {
		throw Error("Option 'tag' must be a string.")
	}
	
	return _etc.marshalling_factory(
		_pyjs.eval(code, s_tag))
}

pyjs.evalAsFile = function (code, path) {
	_etc.parameter_check('evalAsFile', [
		_etc.parameter_check.methods.string,
		_etc.parameter_check.methods.string ],
		arguments)

	code += '\n'
	_pyjs.evalAsFile(code, path)
}

//Shortcuts
//pyjs.e = 

////////////////////////////////////////////
// Helpers (pyjs_common.cpp)
////////////////////////////////////////////

pyjs.$coerceAs = {
	Integer: function(num) {
		if (_etc.parameter_check.methods.number.f(num)) {
			return _etc.marshalling_factory(
				_pyjs.$coerceAs.Integer(num))
		}
		else if (_etc.parameter_check.methods.string.f(num)) {
			return _etc.marshalling_factory(
				_pyjs.$coerceAs.Integer(num))
		}
		else
			throw Error("Unknown type. Unable to coerce as integer.")
	},
	Tuple: function(array) {
		if (!_etc.parameter_check.methods.array.f(array)) {
			throw Error("You must supply an array to coerce as tuple.")
		}

		return _etc.marshalling_factory(
			_pyjs.$coerceAs.Tuple(
				array.map(x => _pyjs.$GetMarshaledObject(x))))
	}
}

//Shortcuts
pyjs.$coerceAs.int = pyjs.$coerceAs.Integer

////////////////////////////////////////////
// Initialization Logic
////////////////////////////////////////////

let _exit_handler;

pyjs.init = ({ 	exitHandler: exit_handler,
				pythonHome: python_home, 
				pythonPath: python_path } = {}) => {

	if (_etc.parameter_check.methods.function.f(exit_handler)) {
		_exit_handler = exit_handler
	}
	else if (!_etc.parameter_check.methods.undefined.f(exit_handler)) {
		throw Error("Option 'exitHandle' must be a function.")
	}

	if (_etc.parameter_check.methods.string.f(python_home)) {
		process.env.PYTHONHOME = python_home
	}
	else if (!_etc.parameter_check.methods.undefined.f(python_home)) {
		throw Error("Option 'pythonHome' must be a string.")
	}

	if (_etc.parameter_check.methods.string.f(python_path)) {
		process.env.PYTHONPATH = python_path
	}
	else if (!_etc.parameter_check.methods.undefined.f(python_path)) {
		throw Error("Option 'pythonPath' must be a string.")
	} 
	
	//Possibly best to refactor this, or maybe just let the user decide on the python path.
	/*if (process.env.PYTHONPATH === undefined
		&& python_path === undefined) {
		try {
			const exec = require('child_process').execSync
			const site = exec('python3 -c "from distutils.sysconfig import get_python_lib; print(get_python_lib())"',
				{encoding: 'utf8'}).trim()
			process.env.PYTHONPATH = `${site}`
		}
		catch {}
	}

	if (process.env.PYTHONPATH === undefined
		&& python_path === undefined) {
		try {
			const exec = require('child_process').execSync
			const ver = exec('python --version', {encoding: 'utf8'}).trim().toLowerCase()
			if (ver.startsWith('python 3')) {
				const site = exec('python -c "from distutils.sysconfig import get_python_lib; print(get_python_lib())"',
					{encoding: 'utf8'}).trim()
				process.env.PYTHONPATH = `${site}`
			}
		}
		catch {}
	}*/

	_pyjs.init()

	let py_path = path.join(__dirname, 'py')
	for (let file of fs.readdirSync(py_path))
	{
		let full_path = path.join(py_path, file)
		let data = fs.readFileSync(full_path, {encoding: 'utf8'})
		_pyjs.evalAsFile(data, full_path)
	}

	_pylib.__pyjs = _etc.marshalling_factory(_pyjs.import('__pyjs'))
	_pylib._callback_factory = _pylib.__pyjs._callback_factory
	_pyjs._pylib = _pylib
	_pylib.exit = _pylib.__pyjs._exit
	_pyjs.pyjs = pyjs
}

////////////////////////////////////////////
// Finalization Logic
////////////////////////////////////////////

let _finalize_listener = () => {
	pyjs.finalize()
	process.exit()
}

pyjs.finalize = () => {
	//Call any handlers if we have them
	if (_exit_handler !== undefined)
		_exit_handler()

	_pyjs.beginFinalize()
	//TODO make sure all async functions are destructed here

	for (let n in pyjs) {
		pyjs[n] = () => {
			throw Error("Finalize has already been called.")	
		}
	}

	//Force Python to clear everything itself, no matter what threads are open
	//Seems that Py_Finalize/Ex sometimes hangs with other threads open
	//try { _pylib.exit() } catch {}

	process.removeListener('SIGINT', _finalize_listener)
	process.removeListener('SIGTERM', _finalize_listener)
}

//Exit is only called when the event loop ends or process.exit is called.
//process.once('exit', pyjs.$destroyAsyncServer)

//Allow user to override defaults here
process.once('SIGINT', _finalize_listener)
process.once('SIGTERM', _finalize_listener)

pyjs.$DisableProcessSigListeners = () => {
	process.removeListener('SIGINT', _finalize_listener)
	process.removeListener('SIGTERM', _finalize_listener)
}

////////////////////////////////////////////
// Initialization Handler
////////////////////////////////////////////

//This needs to be last.
(() => {
	let activated_ = Object.assign({}, pyjs)

	for (let n in pyjs) {
		pyjs[n] = () => {
			throw Error("py.js needs to be initialized. please call init() or wait for python to fully initialize")
		}
	}

	pyjs.init = function() {
		activated_.init.apply(this, arguments)
		for (let n in pyjs) {
			pyjs[n] = activated_[n]
		}

		pyjs.init = () => {
			throw Error("py.js has already been initialized.")
		}

		pyjs.exceptions = require('./js/ex.js')(pyjs, _etc)

		return pyjs
	}
})()

////////////////////////////////////////////
// Export
////////////////////////////////////////////

module.exports = pyjs