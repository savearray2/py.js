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
#//////////////////////////////////////////////////////////////////////////

###################################
# Node/Python Capsule Bridge Setup
###################################
global _cb
global _cb_destroy

def _cb(pointer_data, args, kwargs):
	__pyjs = __import__('__pyjs')
	params = [list(args), kwargs, pointer_data, __pyjs._get_current_thread_id()]
	return __pyjs._callback(pointer_data, params)

def _cb_destroy(pointer_data):
	__pyjs = __import__('__pyjs')
	__pyjs._callback_destroy(pointer_data)

class _callback_factory(tuple):
	def __new__(cls, _pointer_data):
		__pyjs = __import__('__pyjs')
		ptr = (_pointer_data,_cb,_cb_destroy)
		__pyjs._log('creating callback ptr object: ' + str(ptr))
		return tuple.__new__(cls, ptr)
	def __call__(self, *args, **kwargs):
		return self[1](self[0], args, kwargs)
	def __del__(self):
		#print('DELETE GC!!! => ' + str(self._pointer_data))
		self[2](self[0])
	def _pointer_data(self):
		return self[0]
	def __setattr__(self, *ignored):
		raise TypeError
	def __delattr__(self, *ignored):
		raise TypeError

import sys

__pyjs = __import__('__pyjs')

def log_on(msg):
	__pyjs = __import__('__pyjs')
	__pyjs._debug(msg)

def log_off(msg):
	pass

if __pyjs._debug_enabled():
	__pyjs._log = log_on
else:
	__pyjs._log = log_off

__pyjs._callback_factory = _callback_factory
__pyjs._exit = sys.exit

__pyjs._log("py.js python intialization script loaded.")
