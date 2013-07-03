/*
 * journalling.h
 *
 *  Created on: 24.06.2013
 *      Author: andrej
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifndef JOURNALLING_H_
#define JOURNALLING_H_
void to_journal(const std::string& str); //запись в журнал
void set_logfile(const std::string& newfile); //установка нового имени файла ведения журнала

#endif /* JOURNALLING_H_ */
