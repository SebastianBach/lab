// Mon May 23 17:37:55 2022

#include<vector>
#include<iostream>

int main() {

	std::vector<float> data;
	auto res = .0f;

	// hello_world
	std::cout << "Hello World !\n";

	// print_number
	// num : 42
	std::cout<<"Number: "<<42.000000<<"\n";

	// set_values
	// cnt : 0
	data.clear();

	{
		// set_values
		// cnt : 10
		const auto cnt = 10;
		data.resize(cnt);
		for (auto i = 0; i < cnt; ++i) {data[i] = static_cast<float>(i);}
	}

	{
		// check_data
		const auto populated = !data.empty();
		if (!populated) {
			// cleanup
			data.clear();
			data.shrink_to_fit();
			return 0;
		}
	}

	// sum
	res = 0.0f;
	for (const auto&v:data) {res += v;}

	// print
	std::cout << "Result: " << res <<"\n";

	{
		// check
		// ref : 45
		const auto expected_value = 45.000000f;
		const auto res_ok = expected_value == res;
		if (!res_ok) {
			// cleanup
			data.clear();
			data.shrink_to_fit();
			return 0;
		}
	}

	// reset
	data.clear();
	res = 0.0f;

	{
		// set_values
		// cnt : 20
		const auto cnt = 20;
		data.resize(cnt);
		for (auto i = 0; i < cnt; ++i) {data[i] = static_cast<float>(i);}
	}

	{
		// check_data
		const auto populated = !data.empty();
		if (!populated) {
			// cleanup
			data.clear();
			data.shrink_to_fit();
			return 0;
		}
	}

	// print_data
	std::cout << "Data :\n";
	for (const auto& v : data)
		std::cout << v << "\n";

	// product
	res = 1.0f;
	for (const auto&v:data) {res *= v;}

	// print
	std::cout << "Result: " << res <<"\n";

	// cleanup
	data.clear();
	data.shrink_to_fit();

	return 0;
}
