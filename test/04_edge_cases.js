const p = require('..')
const assert = require('chai').assert
const crypto = require('crypto')
const util = require('util')

let rand_int = (min,max) => Math.floor(Math.random() * (max - min + 1) + min)

map_to_object_with_circular_caching = (o, seen_map = new Map(), arr = [], obj = {}) => {
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