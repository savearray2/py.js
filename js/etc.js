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

const util = require('util')
util.inspect.styles.pyobject = 'italic'

const _etc = {}
const _local = {}

_etc.set_pyjs = (pyjs) => {
	_local._pyjs = pyjs
}

////////////////////////////////////////////
// Serialization/Marshalling Logic
////////////////////////////////////////////

_etc.python_object_type = () => {
	let obj_type = [
		'OBJECT',
		'NONE',
		'INTEGER',
		'BOOL',
		'FLOAT',
		'COMPLEX',
		'BYTES',
		'BYTEARRAY',
		'UNICODE',
		'TUPLE',
		'LIST',
		'DICTIONARY',
		'SET',
		'FUNCTION',
		'METHOD',
		'TYPE',
		'DATETIME',
		'_JS_DATETIME',
		'_JS_FUNCTION',
		'_JS_WRAP',
		'PYTHON_EXCEPTION',
		'UNSUPPORTED'
	]

	let types = {}
	for (let i = 0; i < obj_type.length; i++)
		types[obj_type[i]] = i
	return types
}

_etc.python_object_type = _etc.python_object_type()

_etc.default_serializer = (type, obj) => _local.serializers[type](type, obj)

_local.serializers = {
	[_etc.python_object_type.INTEGER]: (type, obj) => BigInt(obj),
	[_etc.python_object_type.COMPLEX]: (type, obj) => new _etc.python_types.Complex(obj[0], obj[1]),
	//[_etc.python_object_type.TUPLE]: (type, obj) => new _etc.python_types.Tuple(obj),
	[_etc.python_object_type.DICTIONARY]: (type, obj) => _etc.python_types.Dictionary(obj),
	[_etc.python_object_type.SET]: (type, obj) => _etc.python_types.Set(obj),
	[_etc.python_object_type.PYTHON_EXCEPTION]: (type, obj) => _etc.python_types.Exception(obj),
	[_etc.python_object_type._JS_DATETIME]: (type, obj) => _etc.python_types._js_datetime(obj),
	[_etc.python_object_type._JS_FUNCTION]: (type, obj) => _etc.python_types._js_function(obj),
	[_etc.python_object_type._JS_WRAP]: (type, obj) => _etc.python_types._js_wrap(obj)
}

_local.type_formatter = (d, o, obj) => {
	if (_etc.parameter_check.methods.string.f(obj)) {
		return `${o.stylize("'" + obj + "'", 'string')}`
	}
	else if (_etc.parameter_check.methods.number.f(obj)) {
		return `${o.stylize(obj, 'number')}`
	}
	else if (_etc.parameter_check.methods.null.f(obj)) {
		return `${o.stylize('null', 'null')}`
	}
	else if (_etc.parameter_check.methods.undefined.f(obj)) {
		return `${o.stylize('undefined', 'undefined')}`
	}
	else if (Array.isArray(obj)) {
		let no = Object.assign({}, o, {
			depth: o.depth === null ? null : o.depth - 1
		})

		return `${util.inspect(obj, no)}`
	}
	else {
		let no = Object.assign({}, o, {
			depth: o.depth === null ? null : o.depth - 1,
			__py_sub: true
		})

		return `${util.inspect(obj, no)}`
	}
}

