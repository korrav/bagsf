/*
 * main.cpp
 *
 *  Created on: 08.05.2013
 *      Author: andrej
 */
/*Параметры командной строки:
 * --ipm1  ip адрес первого мада
 * --ipm2  ip адрес второго мада
 * --ipm3  ip адрес третьего мада
 * --pD    порт данных
 * --pM    порт мониторограмм
 * --pC    порт управления
 * --ipC   ip адрес БЦ
 * --pCen  порт связи с БЦ
 * --s     количество секунд задержки перед стартом программы
 * --scr   набор команд для выполнения вначале программы
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <fstream>
#include "Mad.h"
#include "Center.h"
#include "errno.h"
#include "handl_com.h"
#include "journalling.h"

#define IP_MAD1 "192.168.127.31"
#define IP_MAD2 "192.168.127.32"
#define IP_MAD3 "192.168.127.33"
#define IP_CENTER "192.168.1.156"
#define PORT_DATA 31001
#define PORT_MONITOROGRAMM 31003
#define PORT_CONTROL 31000
#define PORT_CENTRAL 31011
#define LOGFILE "./log"
int main(int argc, char* argv[]) {
	std::ifstream scr; //поток файла команд
	fd_set fdin; //набор дескрипторов, на которых ожидаются входные данные
	char ipm1[15] = IP_MAD1;
	char ipm2[15] = IP_MAD2;
	char ipm3[15] = IP_MAD3;
	char ipC[15] = IP_CENTER;
	//порты
	unsigned int pD = PORT_DATA;
	unsigned int pM = PORT_MONITOROGRAMM;
	unsigned int pC = PORT_CONTROL;
	unsigned int pCen = PORT_CENTRAL;
	set_logfile("./log");
	//время сна команды после запуска
	unsigned int timeSleep = 0;
	//Инициализация аргументами командной строки соответствующих параметров программы
	for (int i = 1; i < argc; i += 2) {
		if (!strcmp("--pD", argv[i]))
			pD = atoi(argv[i + 1]);
		else if (!strcmp("--pM", argv[i]))
			pM = atoi(argv[i + 1]);
		else if (!strcmp("--pC", argv[i]))
			pC = atoi(argv[i + 1]);
		else if (!strcmp("--pCen", argv[i]))
			pCen = atoi(argv[i + 1]);
		else if (!strcmp("--s", argv[i]))
			timeSleep = atoi(argv[i + 1]);
		else if (!strcmp("--ipm1", argv[i]))
			strcpy(ipm1, argv[i + 1]);
		else if (!strcmp("--ipm2", argv[i]))
			strcpy(ipm2, argv[i + 1]);
		else if (!strcmp("--ipm3", argv[i]))
			strcpy(ipm3, argv[i + 1]);
		else if (!strcmp("--ipC", argv[i]))
			strcpy(ipC, argv[i + 1]);
		else if (!strcmp("--scr", argv[i])) {
			scr.open(argv[i + 1]);
			if (!scr.is_open())
				std::cout
						<< "Программа не смогла открыть входной поток данных\n";
		} else
			printf("%d параметр не поддерживается программой\n", i);
	}
	//засыпание перед запуском программы
	sleep(timeSleep);
	set_logfile(LOGFILE);
	std::string str = "Программа запущена"
			+ (" pid = " + std::to_string(getpid()));
	to_journal(str);
	center_n::Center Center(ipC, pCen);
	mad_n::Mad::initialize(Center);
	mad_n::Mad mad[3] = { mad_n::Mad(1, ipm1, pD, pM, pC), mad_n::Mad(2, ipm2,
			pD, pM, pC), mad_n::Mad(3, ipm3, pD, pM, pC) };
	//вычисление наибольшого по значению дескриптора
	int max_d = mad_n::Mad::getSockData();
	if (max_d < mad_n::Mad::getSockMon())
		max_d = mad_n::Mad::getSockMon();
	if (max_d < mad_n::Mad::getSockCon())
		max_d = mad_n::Mad::getSockCon();
	if (max_d < mad_n::Mad::__pcenter->getSock())
		max_d = mad_n::Mad::__pcenter->getSock();
	//выполнение предварительного файла команд
	if (scr.is_open())
		while (scr) {
			try {
				handl_command_line(mad, MAX_NUM_STRING, scr);
			} catch (const char* str) {
				std::cout << str << std::endl;
			} catch (const std::exception& a) {
				std::cout << a.what() << std::endl;
			}
		}
	scr.close();
	//ГЛАВНЫЙ ЦИКЛ
	int status = 0;
	printf("hello\n");
	for (;;) {
		FD_ZERO(&fdin);
		FD_SET(STDIN_FILENO, &fdin);
		FD_SET(mad_n::Mad::getSockData(), &fdin);
		FD_SET(mad_n::Mad::getSockMon(), &fdin);
		FD_SET(mad_n::Mad::getSockCon(), &fdin);
		FD_SET(Center.getSock(), &fdin);
		//ожидание событий
		status = select(max_d + 1, &fdin, NULL, NULL, NULL);
		if (status == -1) {
			if (errno == EINTR)
				continue;
			else {
				perror("Функция select завершилась крахом\n");
				exit(1);
			}
		}
		if (FD_ISSET(mad_n::Mad::getSockData(), &fdin))
			mad_n::receiptData(mad);
		if (FD_ISSET(mad_n::Mad::getSockMon(), &fdin))
			mad_n::receiptMon(mad);
		if (FD_ISSET(STDIN_FILENO, &fdin)) {
			try {
				handl_command_line(mad, MAX_NUM_STRING, std::cin);
			} catch (const char* str) {
				std::cout << str << std::endl;
			} catch (const std::exception& a) {
				std::cout << a.what() << std::endl;
			}
		}
		if (FD_ISSET(mad_n::Mad::getSockCon(), &fdin))
			mad_n::receiptCon(mad, 3);
		if (FD_ISSET(Center.getSock(), &fdin))
			Center.receipt();
	}

}
