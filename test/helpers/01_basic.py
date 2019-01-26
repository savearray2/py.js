def basic_none():
	return None

def basic_bool():
	return False

def basic_float():
	return 10234678.5

def basic_bytes():
	return str.encode('あいうえお')

def basic_byte_array():
	return bytearray('好的', 'utf-8')

def basic_string():
	return 'this is a string'

def basic_list():
	return [None,123.4,str.encode('あいうえお'),bytearray('好的', 'utf-8'),'list']

def basic_dict():
	return {'a': 1.3, 'b': 2.1, 'c': 3.5, 100.1: 's', '好的': True}

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
	