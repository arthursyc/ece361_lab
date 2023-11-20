#include <stdlib.h>
#include <string.h>

#include "helpers.h"

char** parse(char* buffer) {
	char delim[] = " ";
	int arr_size = 0;
	char** array;

	char* ptr = strtok(buffer, delim);
	while (ptr != NULL) {
		array = (char**) reallocarray(array, ++arr_size, sizeof(char*));
		array[arr_size - 1] = ptr;
		ptr = strtok(NULL, delim);
	}

	return array;
}
