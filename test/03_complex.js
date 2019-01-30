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

const p = require('..')
const assert = require('chai').assert
const crypto = require('crypto')
const util = require('util')

getRandom = n => n === undefined ? 
	parseInt(crypto.randomBytes(4).toString('hex'), 16) :
	parseInt(crypto.randomBytes(4).toString('hex'), 16) % n

//Random Values
let bigint = () => {
	return BigInt(getRandom())
}
let str = () => {
	let ret = ''
	for (let i = 0; i < getRandom(30); i++)
		ret += Buffer.from([97 + getRandom(26)]).toString()
	return ret
}
let float = () => Math.random()
let nulls = () => null
let buffer = () => crypto.randomBytes(getRandom(10))

let vals = [
	bigint,
	str,
	float,
	nulls,
	buffer
]

let val_grabber = (xtra) => {
	if (xtra === undefined)
		return vals
	else
		return [...vals, ...xtra]
}

let array_randomizer = (min = 5, count = 25, v = val_grabber(), weight = 0) => {
	let testVals = []
	for (let i = 0; i < min + getRandom(count) - weight; i++)
		testVals.push(v[getRandom(v.length)](undefined,
			undefined, v, weight + 5))

	return testVals
}

let obj_randomizer = (min = 5, count = 25, v = val_grabber(), weight = 0) => {
	let testObj = {}
	for (let i = 0; i < min + getRandom(count) - weight; i++)
		testObj[str() + str()] = v[getRandom(v.length)](undefined,
			undefined, v, weight + 5)

	return testObj
}

map_to_object = o => {
	if (util.isArray(o)) {
		let arr = []
		o.forEach((v,k) => {
			if (v instanceof Map || util.isArray(v))
				arr.push(map_to_object(v))
			else
				arr.push(v)
		})
		return arr
	}
	else if (util.isObject(o)) {
		let obj = {}
		o.forEach((v,k) => {
			if (v instanceof Map || util.isArray(v))
				obj[k] = map_to_object(v)
			else
				obj[k] = v
		})
		return obj
	}
	else
		return o
}

let t = describe('pyjs: complexity testing', function() {
	describe('[rnd] flat random structure echo test', function() {
		it('01_basic#basic_echo_tester(), 10 iterations, random arrays', function() {
			this.slow(700)

			for (let i = 0; i < 10; i++)
			{
				let echo = p.import('01_basic').basic_echo_tester
				let arr = array_randomizer(10,20)
				assert.isTrue(util.isDeepStrictEqual(arr, echo(arr)))
			}
		})

		it('01_basic#basic_echo_tester(), 10 iterations, random objects', function() {
			this.slow(2500)

			for (let i = 0; i < 10; i++)
			{
				let echo = p.import('01_basic').basic_echo_tester
				let obj = obj_randomizer(10,20)
				let check = util.isDeepStrictEqual(obj, map_to_object(echo(obj)))
				assert.isTrue(check)
			}
		})
	})

	describe('[rnd] deep/complex random structure echo test', function() {
		it('01_basic#basic_echo_tester(), 10 iterations, random deep structures in array', function() {
			this.slow(2000)
			this.timeout(8000)

			for (let i = 0; i < 10; i++)
			{
				let echo = p.import('01_basic').basic_echo_tester
				let arr = array_randomizer(5,25, val_grabber([array_randomizer, obj_randomizer]))
				let ret =  map_to_object(echo(arr))

				assert.isTrue(util.isDeepStrictEqual(arr,ret))
			}
		})

		it('01_basic#basic_echo_tester(), 10 iterations, random deep structures in object', function() {
			this.slow(2000)
			this.timeout(8000)

			for (let i = 0; i < 10; i++)
			{
				let echo = p.import('01_basic').basic_echo_tester
				let obj = obj_randomizer(5,25, val_grabber([array_randomizer, obj_randomizer]))
				let ret =  map_to_object(echo(obj))

				assert.isTrue(util.isDeepStrictEqual(obj,ret))
			}
		})
	})
})