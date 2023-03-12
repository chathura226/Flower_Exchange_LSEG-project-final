#include<iostream>
#include<cstdio>
int main() {
	int a, b, c;
	if (FILE* filePointer = fopen("test.csv", "r")) {

		while (fscanf(filePointer, "%d,%d,%d", &a, &b, &c) == 3) {
			std::cout << a << " " << b << " " << c << std::endl;
		}
        std::cout<<"dr";
	}
}