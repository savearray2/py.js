#//////////////////////////////////////////////////////////////////////////
#//	py.js - Node.js/Python Bridge; Node.js-hosted Python.
#//	Copyright (C) 2019  Michael Brown
#//
#//	This program is free software: you can redistribute it and/or modify
#//	it under the terms of the GNU Affero General Public License as
#//	published by the Free Software Foundation, either version 3 of the
#//	License, or (at your option) any later version.
#//
#//	This program is distributed in the hope that it will be useful,
#//	but WITHOUT ANY WARRANTY; without even the implied warranty of
#//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#//	GNU Affero General Public License for more details.
#//
#//	You should have received a copy of the GNU Affero General Public License
#//	along with this program.  If not, see <https://www.gnu.org/licenses/>.
#//
#//	Additional permission under the GNU Affero GPL version 3 section 7:
#//
#//	If you modify this Program, or any covered work, by linking or
#//	combining it with other code, such other code is not for that reason
#//	alone subject to any of the requirements of the GNU Affero GPL
#//	version 3.
#//////////////////////////////////////////////////////////////////////////

def async_function_echo_tester(fun):
	return fun()