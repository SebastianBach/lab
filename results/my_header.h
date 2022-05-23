#pragma once
#include<vector>
#include<iostream>

inline void hello_world(std::vector<float>&data, float&res)
{
	std::cout << "Hello World !\n";
}

inline void print_number_1(std::vector<float>&data, float&res)
{
	// num : 42
	std::cout<<"Number: "<<42.000000<<"\n";
}

inline void set_values_2(std::vector<float>&data, float&res)
{
	// cnt : 0
	data.clear();
}

inline void set_values_3(std::vector<float>&data, float&res)
{
	// cnt : 10
	const auto cnt = 10;
	data.resize(cnt);
	for (auto i = 0; i < cnt; ++i) {data[i] = static_cast<float>(i);}
}

inline auto check_data(std::vector<float>&data, float&res)
{
	const auto populated = !data.empty();
	return populated;
}

inline void sum(std::vector<float>&data, float&res)
{
	res = 0.0f;
	for (const auto&v:data) {res += v;}
}

inline void print(std::vector<float>&data, float&res)
{
	std::cout << "Result: " << res <<"\n";
}

inline auto check_7(std::vector<float>&data, float&res)
{
	// ref : 45
	const auto expected_value = 45.000000f;
	const auto res_ok = expected_value == res;
	return res_ok;
}

inline void reset(std::vector<float>&data, float&res)
{
	data.clear();
	res = 0.0f;
}

inline void set_values_9(std::vector<float>&data, float&res)
{
	// cnt : 20
	const auto cnt = 20;
	data.resize(cnt);
	for (auto i = 0; i < cnt; ++i) {data[i] = static_cast<float>(i);}
}

inline void print_data(std::vector<float>&data, float&res)
{
	std::cout << "Data :\n";
	for (const auto& v : data)
		std::cout << v << "\n";
}

inline void product(std::vector<float>&data, float&res)
{
	res = 1.0f;
	for (const auto&v:data) {res *= v;}
}

inline void _cleanup(std::vector<float>&data, float&res)
{
	data.clear();
	data.shrink_to_fit();
}

