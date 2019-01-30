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

const path = require('path');
const p = require('..')
const assert = require('chai').assert

let t = describe('pyjs: proxy functions', function() {
	describe('[proxy] universal attribute check', function() {
		it('proxy#$toString exists', function() {
			assert.exists(p.$coerceAs.int(1).$toString)
		})

		it('proxy#$isCallable exists', function() {
			assert.exists(p.$coerceAs.int(1).$isCallable)
		})

		it('proxy#$isClass exists', function() {
			assert.exists(p.$coerceAs.int('1').$isClass)
		})

		it('proxy#$getType exists', function() {
			assert.exists(p.$coerceAs.int(1).$getType)
		})

		it('proxy#$newMode exists', function() {
			assert.exists(p.$coerceAs.int('1').$newMode)
		})

		it('proxy#$getMode exists', function() {
			assert.exists(p.$coerceAs.int(1).$getMode)
		})

		it('proxy#$mode exists', function() {
			assert.exists(p.$coerceAs.int('1').$mode)
		})

		it('proxy#$async exists', function() {
			assert.exists(p.$coerceAs.int(1).$async)
		})

		it('proxy#$toString() returns string', function() {
			assert.isString(p.$coerceAs.int(1).$toString())
		})

		it('proxy#$getType() does not throw', function() {
			assert.doesNotThrow(p.$coerceAs.int(1).$getType())
		})

		it('proxy#$isCallable() returns bool', function() {
			assert.isBoolean(p.$coerceAs.int(1).$isCallable())
		})

		it('proxy#$isClass() returns bool', function() {
			assert.isBoolean(p.$coerceAs.int('1').$isClass())
		})
	})

	describe('[proxy] mode defaults', function() {
		it('proxy#$getMode(attributeCheck) returns true', function() {
			assert.isTrue(p.$coerceAs.int(1)
				.$getMode().attributeCheck)
		})

		it('proxy#$getMode(asyncOverride) returns false', function() {
			assert.isFalse(p.$coerceAs.int('1')
				.$getMode().asyncOverride)
		})

		it('proxy#$getMode(getReference) returns false', function() {
			assert.isFalse(p.$coerceAs.int(1)
				.$getMode().getReference)
		})

		it('proxy#$getMode(getReferenceOnIterate) returns false', function() {
			assert.isFalse(p.$coerceAs.int(1)
				.$getMode().getReferenceOnIterate)
		})
	})

	describe('[proxy] mode setting', function() {
		it('proxy#$mode(attributeCheck->false)', function() {
			let c = p.$coerceAs.int(1).$mode({attributeCheck: false})
			assert.isFalse(c.$getMode().attributeCheck)
		})

		it('proxy#$mode(asyncOverride->true)', function() {
			let c = p.$coerceAs.int('1').$mode({asyncOverride: true})
			assert.isTrue(c.$getMode().asyncOverride)
		})

		it('proxy#$mode(getReference->true)', function() {
			let c = p.$coerceAs.int(1).$mode({getReference: true})
			assert.isTrue(c.$getMode().getReference)
		})

		it('proxy#$mode(getReferenceOnIterate->true)', function() {
			let c = p.$coerceAs.int(1).$mode({getReferenceOnIterate: true})
			assert.isTrue(c.$getMode().getReferenceOnIterate)
		})

		it('proxy#$mode({}) returns proxy', function() {
			let proxy = p.$coerceAs.int(1)
			assert.isTrue(proxy === proxy.$mode())
		})

		it('proxy#newMode({}) returns new proxy', function() {
			let proxy = p.$coerceAs.int(1)
			assert.isTrue(proxy !== proxy.$newMode())
		})

		it('proxy#newMode({getReference->true}).getReference !== proxy.getReference', function() {
			let proxy = p.$coerceAs.int(1)
			assert.isTrue(proxy.$getMode().getReference !== proxy.$newMode({getReference: true}).$getMode().getReference)
		})
	})
})