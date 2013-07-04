/*
 * Algorithm1.cpp
 *
 *  Created on: 21.05.2013
 *      Author: andrej
 */

#define TICK 1000000 //период замеров состояния fifo буфера алгоритма (в микросекундах)
#define PERIOD 60
#define STAT_ALG 5 //идентификатор пакета статистики алгоритма
#define LEN_WINDOW (SECTION_LENGTH * 4)	 //длина измерительного окна
namespace mad_n {

template<class Tsampl> Algorithm1<Tsampl>::Algorithm1(
		void (*trans)(void *buf, size_t size), unsigned id) :
		ptrans(trans) {
	isRunThread__ = false;
	isRunFollowThread__ = false;
	pool.end = pool.sampl;
	pool.count = pool.sampl;
	period__ = PERIOD;
	bufStat__.ident = STAT_ALG;
	bufStat__.num_alg = 1;
	bufStat__.id_MAD = id;
}

template<class Tsampl>
void Algorithm1<Tsampl>::pass(void* buf, size_t& size) {
	if (isRunThread__ && size == sizeof(DataUnitPlusTime)) {
		DataUnitPlusTime* pu = reinterpret_cast<DataUnitPlusTime*>(buf);
		mut__.lock();
		fifo__.push_back(*pu);
		mut__.unlock();
	}
	return;
}

template<class Tsampl>
void Algorithm1<Tsampl>::open(void) {
	isRunThread__ = true;
	std::thread t(&Algorithm1::algorithm, this);
	t.detach();
}

template<class Tsampl>
Algorithm1<Tsampl>::~Algorithm1() {
	close();
	close_follow();
	mut__.unlock();
}

template<class Tsampl>
void Algorithm1<Tsampl>::toBank(Tsampl* buf, int num, DataUnitPlusTime* pack,
		int first_count) {
	memcpy(pack->sampl, buf, num * sizeof(Tsampl));
	pack->amountCount = num;
	pack->numFirstCount += first_count / 4;
	ptrans(pack,
			sizeof(DataUnitPlusTime) + num * sizeof(Tsampl)
					- sizeof(pack->sampl));

}

template<class Tsampl>
Algorithm1<Tsampl>::Algorithm1(const Algorithm1& a) {
	this->fifo__ = a.fifo__;
	this->isRunThread__ = a.isRunThread__;
	this->isRunFollowThread__ = a.isRunFollowThread__;
	this->pool = a.pool;
	this->ptrans = a.ptrans;
	this->period__ = a.period__;
}

template<class Tsampl>
void Algorithm1<Tsampl>::algorithm(void) {
	int correct = 0;
	std::string str = "Создан поток алгоритма 1 для МАД "
			+ std::to_string(bufStat__.id_MAD) + " pid = "
			+ std::to_string(getpid());
	to_journal(str);
	for (;;) {
		if (!isRunThread__)
			break;
		mut__.lock();
		if (fifo__.empty()) {
			mut__.unlock();
			continue;
		}
		pool.begin = pool.count + correct;
		//отсчёты сигналов копируются в пул
		memcpy(static_cast<void*>(pool.begin),
				reinterpret_cast<void*>(fifo__.front().sampl),
				sizeof(DataUnitPlusTime::sampl));
		pool.end = pool.begin + sizeof(fifo__.front().sampl) / sizeof(Tsampl);
		mut__.unlock();
		//непосредственно фильтрация
		while ((pool.count + LEN_WINDOW)<= pool.end) {
			if (SectionHasNeutrinoLikePulse(
					reinterpret_cast<unsigned*>(pool.count) /*, LEN_WINDOW*/,
					true)) {
				str = "Пойман нейтрино на МАД "
						+ std::to_string(bufStat__.id_MAD);
				to_journal(str);
				mut__.lock();
				toBank(pool.count, LEN_WINDOW, &fifo__.front(),
						pool.count - pool.begin);
				mut__.unlock();
				pool.count += LEN_WINDOW;
			} else
				pool.count += LEN_WINDOW / 2;
		}
		//правка членов структуры pool
		correct = pool.end - pool.count;
		if (correct)
			//перемещение оставшихся отсчётов в начало пула
			memmove(static_cast<void*>(pool.sampl), pool.count,
					correct * sizeof(Tsampl));
		pool.count = pool.sampl;
		mut__.lock();
		fifo__.pop_front();
		mut__.unlock();

	}
	pool.end = pool.count = pool.sampl;
	str = "Уничтожен поток алгоритма 1 для МАД "
			+ std::to_string(bufStat__.id_MAD);
	to_journal(str);
	return;
}

template<class Tsampl>
void Algorithm1<Tsampl>::open_follow(void) {
	isRunFollowThread__ = true;
	std::thread t1(&Algorithm1::follow_algorithm, this);
	t1.detach();
}

template<class Tsampl>
void Algorithm1<Tsampl>::close_follow(void) {
	isRunFollowThread__ = false;
}

template<class Tsampl>
void Algorithm1<Tsampl>::change_period(unsigned p) {
	period__ = p;
}

template<class Tsampl>
void Algorithm1<Tsampl>::follow_algorithm(void) {
	double num_aver = 0; //среднее наблюдаемое количество пакетов в очереди за время измерения
	unsigned num_max = 0; //максимальное наблюдаемое количество пакетов в очереди за время измерения
	unsigned num = 0;
	unsigned int count = 0; //количество учтённых данных на данном промежутке измерения
	unsigned int sum = 0;
	int t;
	time_t tStamp = time(NULL) + PERIOD;
	std::string str = "Создан поток сопровождения для алгоритма 1 для МАД "
			+ std::to_string(bufStat__.id_MAD) + " pid = "
			+ std::to_string(getpid());
	to_journal(str);
	for (;;) {
		if (!isRunFollowThread__)
			break;
		mut__.lock();
		num = fifo__.size();
		mut__.unlock();
		sum += num;
		num_aver = sum / ++count;
		if (num > num_max)
			num_max = num;
		usleep(TICK);
		if ((t = time(NULL)) >= tStamp) {
			//передача данных на БЦ
			bufStat__.average = static_cast<int>(num_aver);
			bufStat__.maximum = num_max;
			bufStat__.time = t;
			ptrans(&bufStat__, sizeof(StatAlg));
			//обнуление счётчиков
			count = sum = num_max = 0;
			tStamp = t + PERIOD;
		}

	}
	str = "Уничтожен поток сопровождения для алгоритма 1 для МАД "
			+ std::to_string(bufStat__.id_MAD);
	to_journal(str);
	return;
}

} /* namespace mad_n */
