/*
 * handl_com.h
 *
 *  Created on: 21.05.2013
 *      Author: andrej
 */

#ifndef HANDL_COM_H_
#define HANDL_COM_H_
#include <exception>
#include "Mad.h"

#define MAX_NUM_STRING 100

//обработка командной строки. Аргумент -максимальное число эммиторов в системе
void handl_command_line(mad_n::Mad* mad, unsigned int max_num, std::istream& stream)
		throw (const char*, std::exception);
#endif /* HANDL_COM_H_ */
