//////////////////////////////////////////////////////////////////////////
//	py.js - Node.js/Python Bridge; Node.js-hosted Python.
//	Copyright (C) 2019  Michael Brown
//
//	This program is free software: you can redistribute it and/or modify
//	it under the terms of the GNU Affero General Public License as
//	published by the Free Software Foundation, either version 3 of the
//  License, or (at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU Affero General Public License for more details.
//
//	You should have received a copy of the GNU Affero General Public License
//	along with this program.  If not, see <https://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////////

#include "pyjs_.h"
#include "napi_callback.hpp"

////////////////////////////////////////////
// Debug?
////////////////////////////////////////////

std::string pyjs_utils::GetCurrentThreadID()
{
	std::thread::id this_id = std::this_thread::get_id();
	std::stringstream ss;
	ss << this_id;
	return ss.str();
}

unsigned long pyjs_utils::GetCurrentTimeTicks()
{
	std::chrono::milliseconds ms =
		std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch());
	
	return ms.count();
}

/*void py_debug::print(const std::string &msg)
{
	std::time_t t = std::time(nullptr);
	std::cout << std::put_time(std::localtime(&t), "[%H:%M:%S] ") << msg << std::endl;
}*/
