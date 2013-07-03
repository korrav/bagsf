#ifndef TBIPOLAR_PULSE_H
#define TBIPOLAR_PULSE_H

//тип - биполярный импульс
struct TBipolarPulse
{
	int begin;			//начало (непрерывное время) в единицах отчетов, 
	int end;				//конец(непрерывное время) в единицах отчетов, 
	fp amplitude; //размах (непрерывная величина) в делениях АЦП, 
	int duration;	//длительность в единицах отчетов
	bool IsEmpty(){return begin == 0;}; 
};

static const TBipolarPulse EmptyBipolarPulse = {
	0,
	0,
	0.,
	0
};

#endif
