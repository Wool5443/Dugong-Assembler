#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <string.h>
#include "Utils.hpp"

const double ABSOLUTE_TOLERANCE = 1e-5;

void selectionSort(void* data, size_t elementCount, size_t elementSize, CompareFunction_t* compareFunction);

void quickSort(void* start, void* end, size_t elementSize, CompareFunction_t* compareFunction);

void* partition(void* left, void* right, size_t elementSize, CompareFunction_t compareFunction);

void sort3Elements(void* data, size_t elementSize, CompareFunction_t compareFunction);

void sort2Elements(void* data, size_t elementSize, CompareFunction_t compareFunction);

bool IsEqual(const double x1, const double x2)
{
	return fabs(x1 - x2) < ABSOLUTE_TOLERANCE;
}

void CopyArray(double destination[], double source[], int length)
{
	MyAssertHard(destination, ERROR_NULLPTR, );
	MyAssertHard(source, ERROR_NULLPTR, );

	for (int i = 0; i < length; i++)
		destination[i] = source[i];
}

const void* MinArray(const void* data, size_t elementCount, size_t elementSize, CompareFunction_t* compareFunction)
{
	const void* min = data;
	for (size_t i = 1; i < elementCount; i++)
		if (compareFunction(min, data + i * elementSize) > 0)
		{
			min = data + i * elementSize;
		}
	return min;	
}

const void* MaxArray(const void* data, size_t elementCount, size_t elementSize, CompareFunction_t* compareFunction)
{
	const void* max = data;
	for (size_t i = elementSize; i < elementCount; i++)
		if (compareFunction(max, data + i * elementSize) < 0)
			max = data + i * elementSize;
	return max;	
}

void Swap(void* a, void* b, size_t size)
{
    char* _a = (char*)a;
    char* _b = (char*)b;

    for (size_t curByte = 0; curByte < size; curByte++)
    {
        char _temp = _a[curByte];
        _a[curByte] = _b[curByte];
        _b[curByte] = _temp;
    }
}

void Sort(void* data, size_t elementCount, size_t elementSize, CompareFunction_t* compareFunction)
{
	MyAssertHard(data, ERROR_NULLPTR, );
	MyAssertHard(compareFunction, ERROR_NO_COMPARATOR, );

	quickSort(data, data + (elementCount - 1) * elementSize, elementSize, compareFunction);
	// selectionSort(data, elementCount, elementSize, compareFunction);
}

void selectionSort(void* data, size_t elementCount, size_t elementSize, CompareFunction_t* compareFunction)
{
	MyAssertHard(data, ERROR_NULLPTR, );
	MyAssertHard(compareFunction, ERROR_NO_COMPARATOR, );

	for (size_t i = 0; i < elementCount; i++)
	{
		void* temp = (void*)MinArray(data + i * elementSize, elementCount - i, elementSize, compareFunction);
		Swap(data + i * elementSize, temp, elementSize);
	}
}

void quickSort(void* start, void* end, size_t elementSize, CompareFunction_t* compareFunction)
{
	MyAssertHard(start, ERROR_NULLPTR, );
	MyAssertHard(end, ERROR_NULLPTR, );

	if (end <= start)
		return;
	
	size_t dataLength = (size_t)((char*)end - (char*)start) / elementSize + 1;

	switch (dataLength)
	{
	case 2:
		sort2Elements(start, elementSize, compareFunction);
		return;
	case 3:
		sort3Elements(start, elementSize, compareFunction);
		return;	
	default:
		break;
	}

	void* pivot = partition(start, end, elementSize, compareFunction);

	quickSort(start, pivot - elementSize, elementSize, compareFunction);
	quickSort(pivot + elementSize, end, elementSize, compareFunction);
}

