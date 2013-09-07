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
	const int * data,
	const int data_length,
	const bool simple_method = false,
	const bool debug_info = false 
){
	bool result = false;		//Возвращаемый результат
  //unsigned int * UData = new unsigned int [CHNUM * SECTION_LENGTH];		//Удобнее работать с массивом указателей
	int i, j;
	FA ** FAData = new FA * [CHNUM];											//Внутреннее представление данных в виде массивов значений

	for (i = 0; i < CHNUM; i++) 
	{
		FAData[i] = new FA(data_length);
		for (j = 0; j < data_length; j++) 
		{
			(*FAData[i])[j] = data[i + CHNUM*j];
		};
	};

	//Поднять в описание?
	TApproximabilityEventRecognizer recognizer(debug_info);
	recognizer.SwitchToSimpleMethod(simple_method);
	vector <TBipolarPulseVector *> bipolar_pulse_vectors;
	TBipolarPulseVector * temp_bipolar_pulse_vector;

	//Заполняем вектора обнаруженных биполярных импульсов
	for (i = 0; i < CHNUM; i++)
	{
		temp_bipolar_pulse_vector = new TBipolarPulseVector();
		bipolar_pulse_vectors.push_back(temp_bipolar_pulse_vector); 	
		recognizer.Recognize(FAData[i], *(bipolar_pulse_vectors[i]));
	};

	if (debug_info)
	{
		cout<< recognizer.debuginfo->c_str();
		//Выводим содержимое в поток
		for (i = 0; i < CHNUM; i++)
		{
			cout<<"bipolar_pulse_vectors["<<i<<"]"<<endl;
			for (j = 0; j < int((bipolar_pulse_vectors[i])->size()); j++)
			{
				cout<<"pulse number "<<j<<endl;
				cout<<"begin "<<(*bipolar_pulse_vectors[i])[j].begin<<"; ";
				cout<<"end "<<(*bipolar_pulse_vectors[i])[j].end<<"; ";
				cout<<"amplitude "<<(*bipolar_pulse_vectors[i])[j].amplitude<<"; ";
				cout<<"duration "<<(*bipolar_pulse_vectors[i])[j].duration<<"; ";
				cout<<endl;
			};
			cout<<endl;
		};
	};
	
	result = CellTrigger(bipolar_pulse_vectors);
	//try
	
	//Освобождаем переменные
	for (i = 0; i < CHNUM; i++)	delete bipolar_pulse_vectors[i];
	for (i = 0; i < CHNUM; i++)	delete FAData[i];
	delete [] FAData;
	//for (i = 0; i < CHNUM; i++)	delete [] UData[i]; �� ������, ��������� ��� ��������� ���������
	//delete [] UData;
	
	return result;
};

#endif
