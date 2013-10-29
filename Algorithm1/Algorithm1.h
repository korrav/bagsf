/*
 * Algorithm1.h
 *
 *  Created on: 21.05.2013
 *      Author: andrej
 */

#ifndef ALGORITHM1_H_
#define ALGORITHM1_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <deque>
#include <mutex>
#include <future>

namespace mad_n {
#define NUM_SAMPL_PACK (1000 * 4) //количество отсчётов в пакете
#define JUMP  (200 * 4) //шаг перемещения измерительного окна
/*
 *
 */
class Algorithm1 {
public:
	struct DataUnitPlusTime {
		unsigned time;
		int ident; //идентификатор блока данных
		int mode; //режим сбора данных
		unsigned int numFirstCount; //номер первого отсчёта
		int amountCount; //количество отсчётов (1 отс = 4 x 4 байт)
		unsigned int id_MAD; //идентификатор МАДа
		int sampl[NUM_SAMPL_PACK];
	};

	void pass(void* buf, size_t& size);	//передача пакета в алгоритм
	void open(void); //открытие потока алгоритма
	void open_follow(void); //открытие потока сопровождения алгоритма
	void close(void); //закрытие потока алгоритма
	void close_follow(void); //закрытие потока сопровождения алгоритма
	void change_period(unsigned p); //изменение периода замеров статистики алгоритма
	explicit Algorithm1(void (*trans)(void *buf, size_t size), unsigned id);
	Algorithm1(const Algorithm1& a);
	virtual ~Algorithm1();
private:
	std::mutex mut__;
	std::deque<DataUnitPlusTime> fifo__;	//очередь пакетов
	void (*ptrans)(void *buf, size_t size); //указатель на функцию передачи выделенных сигналов на берег
	void toBank(int* buf, int num, DataUnitPlusTime* pack, int first_count); //функция, где блок данных, содержащий выделенный сигнал, упаковывается и пересылается на берег
	void algorithm(void); //алгоритм распознавания, выполняющийся в отдельном потоке
	void follow_algorithm(void); //сопровождающий алгоритма распознавания
	std::future<void> end_alg; //будущий результат, возвращаемый только после уничтожения потока алгоритма
	std::future<void> end_foll_alg; //будущий результат, возвращаемый только после уничтожения потока алгоритма сопровождения

	unsigned int period__; //перид одного измерения ресурсов памяти, используемой для размещения элементов fifo буфера
	bool isRunThread__;	//поток запущен
	bool isRunFollowThread__;//поток сопровождения алгоритма распозования запущен
	struct {
		int sampl[NUM_SAMPL_PACK * 2];	//отсчёты в пуле
		int *begin; //позиция начала обрабатываемого в данный момент пакета данных
		int *count; //текущая позиция
		int *end; //крайняя позиция
	} pool;

	struct StatAlg {
		unsigned time;	//время отправления
		int ident;	//идентификатор пакета
		int num_alg;	//номер аргумента
		unsigned maximum; //максимальное количество пакетов в памяти за период измерения
		unsigned average; //среднее количество пакетов в памяти за период измерения
		unsigned int id_MAD; //идентификатор МАДа
	} bufStat__;
};

inline void Algorithm1::close(void) {
	if (isRunThread__) {
		mut__.lock();
		isRunThread__ = false;
		mut__.unlock();
		close_follow();
		end_alg.get();
	}
}

} /* namespace mad_n */
#endif /* ALGORITHM1_H_ */
