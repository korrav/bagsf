/*
 * handl_com.cpp
 *
 *  Created on: 21.05.2013
 *      Author: andrej
 */
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <string>
#include "handl_com.h"
#include <fstream>

/*формат управляющих сообщений оператора струтурированы следующим образом:
 * КОМУ;КОМАНДА;АРГУМЕНТ 1;АРГУМЕНТ 2......
 */
/*КОМУ*/

#define MAD   		  "mad" //адресат - мад(суффиксом указывается его идентификатор, например m1)
#define PROGRAM       "pr" //адресат - программа
#define ALL_MADS      "amad" //адресат - все мады системы
/*КОМАНДЫ*/
#define SET_GAIN "s_g" //команда установить коэффициент усиления (в Дцб). Аргументы: 4; по порядку для всех каналов
#define SET_NOISE "s_n" //команда установить шумовой порог (в Дцб). Аргументы: 1; значение шума
#define GET_STATUS "g_s" //запрос статуса мада (в Дцб). Аргументы: 0
#define ENABLE_MES_MONITOR  "e_m" //разрешить сообщать о приёме мониторограммы. Аргументы: 0
#define SET_MODE "s_m"	//команда установить режим работы. Аргументы: 1; значение коэффициента усиления
//режимы работы
#define CONT    "c"		//непрерывный поток данных
#define	DET1    "d1" 	//фильтрованный поток данных (алгоритм распознавания по превышению порога)
#define	SIL     "s"     //режим молчания
#define	ALGOR1	"a1"	//режим первого алгоритма
#define	GAS	"g"	//режим Гасик
#define DISABLE_MES_MONITOR  "d_m" //запретить сообщать о приёме мониторограммы. Аргументы: 0
#define ENABLE_MES_DATA  "e_d" //разрешить сообщать о приёме пакетов данных. Аргументы: 0
#define DISABLE_MES_DATA  "d_d" //запретить сообщать о приёме пакетов данных. Аргументы: 0
#define GET_IS_MES_DATA "g_i_d" //получить сведения сообщается ли о приёме новых пакетов данных
#define GET_IS_MES_MONITOR "g_i_m" //получить сведения сообщается ли о приёме новых монирограмм
#define SET_PERIOD_ALG1 "s_p_a1"   //изменить период замеров статистики алгоритма 1. Аргументы 1. Длина периода в секундах
#define ENABLE_FOLLOW_ALG1 "e_f_a1" //разрешить поток сопровождения алгоритма 1
#define DISABLE_FOLLOW_ALG1 "d_f_a1" //разрешить поток сопровождения алгоритма 1
#define RUN "run" //выполнить командный файл. Аргумент: имя командного файла
#define FILL "f" //заполнить файл данных. Аргумент 1: p - размер 1 пакет, число - количество отсчётов. Аргумент 2: имя файла
using namespace std;

/*ОСТАЛЬНОЕ*/
#define MAX_ID_MAD 2
//string str[MAX_NUM_STRING];

//локальные функции
static unsigned short comstring(std::string* l, unsigned short num,
		istream& stream); /*возвращает массив, содержащий слова командной строки.
		 Передаётся незаполненный массив и его длина, возвращается количество слов в строке или 0 в случае ошибки*/
static void remove_spaces(string* s);	//удаление пробелов из строки
static void handl_program_mes(string* str, const unsigned short& num_word,
		mad_n::Mad* pmad); /*обработка сообщений к программе
		 в целом (передаётся массив с управляющим выражением и количество элементов в массиве*/
static void handl_all_mad_mes(mad_n::Mad* pmad, string* str,
		const unsigned short& num_word); //обработка сообщений, адресованных сразу ко всем мадам системы
static void handl_mad_mes(mad_n::Mad& mad, string* str,
		const unsigned short& num_word); //обработка сообщений, адресованных маду
//static short string2short(const string& str); //преобразование аргумента из строкового формата в короткий целочисленный
static int string2int(const string& str); //преобразование аргумента из строкового формата в целочисленный