_etc.python_types = {
	Complex: class Complex {
		constructor(real, imag) {
			this.real = real
			this.imag = imag
		}

		toString() {
			let s = this.imag > 0 ? '+' : ''
			let r = this.real === 0 ? '' : `${this.real}${s}`
			let i = this.imag === 0 ? '' : `${this.imag}i`
			return r+i
		}

		[util.inspect.custom](depth, options) {
			let s = this.imag > 0 ? '+' : ''
			let r = this.real === 0 ? '' : `${options.stylize(this.real, 'number')}${s}`
			let i = this.imag === 0 ? '' : `${options.stylize(this.imag + 'i', 'number')}`
			return `${options.stylize('Python Proxy', 'pyobject')} <class ${options.stylize("'complex'", 'string')}> { ${r}${i} }`
		}
	},
	//Tuple proxy is not required at this time...
	//Might add it back in later if the need is there,
	//or completely remove this code.
	/*Tuple: class Tuple {
		constructor(arr) {
			this.array = arr
		}

		toString() {
			return `(${this.array.join(', ')})`
		}

		[util.inspect.custom](depth, options) {
			if (depth < 0)
			{
				return `( ... )`
			}

			let subopts = Object.assign({}, options, {
				depth: options.depth === null ? null : options.depth - 1,
				__py_sub: true
			})

			let items = []
			let large = false
			let i = 0
			for (let itm of this.array) {
				if (i > subopts.maxArrayLength) {
					large = true
					break
				}
				items.push(_local.type_formatter(depth, subopts, itm))
				i++
			}

			items = items.join(', ') + (large ? '...' : '')
			let tag = `${subopts.stylize('Python Proxy', 'pyobject')} <class ${subopts.stylize("'tuple'", 'string')}> `
			if (options.__py_sub)
				tag = ""

			return `${tag}( ${items} )`
		}
	},*/
	Dictionary: (obj) => {
		let map = new Map()
		for (let i = 0; i < obj.length; i++) {
			let pair = obj[i]
			map.set(
				_etc.marshalling_factory(pair.key), 
				_etc.marshalling_factory(pair.value)
			)
		}

		return map
	},
	Set: (obj) => {
		return new Set(obj)
	},
	Exception: (obj) => {
		let ex = _local._pyjs.pyjs
			.exceptions().PythonException
		let ret = new ex(obj.message, 
			_etc.marshalling_factory(obj.exception))
		return ret
	},
	_js_datetime: (obj) => {
		let datetime = _local._pyjs.pyjs.import('datetime').datetime
		let dt = datetime.fromtimestamp(obj.getTime() / 1000)
		return dt[_local.marshaled_object_tag]
	},
	_js_function: (ptr) => {
		//get a normal (non-async) version of the callback_factory
		//we can't invoke otherwise (we'd get undefined back in FunctionCallAsync)
		let wrapped = _local._pyjs._pylib._callback_factory.$mode({asyncOverride:true})(ptr)
		let wrapper = wrapped[_local.marshaled_object_tag]
		return wrapper
	},
	_js_wrap: (obj) => {
		return _etc.marshalling_factory(obj)
	}
}

_local.object_attributes = {
	all: {
		//Proxy & Marshalling Helpers
		$isCallable: (t) => t.$isCallable,
		$isClass: (t) => t.$isClass,
		$isIterable: (t) => t.$isIterable,
		$getType: (t) => t.$getType,
		$newMode: (t) => t.$newMode,
		$getMode: (t) => t.$getMode,
		$mode: (t) => t.$mode,
		//Async handler
		$async: (t) => function (fn) {
			if (fn !== undefined 
				&& !_etc.parameter_check.methods.function.f(fn))
				throw Error("Callback must be a function.")

			return t.$hidden_mode({explicitAsync: true, callback: fn})
		}
	},
	dunder: {
		//Magic Name/Dunder shortcuts
		$str: '__str__',

		//===> Comparisons
		$lt: '__lt__',
		$lte: '__le__',
		$eq: '__eq__',
		$ne: '__ne__',
		$gt: '__gt__',
		$gte: '__ge__',
	
		$add: '__add__',
		$sub: '__sub__',
		$mul: '__mul__',
		$div: '__div__',
	
		//===> General Properties
		$length: '__len__'
	},
	callable: {
		$apply: (t) => t.$apply
	},
	class: {
		$apply: (t) => t.$apply
	}
}

_local.marshaled_object_tag = Symbol('marshaled obj')
_etc.get_raw_object = (obj) => {
	return obj[_local.marshaled_object_tag]
}
_etc.unmarshalling_filter = (obj) => {
	let lmo = obj[_local.marshaled_object_tag]
	if (lmo !== undefined)
		return lmo
	return undefined
}

_etc.serializaition_filter = () => {
	let _sequence_map = new Map()

	return {
		SequenceTest(obj) {
			return _sequence_map.get(obj);
		},
		SequenceRegister(o, p) {
			_sequence_map.set(o, p)
			return undefined
		},
		Examine() {
			return _sequence_map
		},
		Finalize() {
			_sequence_map.clear()
		}
	}
}

