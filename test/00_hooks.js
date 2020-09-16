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

before(function() {
	assert.doesNotThrow(() => p.init({
		pythonPath: 
			`${path.join(process.cwd(), 'test', 'helpers')}`
	}))
})

after(function () {
	assert.doesNotThrow(() => p.finalize())
	// force the process to close so that p
	// isn't touched after finalize
	// otherwise a seg fault happens
	// (but only when using npm run test,
	// not directly...)
	console.log()
	process.exit(0)
})