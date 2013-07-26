/*
 * journalling.cpp
 *
 *  Created on: 24.06.2013
 *      Author: andrej
 */

#include <mutex>
#include <iomanip>
#include <time.h>
#include <ctime>
#include <fstream>
#include "journalling.h"
#include <iostream>

using namespace std;

static struct {
	string file;
	mutex mut;
} logfile;

void to_journal(const string& str) {
	static int day = -1;
	char m[30];
	ofstream log;
	logfile.mut.lock();
	log.open(logfile.file, ios::out | ios::app);
	if (!log.is_open()) {
		cout << "Невозможно открыть файл ведения журнала\n";
		return;
	}
	time_t tim = time(NULL);
	tm* t = localtime(&tim);
	if (day != t->tm_mday) {
		day = t->tm_mday;
		strftime(m, sizeof(m), "%d %B %y ( %A )", t);
		log << "\t\t\t" << m << endl;
	}
	strftime(m, sizeof(m), "%X", t);
	log << left << setw(10) << m << str << endl;
	log.close();
	logfile.mut.unlock();
}

void set_logfile(const string& newfile) {
	logfile.mut.lock();
	logfile.file = newfile;
	logfile.mut.unlock();
	return;
}