void handl_command_line(mad_n::Mad* mad, unsigned int max_num, istream& stream)
		throw (const char*, std::exception) {
	string str[MAX_NUM_STRING];
	unsigned short num_word = 0;
	unsigned int id;
	size_t idx = 0;
	string digit;
	if (!(num_word = comstring(str, MAX_NUM_STRING, stream))) {
		throw "Строка команд содержит слишком много слов\n";
	} else if (num_word < 2) {
		throw "Строка команд содержит слишком мало слов\n";
	}
	//кому передаются сообщения
	if (str[0] == PROGRAM)
		handl_program_mes(&str[1], (num_word - 1), mad);
	else if (str[0] == ALL_MADS)
		handl_all_mad_mes(mad, &str[1], (num_word - 1));
	else if ((str[0].find(MAD, 0)) == 0) {
		digit.assign(str[0], strlen(MAD), string::npos);
		id = static_cast<unsigned int>(stoi(digit, &idx, 10));
		if (idx < digit.length() || id >= max_num)
			throw "МАДа с таким идентификатором в системе не существует\n";
		else
			handl_mad_mes(mad[id - 1], &str[1], (num_word - 1));
	} else
		cout << "Такая инструкция программой не поддерживается\n" << endl;
	return;
}

unsigned short comstring(string* l, unsigned short num, istream& stream) {
	unsigned short i = 0;
	unsigned int start = 0, end;
	string comlin;
	getline(stream, comlin);
	while ((end = comlin.find(";", start)) != string::npos) {
		l[i].assign(comlin, start, (end - start));
		remove_spaces(&l[i]);
		start = end + 1;
		if (++i >= num)
			return 0;
	}
	l[i].assign(comlin, start, string::npos);
	remove_spaces(&l[i]);
	return ++i;
}

void remove_spaces(string* s) {
	for (unsigned int i = 0; (i = s->find(" ", i)) < s->length();) {
		s->erase(i, 1);
		i = 0;
	}
}

void handl_program_mes(string* str, const unsigned short& num_word,
		mad_n::Mad* pmad) {
	if (str[0] == ENABLE_MES_MONITOR && num_word == 1) {
		mad_n::Mad::enableMesMonitor(1);
	} else if (str[0] == DISABLE_MES_MONITOR && num_word == 1) {
		mad_n::Mad::enableMesMonitor(0);
	} else if (str[0] == ENABLE_MES_DATA && num_word == 1) {
		mad_n::Mad::enableMesData(1);
	} else if (str[0] == DISABLE_MES_DATA && num_word == 1) {
		mad_n::Mad::enableMesData(0);
	} else if (str[0] == GET_IS_MES_DATA && num_word == 1) {
		if (mad_n::Mad::getIsEnableMesData())
			cout
					<< "Сообщения о приёме новых пакетов данных выводятся на экран\n";
		else
			cout
					<< "Сообщения о приёме новых пакетов данных не выводятся на экран\n";
	} else if (str[0] == GET_IS_MES_MONITOR && num_word == 1) {
		if (mad_n::Mad::getIsEnableMesMonitor())
			cout
					<< "Сообщения о приёме новых мониторограмм выводятся на экран\n";
		else
			cout
					<< "Сообщения о приёме новых мониторограмм не выводятся на экран\n";
	} else if (str[0] == RUN && num_word == 2) {
		ifstream scr(str[1]);
		//scr.open(str[1]);
		if (scr.is_open()) {
			while (scr) {
				try {
					handl_command_line(pmad, MAX_NUM_STRING, scr);
				} catch (const char* str) {
					std::cout << str << std::endl;
				} catch (const std::exception& a) {
					std::cout << a.what() << std::endl;
				}
			}
		} else
			cout << "Программа не может открыть командный файл " << str[1]
					<< endl;
		scr.close();
	} else
		throw "Неизвестная команда\n";
	return;
}

