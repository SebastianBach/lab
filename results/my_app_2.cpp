#include "my_header.h"

int main() {

	std::vector<float> data;
	auto res = .0f;

	hello_world(data, res);

	print_number_1(data, res);

	set_values_2(data, res);

	set_values_3(data, res);

	if (!check_data(data, res)) {
		_cleanup(data, res);
		return 0;
	}

	sum(data, res);

	print(data, res);

	if (!check_7(data, res)) {
		_cleanup(data, res);
		return 0;
	}

	reset(data, res);

	set_values_9(data, res);

	if (!check_data(data, res)) {
		_cleanup(data, res);
		return 0;
	}

	print_data(data, res);

	product(data, res);

	print(data, res);

	_cleanup(data, res);

	return 0;
}
