/*
 * novocaine - hooking (redirection) library for Windows x86
 */

#include "../novocaine/novocaine.hpp"
#include <iostream>

__declspec(noinline) int add(int a, int b)
{
	return a + b;
}

__declspec(noinline) int sub(int a, int b)
{
	return a - b;
}

int main()
{
	novocaine::redirect func_redirect(add, sub);

	std::cout << "Result of function before redirect (6): " << add(1, 5) << std::endl;
	func_redirect.transact();
	std::cout << "Result of function after redirect (-4): " << add(1, 5) << std::endl;
	func_redirect.rewind();
	std::cout << "Result of function after rewind (6): " << add(1, 5) << std::endl;

	return 0;
}