static void handl_all_mad_mes(mad_n::Mad* pmad, string* str,
		const unsigned short& num_word) {
	if (str[0] == SET_GAIN && num_word == 5) {
		int gain[4];
		for (int i = 1; i < 5; i++)
			gain[i - 1] = string2int(str[i]);
		pmad->comChangeGain(true, gain);
	} else if (str[0] == SET_NOISE && num_word == 2) {
		int noise = string2int(str[1]);
		pmad->comChangeNoise(true, noise);
	} else if (str[0] == GET_STATUS && num_word == 1) {
		pmad->comGetStatus(true);
	} else if (str[0] == SET_MODE && num_word == 2) {
		int mode = 0;
		if (str[1] == CONT)
			mode = mad_n::Mad::CONTINUOUS;
		else if (str[1] == DET1)
			mode = mad_n::Mad::DETECTION1;
		else if (str[1] == SIL)
			mode = mad_n::Mad::SILENCE;
		else if (str[1] == ALGOR1)
			mode = mad_n::Mad::ALGORITHM1;
		else if (str[1] == GAS)
			mode = mad_n::Mad::GASIK;
		else {
			cout << "Такой режим работы не поддерживается" << std::endl;
			return;
		}
		pmad->comChangeMode(true, mode);
	} else if (str[0] == SET_PERIOD_ALG1 && num_word == 2) {
		unsigned period = static_cast<unsigned>(string2int(str[1]));
		for (unsigned i = 0; i < pmad->getNumMad(); i++)
			pmad[i].__alg1.change_period(period);
	} else if (str[0] == ENABLE_FOLLOW_ALG1 && num_word == 1) {
		pmad->__alg1.open_follow();
	} else if (str[0] == DISABLE_FOLLOW_ALG1 && num_word == 1) {
		pmad->__alg1.close_follow();
	} else
		throw "Неизвестная команда\n";
	return;
}

static void handl_mad_mes(mad_n::Mad& mad, string* str,
		const unsigned short& num_word) {
	if (str[0] == SET_GAIN && num_word == 5) {
		int gain[4];
		for (int i = 1; i < 5; i++)
			gain[i - 1] = string2int(str[i]);
		mad.comChangeGain(false, gain);
	} else if (str[0] == SET_NOISE && num_word == 2) {
		int noise = string2int(str[1]);
		mad.comChangeNoise(false, noise);
	} else if (str[0] == GET_STATUS && num_word == 1) {
		mad.comGetStatus(false);
	} else if (str[0] == SET_MODE && num_word == 2) {
		int mode = 0;
		if (str[1] == CONT)
			mode = mad_n::Mad::CONTINUOUS;
		else if (str[1] == DET1)
			mode = mad_n::Mad::DETECTION1;
		else if (str[1] == SIL)
			mode = mad_n::Mad::SILENCE;
		else if (str[1] == ALGOR1)
			mode = mad_n::Mad::ALGORITHM1;
		else if (str[1] == GAS)
			mode = mad_n::Mad::GASIK;
		else {
			cout << "Такой режим работы не поддерживается" << std::endl;
			return;
		}
		mad.comChangeMode(false, mode);
	} else if (str[0] == SET_PERIOD_ALG1 && num_word == 2) {
		unsigned period = static_cast<unsigned>(string2int(str[1]));
		mad.__alg1.change_period(period);
	} else if (str[0] == ENABLE_FOLLOW_ALG1 && num_word == 1) {
		mad.__alg1.open_follow();
	} else if (str[0] == DISABLE_FOLLOW_ALG1 && num_word == 1) {
		mad.__alg1.close_follow();
	} else if (str[0] == FILL && (num_word == 2 || num_word == 3)) {
		int count = mad_n::Mad::SIZE_P;
		std::string path = "./data";
		if (str[1] != "p")
			count = string2int(str[1]);
		if (num_word != 2)
			path = str[2];
		mad.writeFile(count, path);
	} else
		throw "Неизвестная команда\n";
	return;
}

/*short string2short(const string& str) {
 size_t idx = 0;
 short dig = static_cast<short>(stoi(str, &idx, 10));
 if (idx < str.length())
 throw "У команды неправильный аргумент\n";
 else
 return dig;
 }*/

int string2int(const string& str) {
	size_t idx = 0;
	int dig = static_cast<int>(stoi(str, &idx, 10));
	if (idx < str.length())
		throw "У команды неправильный аргумент\n";
	else
		return dig;
}
