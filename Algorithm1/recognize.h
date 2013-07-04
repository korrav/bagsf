/************************************************/
//recognize.h
/************************************************/

//Это модуль содержит интерфейсную функцию SectionHasNeutrinoLikePulse

#ifndef RECOGNIZE_H
#define RECOGNIZE_H

#include "TEventRecognizer.h"
#include "TData.h"

#define SECTION_LENGTH TIME_WINDOW * 2	//Величина временного окна в единицах отчетов удваиваем, с учетом перекрытия, чтобы не пропустить импульсов на стыке
#define JUMP_SIZE SECTION_LENGTH/2			//Величина скачка - окна перекрываются на половину
#define CHNUM 4

//На вход подается четыре массива (по числу каналов) четырехбайтовых беззнаковых целых 
//значений отчетов АЦП. Длина массивов = SECTION_LENGTH на выходе true, 
//если участок данных содержит нейтриноподобные импульсы
bool SectionHasNeutrinoLikePulse(
	const unsigned int * Data,
	const bool simple_method = false,
	const bool debug_info = false 
);

#endif
