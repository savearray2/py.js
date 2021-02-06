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

let t = describe('pyjs: async protocol & marshalling node.js functions', function() {
	describe('[js->py->js] async function call (calling js function from python)', function() {
		it('05_async#async_function_echo_tester(() => 100n) returns 100n async', function(done) {
			this.timeout(5000)
			this.slow(4000)

			let async = p.import('05_async')
			let remote_function = async.async_function_echo_tester.$async((ret_val) => {
				assert.strictEqual(100n, ret_val)
				done()
			})

			remote_function(() => 100n)
		})
	})
})