_local.dunder_regex = /^__.+__$/g
_local.marshalling_helper = (obj) => {
	if (obj === undefined || obj === null)
		return _local._pyjs.$GetMarshaledObject(obj)

	let lmo = obj[_local.marshaled_object_tag]
	if (lmo !== undefined)
		return lmo

	if (_etc.parameter_check.methods.function.f(obj)) {
		return _local._pyjs.$GetMarshaledObject(function(args) {
			//return _local.marshalling_helper(obj(args))
			return obj(args) //return raw napi values
		})
	}

	if (_local._pyjs.$IsInstanceOf(obj))
		return obj
	
	return _local._pyjs.$GetMarshaledObject(obj)
}
_local.marshalling_option_helper = ({getReference}) => {
	return { 
		getReference
	}
}
_local.default_marshalling_modes = {
	attributeCheck: true,
	asyncOverride: false,
	getReference: false,
	getReferenceOnIterate: false
}

_local.default_hidden_marshalling_modes = {
	explicitAsync: false
}

////////////////////////////////////////////
// Marshalling Proxy & Factory
////////////////////////////////////////////

_etc.marshalling_factory_cloner = (obj, {_mode, _hidden_mode}) => {
	let lmo = obj[_local.marshaled_object_tag]
	if (lmo === undefined)
		throw Error('Internal Error.')
	
	let _proxy = _etc.marshalling_factory(lmo, 
		{ _mode, _hidden_mode })
	return _proxy
}

