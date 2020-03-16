#include "../include/marlin/ABCInterface.hpp"
#include <uv.h>

int main() {
	marlin::ABCInterface abcIface;
	return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
