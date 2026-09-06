/*******************************************************************************
 * Copyright (C) 2026 Commissariat a l'energie atomique et aux energies alternatives (CEA)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * * Neither the name of CEA nor the names of its contributors may be used to
 *   endorse or promote products derived from this software without specific
 *   prior written permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 ******************************************************************************/

#include <stdio.h>
#include <unistd.h>

#include <concepts>
#include <random>
#include <vector>

template <std::integral T, typename Engine>
std::uniform_int_distribution<T> random_init(Engine& engine)
{
	return std::uniform_int_distribution<T>(std::numeric_limits<T>::min());
}

template <std::floating_point T, typename Engine>
std::normal_distribution<T> random_init(Engine& engine)
{
	return std::normal_distribution<T>();
}

template < typename T, typename Engine>
std::vector<T> random_vector(const int& size, Engine& engine)
{
	std::vector<T> result(size);
	auto dist = random_init<T, Engine>(engine);
	for (auto&& elem: result) {
		elem = dist(engine);
	}
	return result;
}

/// return the type as a string
template <typename T>
std::string type_name()
{
	if constexpr (std::is_same_v<T, int>)
		return "int";
	else if constexpr (std::is_same_v<T, long>)
		return "long";
	else if constexpr (std::is_same_v<T, float>)
		return "float";
	else if constexpr (std::is_same_v<T, double>)
		return "double";
	else if constexpr (std::is_same_v<T, short>)
		return "short";
	// else if constexpr (std::is_same_v<T, bool>)
	//     return "bool";
	else
		throw std::runtime_error("This type is not supported");
}

/**
 * replace all occurrence of a given substring inside a string 
 * 
 * \param str string that will be changed
 * \param old_substring the substring that will be replace
 * \param new_subtring the new value of the old_substring
 */

std::string replace_all(std::string str, const std::string& old_substring, const std::string& new_substring)
{
	size_t start_pos = 0;
	while ((start_pos = str.find(old_substring, start_pos)) != std::string::npos) {
		str.replace(start_pos, old_substring.length(), new_substring);
		start_pos += new_substring.length(); // Handles case where 'new_substring' is a substring of 'old_substring'
	}
	return str;
}
