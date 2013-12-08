/*
 * Gasik.h
 *
 *  Created on: 24 нояб. 2013 г.
 *      Author: andrej
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <deque>
#include <mutex>
#include <future>
#include <memory>
#include <algorithm>

#ifndef GASIK_H_
#define GASIK_H_

namespace mad_n {
#define NUM_SAMPL_GASIK (2000 * 4) //количество отсчётов в пакете
/*
 *
 */
class Gasik {
public:
	typedef std::deque<std::unique_ptr<int[]> > deqU;
	struct DataUnitPlusTime {
		unsigned time;
		int ident; //идентификатор блока данных
		int mode; //режим сбора данных
		unsigned int numFirstCount; //номер первого отсчёта
		int amountCount; //количество отсчётов (1 отс = 4 x 4 байт)
		unsigned int id_MAD; //идентификатор МАДа
		//int sampl[NUM_SAMPL_GASIK];
	};
	void pass(const int* buf, unsigned size);	//передача пакета в алгоритм
	void open(void); //открытие потока алгоритма
	void close(void); //закрытие потока алгоритма
	Gasik(void (*trans)(void *buf, size_t size), unsigned const& id);
	Gasik(Gasik&& a);
	virtual ~Gasik();
private:
	std::mutex mut__;
	deqU fifo__;	//очередь пакетов
	void (*ptrans)(void *buf, size_t size); //указатель на функцию передачи выделенных сигналов на берег
	bool isRunThread__;	//поток запущен
	std::future<void> end__; //будущий результат, возвращаемый только после уничтожения потока алгоритма

	void algorithm(void); //алгоритм Гасик
};

} /* namespace mad_n */

#endif /* GASIK_H_ */
