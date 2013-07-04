/*
 * Mad.h
 *
 *  Created on: 08.05.2013
 *      Author: andrej
 */

#ifndef MAD_H_
#define MAD_H_
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include "Center.h"
#include "Algorithm1.h"

//bool filter1(int* buf, unsigned num);
namespace mad_n {

/*
 *
 */

//ШАПКА СТРУКТУРЫ ПЕРЕДАЧИ ДАННЫХ
#define SIGNAL_SAMPL 3	 //код, идентифицирующий блок данных сигнала
struct DataUnit {
	int ident; //идентификатор блока данных
	int mode; //режим сбора данных
	unsigned int numFirstCount; //номер первого отсчёта
	int amountCount; //количество отсчётов (1 отс = 4 x 4 байт)
	unsigned int id_MAD; //идентификатор МАДа
	int sampl;
};

class Mad {
	//ЗАКРЫТЫЕ ЧЛЕНЫ
	unsigned int __id; //идентификатор мада
	int __mode;	//режим работы мада
	sockaddr_in __madAddr; //адрес управляющего канала мада
	static sockaddr_in __madAddr_brod; //широковещательный адрес подсети мадов
	static int __sockData; //сокет приёма данных от мада
	static int __sockMon; //сокет приёма мониторограмм от мада
	static int __sockCon; //сокет управления мадами (передающий)
	time_t __curTimeOut;	//хранит текущий штамп времени исходящих пакетов
	unsigned int __countFreqOut; //счётчик быстрых пакетов
	static time_t __timeSync;	//хранит время текущего sync
	static int __buf[4100];	//приёмный буфер
	static size_t __len;//длина блока полезных данных, хранящегося в приёмном буфере(не считая метки времени)
	static unsigned int __numMad; //количество мадов в системе
	static bool __enabMesMonit; //если true - разрешено сообщать о приёме мониторограммы
	static bool __enabMesData; //если true - разрешено сообщать о приёме пакетов данных
	bool __isEnableTrans;	//разрешение передачи данных на БЦ
	center_n::Center* __pcenter;	//объект БЦ
	//ЗАКРЫТЫЕ ФУНКЦИИ
	void recD(void); //обработка входных пакетов данных
	void recM(void); //обработка входных пакетов мониторограмм
	//void sync(void);	//передача синхросигнала маду
	void checkRate(void); //проверка на излишне большую величину потока исходящих пакетов данных
	//идентификаторы команд для мада
	enum {
		COM_SET_GAIN = 2, //изменить коэфиициент усиления
		COM_SET_MODE = 4, //изменить режим работы
		COM_GET_STATUS_MAD = 8, //запрос глобальной структуры состояния мада
		COM_SET_NOISE = 10 //изменить шумовой порог

	};
public:
	//классы алгоритмы
	Algorithm1<short> __alg1;
//ОТКРЫТЫЕ ФУНКЦИИ
	enum {
		CONTINUOUS, //непрерывный поток данных
		DETECTION1, //фильтрованный поток данных (1 алгоритм распознавания)
		SILENCE, //режим молчания
		ALGORITHM1	//режим первого алгоритма
	};
	friend void receiptData(Mad* pmad); //обработка входных пакетов данных
	friend void receiptMon(Mad* pmad); //обработка входных пакетов мониторограмм
	friend void receiptCon(Mad* pmad, int num); //обработка входных пакетов на управляющем порту
	static void synchronization(void); //синхронизация мадов
	static int getSockData(void); //возвращает сокет для приёма данных от мадов
	static int getSockMon(void); //возвращает сокет для приёма мониторограмм от мадов
	static int getSockCon(void); //возвращает сокет для приёма мониторограмм от мадов
	static void enableMesMonitor(bool status); //разрешить или запретить сообщать о приёме мониторограмм
	static bool getIsEnableMesMonitor(void); //получить статус сообщений о приёме мониторограмм
	static void enableMesData(bool status); //разрешить или запретить сообщать о приёме пакетов данных
	static bool getIsEnableMesData(void); //получить статус сообщений о приёме пакетов данных
	void comChangeGain(bool isBrod, int *gain); //команда изменить коэффициент усиления мада, если isBrod = 1 - то для всех мадов
	void comChangeNoise(bool isBrod, int& noise); //команда изменить шумовой порог мада, если isBrod = 1 - то для всех мадов
	void comChangeMode(bool isBrod, int& mode); //команда изменить режим мада, если isBrod = 1 - то для всех мадов
	void comGetStatus(bool isBrod); //запрос статуса мада, если isBrod = 1 - то для всех мадов
	unsigned getNumMad(void);	//получить количество мадов
	Mad(center_n::Center* pcen, unsigned int i = 1,
			char* cip = "192.168.203.31", unsigned int pD = 31001,
			unsigned int pM = 31003, unsigned int pC = 31000, bool isET = true);
	Mad(const Mad&);
	virtual ~Mad();
};
void receiptData(Mad* pmad);
void receiptMon(Mad* pmad);
void receiptCon(Mad* pmad, int num);

inline int Mad::getSockData(void) {
	return __sockData;
}

inline int Mad::getSockMon(void) {
	return __sockMon;
}

inline int Mad::getSockCon(void) {
	return __sockCon;
}

inline void Mad::enableMesMonitor(bool status) {
	__enabMesMonit = status;
}

inline bool Mad::getIsEnableMesMonitor(void) {
	return __enabMesMonit;
}

inline void Mad::enableMesData(bool status) {
	__enabMesData = status;
}

inline bool Mad::getIsEnableMesData(void) {
	return __enabMesData;
}

} /* namespace mad_n */
#endif /* MAD_H_ */
