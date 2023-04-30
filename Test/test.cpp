#include <Windows.h>
#include <cmath>
#include <mutex>
#include "..\Pseudocode Interpreter MFC\IndexedList.h"
#include "iostream"
#include <random>
#include <ctime>
using namespace std;

void test1() {
	LARGE_INTEGER li, t1, t2, t3, t4;
	QueryPerformanceFrequency(&li);
	IndexedList<int> list1;
	for (int i = 0; i != 100000; i++) {
		list1.append(i);
	}
	IndexedList<int> list2(list1);
	QueryPerformanceCounter(&t1);
	list1[50000];
	QueryPerformanceCounter(&t2);
	cout << "普通链表查找：" << (double)(t2.QuadPart - t1.QuadPart) / li.QuadPart * 10e6 << "μs" << endl;
	QueryPerformanceCounter(&t3);
	list2[50000];
	QueryPerformanceCounter(&t4);
	cout << "跳跃式链表查找：" << (double)(t4.QuadPart - t3.QuadPart) / li.QuadPart * 10e6 << "μs" << endl;
}

void test2() {
	IndexedList<int> list1;
	for (int i = 0; i != 100; i++) {
		list1.append(i);
	}
	for (int i = 100; i != 200; i++) {
		list1.insert(0, i);
	}
	for (IndexedList<int>::iterator iter = list1.begin(); iter != list1.end(); iter++) {
		cout << *iter << ' ';
	}
}

void test3() {
	IndexedList<int> list1;
	for (int i = 0; i != 100; i++) {
		list1.append(i);
	}
	for (int i = 100; i != 0; i--) {
		srand(time(0));
		list1.pop(rand() % i);
		for (IndexedList<int>::iterator iter = list1.begin(); iter != list1.end(); iter++) {
			cout << *iter << ' ';
		}
		cout << endl;
	}
}

int main() {
	test3();
}