void* partition(void* start, void* end, size_t elementSize, CompareFunction_t compareFunction)
{
	MyAssertHard(start, ERROR_NULLPTR, );
	MyAssertHard(end, ERROR_NULLPTR, );
	
	srand((unsigned int)time(NULL));
	size_t arrayLength = ((size_t)end - (size_t)start) / elementSize + 1;

	// void* pivotPtr = start + arrayLength / 2 * elementSize;
	void* pivotPtr = start + ((size_t)rand() % (arrayLength - 2) + 1) * elementSize;

#ifdef TESTING
	SetConsoleColor(stdout, RED);
	printf("Pivot = %d\n", *(int*)pivotPtr);
	SetConsoleColor(stdout, WHITE);

	for (void* t = start; t <= end; t += elementSize)
		printf("%d ", *(int*)t);
	printf("\n\n");
#endif

	Swap(start, pivotPtr, elementSize);
	void* pivotValue = start;
	start += elementSize;

	void* left  = start;
	void* right = end;

	int comp1 = 0, comp2 = 0;
	while (left < right)
	{
	    comp1 = compareFunction(left,  pivotValue);
		comp2 = compareFunction(right, pivotValue);
	
		while (comp1 < 0 && left < right)
		{
			left += elementSize;
			comp1 = compareFunction(left, pivotValue);
		}
	
		while (comp2 > 0 && left < right) 
		{
			right -= elementSize;
			comp2 = compareFunction(right, pivotValue);
		}

#ifdef TESTING
		SetConsoleColor(stdout, GREEN);
		for (void* t = start; t <= end; t += elementSize)
			printf("%zu ", ((size_t)t - (size_t)start) / elementSize);
		SetConsoleColor(stdout, WHITE);
		printf("\n");

		for (void* t = start; t <= end; t += elementSize)
			printf("%d ", *(int*)t);
		printf("\n");

		printf("Left = %d, i =  %zu Right = %d, i = %zu\n", *(int*)left,  ((size_t)left  - (size_t)start) / elementSize,
													        *(int*)right, ((size_t)right - (size_t)start) / elementSize);
#endif

		if(comp1 == 0 && comp2 == 0)
			right -= elementSize;
		else
			Swap(left, right, elementSize);

#ifdef TESTING
		SetConsoleColor(stdout, GREEN);
		for (void* t = start; t <= end; t += elementSize)
			printf("%zu ", ((size_t)t - (size_t)start) / elementSize);
		SetConsoleColor(stdout, WHITE);
		printf("\n");

		for (void* t = start; t <= end; t += elementSize)
			printf("%d ", *(int*)t);
		printf("\n\n");
#endif
	}

	start -= elementSize;

	if (comp1 >= 0)
		pivotPtr = left - elementSize;
	else
		pivotPtr = left;

	Swap(pivotValue, pivotPtr, elementSize);

#ifdef TESTING
	SetConsoleColor(stdout, GREEN);
	for (void* t = start; t <= end; t += elementSize)
		printf("%zu ", ((size_t)t - (size_t)start) / elementSize);
	printf("\n");
	SetConsoleColor(stdout, WHITE);


	for (void* t = start; t <= end; t += elementSize)
		printf("%d ", *(int*)t);
	printf("\n\n");
#endif

	return pivotPtr;
}
void sort3Elements(void* data, size_t elementSize, CompareFunction_t compareFunction)
{
	if (compareFunction(data, data + elementSize) < 0)
	{
		if (compareFunction(data, data + 2 * elementSize) > 0)
			Swap(data, data + 2 * elementSize, elementSize);
	}
	else
	{
		if (compareFunction(data + elementSize, data + 2 * elementSize) < 0)
		{
			Swap(data, data + 1 * elementSize, elementSize);
		}
		else
		{
			Swap(data, data + 2 * elementSize, elementSize);
		}
	}
	if (compareFunction(data + elementSize, data + 2 * elementSize) > 0)
		Swap(data + elementSize, data + 2 * elementSize, elementSize);
}

void sort2Elements(void* data, size_t elementSize, CompareFunction_t compareFunction)
{
	if (compareFunction(data, data + elementSize) > 0)
		Swap(data, data + elementSize, elementSize);
}

void ClearBuffer(void)
{
	while (getchar() != '\n') { ; }
}

bool CheckInput()
{
	int c = getchar();
	while (c == ' ' || c == '\t') { c = getchar(); }

	return c == '\n';
}

void SetConsoleColor(FILE* place, const enum Color color)
{
	switch (color)
	{
	case RED:
		fprintf(place, "\033[0;31m");
		break;
	case GREEN:
		fprintf(place, "\033[0;32m");
		break;
	case WHITE:
		fprintf(place, "\033[0;37m");
		break;
	default: //set the color to white
		fprintf(place, "\033[0;37m");
		break;
	}
}

#define mmix(h, k) do { k *= m; k ^= k >> r; k *= m; h *= m; h ^= k; } while (0)

unsigned int CalculateHash(const void *key, size_t len, unsigned int seed)
{
	const unsigned int m = 0x5bd1e995;
	const int r = 24;
	unsigned int l = len;

	const unsigned char* data = (const unsigned char *)key;

	unsigned int h = seed;
	unsigned int k;

	while(len >= 4)
	{
		k = *(unsigned int*)data;

		mmix(h,k);

		data += 4;
		len -= 4;
	}

	unsigned int t = 0;

	switch(len)
	{
	case 3: t ^= data[2] << 16u;
	case 2: t ^= data[1] << 8u;
	case 1: t ^= data[0];
	default: break;
	};

	mmix(h,t);
	mmix(h,l);

	h ^= h >> 13;
	h *= m;
	h ^= h >> 15;

	return h;
}
