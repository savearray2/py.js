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

# Py->JS Tests

def basic_none():
	return None

def basic_bool():
	return False

def basic_int():
	return 1789

def basic_float():
	return 10234678.5

def basic_bytes():
	return str.encode('あいうえお')

def basic_byte_array():
	return bytearray('好的', 'utf-8')

def basic_string():
	return 'this is a string'

def basic_tuple():
	return (1,2,3,4)

def basic_list():
	return [1,None,123.4,str.encode('あいうえお'),bytearray('好的', 'utf-8'),'list']

def basic_dict():
	return {'a': 1.3, 'b': 2.1, 'c': 3.5, 100.1: 's', '好的': True}

def basic_set():
	return set([1,2,3,4,'c',10.1])

def basic_function():
	def bfun():
		return 1.1*2
	return bfun

def basic_class():
	class bclass():
		def __init__(self, i):
			self.i = i
		def test(self):
			return self.i
	return bclass

def basic_exception():
	raise TypeError('basic exception')

def basic_iterator(c):
	class TestIterable:
		def __init__(self, count):
			self.count = count
		def __iter__(self):
			self.counter = 0
			return self
		def __next__(self):
			if self.counter >= self.count:
				raise StopIteration
			else:
				self.counter += 1
				return { 'updated_count': self.counter + 100, 'count': self.counter }

	return TestIterable(c)

#//////////////////////////////////////////////////////////////////////////

# JS->Py Tests

def basic_none_j(o):
	return o == None

def basic_bool_j(a,b):
	return a == True and b == False

def basic_int_j(a):
	return a == 98359834579

def basic_float_j(a):
	return a == 12830.8877

def basic_string_j(a):
	return 'あいうえお' == a

def basic_tuple_j(a):
	return (1.1,"a",99.9,True,1) == a

def basic_list_j(a):
	return [1.1,'a',99.9,True] == a

def basic_bytes_j(a):
	return b'abcdefg' == a

def basic_dict_j(a):
	return {'a': 1.3, 'b': 100.1, 'あいうえお': { '7':True } } == a

def basic_datetime_j(a):
	return a.hour == 12 and a.minute == 55 and a.second == 11 and a.month == 1 and a.year == 2019 and a.day == 4

#//////////////////////////////////////////////////////////////////////////

def basic_echo_tester(a):
	return a