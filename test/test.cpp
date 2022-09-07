/*
 ELEMENT DISTINCTNESS ALGORITHM
 There are multiple ways of detecting whether the elements are distinct or not.
 One of the ways is to sort the elements in the array and check if the elements next to each other
 are equal or not.
 On the other hand an n^2 loop can be used to select an element from the array and check if the
 the element exists.
 The approach used in this algorithm is adding the elements in an array implementation of a binary tree
 If the elements already exists in the tree then the elements are not distinct else it is. The function
 performing the algorithm is isDistinct function.
*/
#include <iostream>
#include <limits>
#include <iomanip>
#include "../config.h"

using namespace std;

int main(void)
{
	init_accel();
	VIP_ENCINT a = 1;
	std::cout <<"a: ";
	a.print_val();
	std::cout<< "a:"<<std::hex <<a.decrypt_int()<<std::endl;
 	VIP_ENCINT b = 0;
	std::cout <<"b: ";
	b.print_val();
	std::cout<< "b:"<<std::hex <<b.decrypt_int()<<std::endl;
	std::cout<< "secret key:"<<std::hex << SECRET_KEY.getUpperValue_64b() << SECRET_KEY.getLowerValue_64b()<<std::endl;
	VIP_ENCINT c = a + b;
	std::cout <<"c: ";
	c.print_val();
	fprintf(stdout, " c = %d\n", VIP_DEC(c));
	return 0;
}

