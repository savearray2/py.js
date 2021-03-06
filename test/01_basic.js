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

const path = require('path');
const p = require('..')
const assert = require('chai').assert

let map_to_object = o => {
	let obj = {}
	o.forEach((v,k) => {
		if (v instanceof Map)
			obj[k] = map_to_object(v)
		else
			obj[k] = v
	})
	return obj
}

describe('pyjs: basic functions', function() {
	describe('[pyjs] #import("01_basic")', function() {
		it('should not throw', function() {
			assert.doesNotThrow(() => p.import('01_basic'))
		})
	})

	describe('[pyjs] #base()', function() {
		it('should not throw', function() {
			assert.doesNotThrow(p.base)
		})

		it("base includes <class 'type'>", function() {
			assert.exists(p.base().type)
		})

		it("base includes 'str' function", function() {
			assert.exists(p.base().str)
		})

		it("base <class 'dict'> includes string 'dict' from 'str' function", function() {
			assert.include(p.base().str(p.base().dict), 'dict')
		})
	})

	describe('[pyjs] $coerceAs', function() {
		it('should exist', function() {
			assert.exists(p.$coerceAs)
		})

		it('#Integer(), #int() should be functions', function() {
			assert.isFunction(p.$coerceAs.Integer)
			assert.isFunction(p.$coerceAs.int)
		})

		it("#Integer(4), #int(4) should return a Python int object, and equal 4", function() {
			assert.isFalse(p.$coerceAs.int(4).$isCallable())
			assert.isFalse(p.$coerceAs.Integer(4).$isCallable())
			assert.include(p.base().str(p.base().type(p.$coerceAs.int(4))), 'int')
			assert.include(p.base().str(p.base().type(p.$coerceAs.Integer(4))), 'int')
			assert.strictEqual(p.base().str(p.$coerceAs.int(4)), '4')
			assert.strictEqual(p.base().str(p.$coerceAs.Integer(4)), '4')
		})

		it("#Integer('10'), #int('10') should return a Python int object, and equal 10", function() {
			assert.isFalse(p.$coerceAs.int('10').$isCallable())
			assert.isFalse(p.$coerceAs.Integer('10').$isCallable())
			assert.include(p.base().str(p.base().type(p.$coerceAs.int('10'))), 'int')
			assert.include(p.base().str(p.base().type(p.$coerceAs.Integer('10'))), 'int')
			assert.strictEqual(p.base().str(p.$coerceAs.int('10')), '10')
			assert.strictEqual(p.base().str(p.$coerceAs.Integer('10')), '10')
		})

		it('#Tuple() should be a function', function() {
			assert.isFunction(p.$coerceAs.Tuple)
		})

		it("#Tuple([1,2,3]) should return a Python tuple object", function() {
			assert.include(p.base().str(p.base().type(p.$coerceAs.Tuple([1,2,3,4]))), 'tuple')
		})

		it("#Tuple([1,2,3]) should return a Python tuple object", function() {
			assert.include(p.base().str(p.base().type(p.$coerceAs.Tuple([1,2,3,4]))), 'tuple')
		})
	})

	describe('[py->js] none to null', function() {
		it('01_basic#basic_none() returns null', function() {
			assert.isNull(p.import('01_basic').basic_none())
		})
	})

	describe('[py->js] bool to bool', function() {
		it('01_basic#basic_bool() returns false', function() {
			assert.isFalse(p.import('01_basic').basic_bool())
		})
	})

	describe('[py->js] int to bigint', function() {
		it('01_basic#basic_int()returns a bigint', function() {
			assert.isTrue(typeof(p.import('01_basic').basic_int()) == 'bigint')
		})

		it('01_basic#basic_int() returns 1789n', function() {
			assert.strictEqual(p.import('01_basic').basic_int(), 1789n)
		})
	})

	describe('[py->js] float to float', function() {
		it('01_basic#basic_float()returns a float', function() {
			assert.isNumber(p.import('01_basic').basic_float())
		})

		it('01_basic#basic_float() returns 10234678.5', function() {
			assert.strictEqual(p.import('01_basic').basic_float(), 10234678.5)
		})
	})

	describe('[py->js] bytes to buffer', function() {
		it('01_basic#basic_bytes() returns a Buffer', function() {
			assert.instanceOf(p.import('01_basic').basic_bytes(), Buffer)
		})

		it('01_basic#basic_bytes() returns b"あいうえお" (unicode/utf-8 test)', function() {
			assert.strictEqual(p.import('01_basic').basic_bytes().toString('utf8'), "あいうえお")
		})
	})

	describe('[py->js] bytearray to buffer', function() {
		it('01_basic#basic_byte_array() returns a Buffer', function() {
			assert.instanceOf(p.import('01_basic').basic_byte_array(), Buffer)
		})

		it('01_basic#basic_byte_array() returns b"好的" (unicode/utf-8 test)', function() {
			assert.strictEqual(p.import('01_basic').basic_byte_array().toString('utf8'), "好的")
		})
	})

	describe('[py->js] string to string', function() {
		it('01_basic#basic_string() returns a string', function() {
			assert.isString(p.import('01_basic').basic_string())
		})

		it('01_basic#basic_string() returns "this is a string"', function() {
			assert.strictEqual(p.import('01_basic').basic_string(), "this is a string")
		})
	})

	describe('[py->js] tuple to array', function() {
		it('01_basic#basic_tuple() returns an array', function() {
			assert.isArray(p.import('01_basic').basic_tuple())
		})

		it('01_basic#basic_tuple() returns an array', function() {
			assert.deepStrictEqual(p.import('01_basic').basic_tuple(), [1n,2n,3n,4n])
		})
	})

	describe('[py->js] list to array', function() {
		it('01_basic#basic_list() returns an array', function() {
			assert.isArray(p.import('01_basic').basic_list())
		})

		it('01_basic#basic_list() returns "[null,123.4,Buffer.from("あいうえお"),Buffer.from("好的"),"list"]', function() {
			assert.deepStrictEqual(p.import('01_basic').basic_list(), [1n, null, 123.4, Buffer.from("あいうえお"), Buffer.from("好的"), "list"])
		})
	})

	describe('[py->js] dictionary to map', function() {
		//Our map to test
		let map = new Map
		map.set('a', 1.3)
		map.set('b', 2.1)
		map.set('c', 3.5)
		map.set(100.1, 's')
		map.set('好的', true)
		
		it('01_basic#basic_list() returns a map', function() {
			assert.instanceOf(p.import('01_basic').basic_dict(), Map)
		})

		it('01_basic#basic_list() returns "{\'a\': 1.3, \'b\': 2.1, \'c\': 3.5, 100.1: \'s\', 好的: true }"', function() {
			assert.deepStrictEqual(map, p.import('01_basic').basic_dict(), "marshalled map does not match expected return")
		})

		it('01_basic#basic_list() additional comparison test', function () {
			let dict = p.import('01_basic').basic_dict()
			for (let [a,z] of dict.entries()) {
				assert.isTrue(map.has(a))
				assert.deepStrictEqual(dict.get(a), map.get(a))
			}
		})
	})

	describe('[py->js] set to set', function() {
		it('01_basic#basic_set()', function () {
			let set = p.import('01_basic').basic_set()
			let arr = [1n,2n,3n,4n,'c',10.1]
			arr.forEach(n => assert.isTrue(set.has(n)))
		})
	})

	describe('[py->js] function to function', function() {
		it('01_basic#basic_function() returns a function (() => 1.1*2)', function() {
			assert.exists(p.import('01_basic').basic_function().$isCallable)
			assert.isTrue(p.import('01_basic').basic_function().$isCallable())
		})

		it('01_basic#basic_function()() returns 1.1*2', function() {
			assert.strictEqual(p.import('01_basic').basic_function()(), 2.2)
		})
	})

	describe('[py->js] class to object', function() {
		it('01_basic#basic_class() returns a class', function() {
			assert.exists(p.import('01_basic').basic_class().$isClass)
			assert.isTrue(p.import('01_basic').basic_class().$isClass())
		})

		it('01_basic#basic_class()(50.5) instantiates class', function() {
			assert.exists(p.import('01_basic').basic_class()(50.5).$isClass)
			assert.isFalse(p.import('01_basic').basic_class()(50.5).$isClass())
		})

		it('01_basic#basic_class()(50.5).test() calls instance method, returns 50.5', function() {
			assert.exists(p.import('01_basic').basic_class()(50.5).test)
			assert.isTrue(p.import('01_basic').basic_class()(50.5).test.$isCallable())
			assert.strictEqual(p.import('01_basic').basic_class()(50.5).test(), 50.5)
		})
	})

	describe('[py->js] exception to exception', function() {
		it('01_basic#basic_exception() throws a TypeError', function() {
			let threwError = false
			try {
				p.import('01_basic').basic_exception()
			}
			catch (err) {
				if (err instanceof p.exceptions().PythonException)
					if (err.py_name === 'TypeError')
						threwError = true
			}

			assert.isTrue(threwError)
		})
	})

	describe('[py->js] iterator to iterator', function() {
		it('01_basic#basic_iterator(100) returns an interable', function() {
			assert.isTrue(p.import('01_basic').basic_iterator(100).$isIterable())
		})

		it('01_basic#basic_iterator(100) iterates as expected', function() {
			let count = 1n
			for (let i of p.import('01_basic').basic_iterator(100)) {
				assert.isTrue(i.get('updated_count') === (count + 100n))
				assert.isTrue(i.get('count') === count)
				count++
			}
		})
	})

	describe('[js->py] null or undefined to none', function() {
		it('01_basic#basic_none_j(null) returns true', function() {
			assert.isTrue(p.import('01_basic').basic_none_j(null))
		})
		
		it('01_basic#basic_none_j(undefined) returns true', function() {
			assert.isTrue(p.import('01_basic').basic_none_j(undefined))
		})
	})

	describe('[js->py] bool to bool', function() {
		it('01_basic#basic_bool_j(true,false) returns true', function() {
			assert.isTrue(p.import('01_basic').basic_bool_j(true,false))
		})
		
		it('01_basic#basic_bool_j(true,false) returns true', function() {
			assert.isFalse(p.import('01_basic').basic_bool_j(false,true))
		})
	})

	describe('[js->py] bigint to int', function() {
		it('01_basic#basic_int_j(98359834579n) returns true', function() {
			assert.isTrue(p.import('01_basic').basic_int_j(98359834579n))
		})
	})

	describe('[js->py] float to float', function() {
		it('01_basic#basic_float_j(12830.8877) returns true', function() {
			assert.isTrue(p.import('01_basic').basic_float_j(12830.8877))
		})
	})

	describe('[js->py] string to string', function() {
		it('01_basic#basic_string_j("あいうえお") returns true', function() {
			assert.isTrue(p.import('01_basic').basic_string_j('あいうえお'))
		})
	})

	describe('[js->py] tuple (coerce) to tuple', function() {
		it('01_basic#basic_tuple_j([1.1,"a",99.9,True,1]) returns true', function() {
			assert.isTrue(p.import('01_basic').basic_tuple_j(p.$coerceAs.Tuple([1.1,'a',99.9,true,1n])))
		})
	})

	describe('[js->py] array to list', function() {
		it('01_basic#basic_list_j([1.1,"a",99.9,True]) returns true', function() {
			assert.isTrue(p.import('01_basic').basic_list_j([1.1,'a',99.9,true]))
		})
	})

	describe('[js->py] array to list', function() {
		it('01_basic#basic_list_j(b"abcdefg") returns true', function() {
			assert.isTrue(p.import('01_basic').basic_bytes_j(Buffer.from('abcdefg')))
		})
	})

	describe('[js->py] object to dictionary', function() {
		it('01_basic#basic_list_j({a: 1.3, b: 100.1,"あいうえお":{7.0: True}}) returns true', function() {
			assert.isTrue(p.import('01_basic').basic_dict_j(
				{
					a: 1.3, b: 100.1,
					"あいうえお": {
						'7': true
					}
				}
			))
		})
	})

	//This bugs out on 3.8.0 on Linux for unknown reasons...
	/*it('[js->py] date to datetime', function() {
		let date = new Date(2019, 0, 4, 12, 55, 11, 14)
		assert.isTrue(p.import('01_basic').basic_datetime_j(date))
	})*/
	/*
		PythonException [#ImportError]: ImportError: /home/travis/.pyenv/versions/3.8.0/lib/python3.8/lib-dynload/math.cpython-38-x86_64-linux-gnu.so: undefined symbol: PyFloat_Type
		ImportError: /home/travis/.pyenv/versions/3.8.0/lib/python3.8/lib-dynload/math.cpython-38-x86_64-linux-gnu.so: undefined symbol: PyFloat_Type
	*/

	describe('[js->py->js] echo testing', function() {
		it('01_basic#basic_echo_tester(null)', function() {
			assert.strictEqual(null, p.import('01_basic').basic_echo_tester(null))
		})

		it('01_basic#basic_echo_tester(undefined) -> null', function() {
			assert.strictEqual(null, p.import('01_basic').basic_echo_tester(undefined))
		})

		it('01_basic#basic_echo_tester(true)', function() {
			assert.strictEqual(true, p.import('01_basic').basic_echo_tester(true))
		})

		it('01_basic#basic_echo_tester(false)', function() {
			assert.strictEqual(false, p.import('01_basic').basic_echo_tester(false))
		})

		it('01_basic#basic_echo_tester(1)', function() {
			let big = ""
			for (let i = 0; i < 500; i++) big += "9"
			assert.strictEqual(big, p.import('01_basic').basic_echo_tester(big))
		})

		it('01_basic#basic_echo_tester(1.234)', function() {
			assert.strictEqual(1.234, p.import('01_basic').basic_echo_tester(1.234))
		})

		it('01_basic#basic_echo_tester("abcd")', function() {
			assert.strictEqual("abcd", p.import('01_basic').basic_echo_tester("abcd"))
		})

		it('01_basic#basic_echo_tester(coerce([9n,3n,"a",{d:true}]))', function() {
			let echo = p.import('01_basic').basic_echo_tester(
				p.$coerceAs.Tuple([9n,3n,'a',{d:true}]))
			echo[3] = map_to_object(echo[3])
			assert.deepStrictEqual([9n,3n,'a',{d:true}], echo)
		})

		it('01_basic#basic_echo_tester([1,2,3,"あ"])', function() {
			assert.deepStrictEqual([1,2,3,'あ',1500n], p.import('01_basic').basic_echo_tester([1,2,3,'あ',1500n]))
		})

		it('01_basic#basic_echo_tester(b"あいうえお")', function() {
			assert.isTrue(p.import('01_basic').basic_echo_tester(Buffer.from('あいうえお'))
				.equals(Buffer.from('あいうえお')))
		})

		it('01_basic#basic_echo_tester({a: 1, b: 2, c: {d: true}})', function() {
			let obj = {a: 1, b: 2, c: {d: true, e: 10000n}}
			let map = p.import('01_basic').basic_echo_tester(obj)
			assert.deepStrictEqual(obj, map_to_object(map))
		})
	})
})