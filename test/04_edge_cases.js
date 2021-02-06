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

const p = require('..')
const assert = require('chai').assert
const crypto = require('crypto')
const util = require('util')

let rand_int = (min,max) => Math.floor(Math.random() * (max - min + 1) + min)

let map_to_object_with_circular_caching = (o, seen_map = new Map(), arr = [], obj = {}) => {
	if (util.isArray(o)) {
		o.forEach((v,k) => {
			if (seen_map.has(v))
				arr.push(seen_map.get(v))
			else if (v instanceof Map) {
				let newObj = {}
				seen_map.set(v, newObj)
				arr.push(map_to_object_with_circular_caching(
					v, seen_map, null, newObj))
			}
			else if (util.isArray(v)) {
				let newArr = []
				seen_map.set(v, newArr)
				arr.push(map_to_object_with_circular_caching(
					v, seen_map, newArr, null))
			}
			else
				arr.push(v)
		})
		return arr
	}
	else if (util.isObject(o)) {
		o.forEach((v,k) => {
			if (seen_map.has(v))
				obj[k] = seen_map.get(v)
			else if (v instanceof Map) {
				let newObj = {}
				seen_map.set(v, newObj)
				obj[k] = map_to_object_with_circular_caching(
					v, seen_map, null, newObj)
			}
			else if (util.isArray(v)) {
				let newArr = []
				seen_map.set(v, newArr)
				obj[k] = map_to_object_with_circular_caching(
					v, seen_map, newArr, null)
			}
			else
				obj[k] = v
		})
		return obj
	}
	else
		return o
}

let t = describe('pyjs: edge cases', function() {
	describe('[py->js] dict types w/ tuples as keys', function() {
		it('04_edge#edge_dict_tuple() returns map {[1n,2n,3n] => "a"}', function() {
			let edge = p.import('04_edge')
			let vals = [...edge.edge_dict_tuple()][0]
			assert.deepStrictEqual(vals[0],[1n,2n,3n])
			assert.deepStrictEqual(vals[1], 'a')
		})
	})

	describe('[js->py->js] circular references', function() {
		it('04_edge#edge_echo() circular reference test #1', function() {
			let edge = p.import('04_edge')
			let obj = {
				a: 1n,
				b: 2n
			}
			obj.f = obj
			let arr = [ 1n, 2, 5.4, obj, 'a', Buffer.from('abc'), obj ]
			let e = edge.edge_echo(arr)
			let echo = map_to_object_with_circular_caching(e)
			assert.deepStrictEqual(arr, echo)
		})

		it('04_edge#edge_echo() circular reference test #2', function() {
			let edge = p.import('04_edge')
			let obj = {
				a: 1n,
				b: 2n
			}
			obj.f = obj
			let o = { x: 1n, y: 2, h: 5.4, tz: obj, m: 'a', d: Buffer.from('abc'), b: obj }
			let e = edge.edge_echo(o)
			let echo = map_to_object_with_circular_caching(e)
			assert.deepStrictEqual(o, echo)
		})

		it('04_edge#edge_echo() circular reference test #3', function() {
			let edge = p.import('04_edge')
			let obj = {
				a: 1n,
				b: 2n
			}
			obj.f = obj
			let o = { x: 1n, y: 2, h: 5.4, tz: obj, m: 'a', d: Buffer.from('abc'), b: obj }
			o = {a: 777777n, b: o, z: obj }
			let e = edge.edge_echo(o)
			let echo = map_to_object_with_circular_caching(e)
			assert.deepStrictEqual(o, echo)
		})

		it('04_edge#edge_echo() circular reference test #4', function() {
			let edge = p.import('04_edge')
			let obj = {
				a: 1n,
				b: 2n
			}
			obj.f = obj
			let o = { x: 1n, y: 2, h: 5.4, tz: obj, m: 'a', d: Buffer.from('abc'), b: obj }
			o = [5n, o, '6']
			let e = edge.edge_echo(o)
			let echo = map_to_object_with_circular_caching(e)
			assert.deepStrictEqual(o, echo)
		})
	})
})