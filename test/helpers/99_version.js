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

//Print version information for build tests
console.log('---------------------------------------------')
console.log(process.env)
console.log('---------------------------------------------')
console.log({cwd:`${process.cwd()}`})
console.log({__dirname:`${__dirname}`})
const p = require('../../')
console.log({'python_version': `${p.init().instance().python_version}`})
console.log('---------------------------------------------')
p.finalize()