_etc.marshalling_factory = (obj, modes) =>　{

	//let lmo = obj[_local.marshaled_object_tag]
	//if (lmo !== undefined)
	//	return lmo;
	if (!_local._pyjs.$IsInstanceOf(obj))
		return obj

	let func = () => undefined
	func._current_call = {} //cache data for invocations/accessors
	func.construct = () => undefined
	func.py = obj //NapiPyObject instance
	func.$_function_prototype = (t) =>
	{
		func._current_call.has_function = false

		return (args, dict) => {
			if (_etc.parameter_check.methods.object.f(args) 
				&& !_etc.parameter_check.methods.array.f(args)
				&& dict == undefined) {
				dict = args
				args = []
			}

			if (!_etc.parameter_check.methods.array.f(args))
				throw Error("Function invocation requires an array.")

			let pyargs = []
			for (let arg of args)
			{
				arg = _local.marshalling_helper(arg)
				if (arg.GetObjectType() == _etc.python_object_type.FUNCTION)
					func._current_call.has_function = true

				pyargs.push(arg)
			}

			let pyargs2 = undefined
			if (dict !== undefined)
			{
				if (!_etc.parameter_check.methods.object.f(dict))
					throw Error("Not a list of key-value pairs (Javascript Object).")

				for (let key in dict)
				{
					let arg = _local.marshalling_helper(dict[key])
					if (arg.GetObjectType() == _etc.python_object_type.FUNCTION)
						func._current_call.has_function = true

					dict[key] = arg
				}

				pyargs2 = dict
			}

			let f_target = (p,p2) => {
				if (func._current_call.has_function && !func._mode.asyncOverride) {
					if (!func._hidden_mode.explicitAsync)
						throw Error("Function invocation includes a function as a parameter.\n" 
							+ "Please explicitly invoke as '$async' to evaluate this expression as a callback.")

					if (func._hidden_mode.callback !== undefined)
					{
						return t.py.FunctionCallAsync(p, p2,
							(res) => func._hidden_mode.callback(
								_etc.marshalling_factory(res)))
					}
					else
						return t.py.FunctionCallAsync(p, p2)
				}
				else {
					return t.py.FunctionCall(p, p2,
						_local.marshalling_option_helper(func._mode))
				}
			}

			if (t.py.IsCallable())
				return _etc.marshalling_factory(f_target(pyargs, pyargs2))
			else if (t.py.GetObjectType() == _etc.python_object_type.TYPE)
				return _etc.marshalling_factory(f_target(pyargs, pyargs2))
			else
				return undefined
		}
	}
	//func.$call = func.$function_prototype(func).call
	func.$apply = func.$_function_prototype(func)
	func.$isCallable = () => func.py.IsCallable()
	func.$isClass = () => func.py.GetObjectType() 
		== _etc.python_object_type.TYPE
	func.$isIterable = () => func.py.GetAttributeList().includes('__iter__')
	func.$_get_special_attributes = (t) => {
		let m = new Map(Object.entries(_local.object_attributes.all))
		let o_type = t.py.GetObjectType()
		let is_class = o_type == _etc.python_object_type.TYPE
		let attrs = t.py.GetAttributeList()

		if (t.py.IsCallable())
			for (const attr of Object.entries(_local.object_attributes.callable))
				m.set(attr[0], attr[1])
		
		if (is_class)
			for (const attr of Object.entries(_local.object_attributes.class))
				m.set(attr[0], attr[1])

			for (const attr of Object.entries(_local.object_attributes.dunder))
			{
				if (attrs.includes(attr[1]))
					m.set(attr[0], (t) => func._p[attr[1]])
			}

		return m
	}
	func.$getType = () => _etc.marshalling_factory(
		func.py.GetPythonTypeObject())

	if (modes !== undefined)
	{
		func._mode = Object.assign({}, modes._mode)
		func._hidden_mode = Object.assign({}, modes._hidden_mode)
	}
	else
	{
		func._mode = Object.assign({}, _local.default_marshalling_modes)
		func._hidden_mode = Object.assign({}, _local.default_hidden_marshalling_modes)
	}
	
	func.$mode = ({ attributeCheck = func._mode.attributeCheck, asyncOverride = func._mode.asyncOverride,
		getReference = func._mode.getReference, getReferenceOnIterate = func._mode.getReferenceOnIterate } = {}) => {
			func._mode.attributeCheck = attributeCheck
			func._mode.asyncOverride = asyncOverride
			func._mode.getReference = getReference
			func._mode.getReferenceOnIterate = getReferenceOnIterate
			return func._p
	}
	func.$hidden_mode = ({ explicitAsync = func._hidden_mode.explicitAsync, callback = undefined } = {}) => {
		func._hidden_mode.explicitAsync = explicitAsync
		func._hidden_mode.callback = callback
		return func._p
	}
	func.$read_only_hidden_mode = ({explicitAsync, callback}) => {
		return { explicitAsync,  callback }
	}
	func.$newMode = (modes = {}) => {
		let np = _etc.marshalling_factory_cloner(func._p, 
			{
				_mode: func._mode,
				_hidden_mode: func._hidden_mode
			})
		return np.$mode(modes)
	}
	//Display current modes and hidden (non-internal use) modes we want the user to see.
	func.$getMode = () => Object.assign(Object.assign({}, func._mode), func.$read_only_hidden_mode(func._hidden_mode))

	const handler = {
		apply: (t, th, args) => t.$apply(args),
		construct: (t, a) => { },
		get: (t, k) => {
			if (k === _local.marshaled_object_tag)
				return t.py
			// node.js console.log output
			else if (k === util.inspect.custom)
			{
				let o_type = t.py.GetObjectType()
				let name = t.py.GetAttribute('__name__', null)
				let c = t.py.GetAttribute('__class__', null)
				if (c !== undefined) c = c.GetAttribute('__name__', null)

				if (c === undefined && o_type == _etc.python_object_type.OBJECT) {
					let type = t.py.GetPythonTypeObject()

					if (name === undefined) {
						name = type.GetAttribute('__name__', null)
					}

					c = type.GetAttribute('__class__', null)
					if (c !== undefined) c = c.GetAttribute('__name__', null)
					if (c === '__class__') c = "object"
				}
					
				let is_callable = t.py.IsCallable()
				let is_class = o_type == _etc.python_object_type.TYPE
				let attr = t.py.GetAttributeList()
				let dunder = _local.dunder_regex
				let desc = is_class ? 'Class' : 'Object'

				return (depth, options) => {
					//get name & class name from python
					name = name === undefined ? ' ' : ` #${name} `
					c = c === undefined ? '' : `<class ${options.stylize("'" + c + "'", 'string')}> `

					let attrs = []
					if (is_callable)
						attrs.push(`${options.stylize("[callable]", 'special')}`)
					if (is_class)
						attrs.push(`${options.stylize("[instantiates]", 'special')}`)
					for (const [at,fn] of func.$_get_special_attributes(t)) {
						attrs.push(`${options.stylize("'" + at + "'", 'special')}`)
					}
					for (const at of attr) {
						if (at.match(dunder))
							attrs.push(`${options.stylize("'" + at + "'", 'undefined')}`)
						else
							attrs.push(`'${at}'`)
					}

					let d = `${options.stylize('Python ' + desc, 'pyobject')}`
					let extra = `{ ${attrs.join(', ')} }`;
					if (options.__py_sub)
					{
						extra = ""
						d = ""
						if (name.length > 0) name = name.substring(1)
						if (c.length > 0) c = c.substring(0, c.length - 1)
					}

					return `${d}${options.stylize(name, 'special')}${c}${extra}`
				}
			}
			//for of ('values' or anything custom defined of obj)
			else if (k === Symbol.iterator) {
				if (t.py.GetAttributeList().includes('__iter__')) {
					return function*() {
						let next = func._p.__iter__().__next__
						if (func._mode.getReferenceOnIterate) {
							next = next.$newMode({getReference: true})
						}
						let pe = _local._pyjs.pyjs.exceptions().PythonException
						let hasItems = true
						while (hasItems) {
							try {
								yield _etc.marshalling_factory(next())
							}
							catch (err) {
								if (err instanceof pe
									&& err.IsStopIterationException()) {
									hasItems = false
								}
								else {
									throw err
								}
							}
						}
					}
				}
				else {
					return function*() {}
				}
			}
			else if (func.$_get_special_attributes(t).has(k)) {
				return func.$_get_special_attributes(t).get(k)(t)
			}
			else if (_etc.parameter_check.methods.string.f(k)) {
				if (func._mode.attributeCheck) {
					if (t.py.GetAttributeList().includes(k)) {
						return _etc.marshalling_factory(
							t.py.GetAttribute(k, 
								_local.marshalling_option_helper(func._mode)))
					}
				}
				else
					return _etc.marshalling_factory(t.py.GetAttribute(k, 
						_local.marshalling_option_helper(func._mode)))
			}

			return undefined
		},
		set: (o, attr, val) => {
			//Will throw if an error occurs.
			let obj = _local.marshalling_helper(val)
			func.py.SetAttribute(attr, obj)
			return true
		},
		has: (t,k) => {
			if (func.$_get_special_attributes(t).has(k))
				return true
			if (t.py.GetAttributeList().includes(k))
				return true
			return false
		},
		// for in (properties of obj)
		ownKeys: (t) => {
			let props = [...func.$_get_special_attributes(t).keys(), 
				...t.py.GetAttributeList()]

			return props
		},
		getOwnPropertyDescriptor: (p) => {
			return {
				enumerable: true,
				configurable: true
			}
		}
	}

	let p = new Proxy(func, handler)
	func._p = p

	return p
}

////////////////////////////////////////////
// Basic Helpers
////////////////////////////////////////////

_etc.parameter_check = (fn, t, p) =>　{
	for (let i in p)
		if (!t[i] || !t[i].f(p[i]))
			throw Error(`Function '${fn}' requires a signature of (${t.map(x => x.n).join(',')}).`)
}

_etc.parameter_check.methods = {
	string: {
		n: 'string',
		f: (v) => {
			return typeof v === 'string' || v instanceof String
		}
	},
	number: {
		n: 'number',
		f: (v) => {
			return typeof v === 'number' && isFinite(v)
		}
	},
	null: {
		n: 'null',
		f: (v) => {
			return v === null
		}
	},
	undefined: {
		n: 'undefined',
		f: (v) => {
			return typeof v === 'undefined'
		}
	},
	function: {
		n: 'function',
		f: (v) => {
			return typeof v === 'function'
		}
	},
	object: {
		n: 'object',
		f: (v) => {
			return typeof v === 'object' && v !== null
		}
	},
	array: {
		n: 'array',
		f: (v) => {
			return Array.isArray(v)
		}
	}
}

////////////////////////////////////////////
// Export
////////////////////////////////////////////

module.exports = _etc