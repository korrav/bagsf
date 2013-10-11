/************************************************/
//TEventRecognizer.h
/************************************************/

//Набор классов для быстрого обнаружения нейтриноподобного акустического импульса, 
//его выделения из фона и определения его основных характеристик.
//Применяется на короткой акустичекой дорожке.

//Семейство классов, унаследованных от TBaseEventRecognizer являются реализациями
//различных методов выделения события из фона и определения основных характеристик
//акустического импульса

//Везде используется шаблон valarray, через который в c++ реализованы
//ССЫЛОЧНАЯ СЕМАНТИКА И ОТЛОЖЕННЫЕ ВЫЧИСЛЕНИЯ, например:
//FA((*PData)[abs(*PData) > Level/*это срез*/]/*это ссылки на значения*/)/*новый массив заполненный данными*/;

#ifndef TEVENT_RECOGNIZER_H
#define TEVENT_RECOGNIZER_H

#include "TData.h"
#include "VectorOperations.h"
#include "TBipolarPulse.h"
#include "WriteToLog.h"
//#include "Reconstruction.h"

enum TenumRecognitionMethod { irmApproximability, irmFourier, irmWavelet, irmWaveletWithNeuro };

typedef vector <TBipolarPulse> TBipolarPulseVector;

typedef TBipolarPulseVector * pBipolarPulseVector;

static const fp		ADC_UNIT_TIME = 5.3333333e-6,//5.3333333e-6,//соотв 187000Гц // 5.e-6,	//s, соотв 200kHz
									L = 1.5;								//Характерные размеры кластера установки
static const int  TIME_WINDOW = int(L/1420/ADC_UNIT_TIME);  //Временное окно (количество отсчетов)

static const int	C = 6,									//C*(W1+W2) > D
									MIN_D = 25,							//mks 
									MAX_D = 80;						//mks
static const fp		SIGMA_LEVEL = 2.0,//2.0,
									MIN_A =	SIGMA_LEVEL * 2.,							//sigma
									NORM_CHI_SQUARE_THRESHOLD = 0.5;//0.1;

/************************************************/
//Триггерная функция для отдельной ячейки детектора, возвращает true, если
//собирается по одному импульсу из каждого канала в пределах временного окна
//Действует по принципу "жадного" алгоритма, может работать неадекватно при 
//большой плотности импульсов
bool CellTrigger(
				vector <TBipolarPulseVector *> &bipolar_pulse_vectors,
				void * /*TBaseReconstructor * */ reconstructor = 0,
				void * /*TSourceParametersVector * */ source_parameters_vector = 0
			);

/************************************************/

class TBaseEventRecognizer {
protected:
 bool WriteDebugInfo;
 bool SimpleMethod;
public:
	TBaseEventRecognizer(const bool write_debug_info){WriteDebugInfo = write_debug_info;};
	virtual ~TBaseEventRecognizer(){};

	void SwitchToSimpleMethod(const bool simple_method = true){SimpleMethod = simple_method;};
	void SetWriteDebugInfo(bool write_debug_info){WriteDebugInfo = write_debug_info;};

	//Сюда можно подавать данные как знаковые, так и беззнаковые, как отцентрованные, так и не отцентрованные
	virtual void Recognize(
		FA * pudata,
		TBipolarPulseVector &bipolar_pulse_vector
	){};
};

/************************************************/

typedef struct {
	unsigned long index; 
	bool up_level;				//Пересечение с отрицательным уровнем или с положительным
	bool ascend;				//Восходящее/нисходящее пересечение(в большую по модулю сторону)
} TIntersecPoint;

typedef pair <TIntersecPoint, TIntersecPoint> TIntersecPointPair;
typedef vector <TIntersecPointPair> TPairVector;

static const TIntersecPoint EMPTY_INTERSEC_POINT = {0,false,false};
static const TIntersecPointPair EMPTY_POINT_PAIR = make_pair(EMPTY_INTERSEC_POINT, EMPTY_INTERSEC_POINT);

//Сравнение пересечений
inline bool operator == (const TIntersecPoint &p1, const TIntersecPoint &p2)
{
	if (p1.ascend != p2.ascend) return false;
	if (p1.index != p2.index) return false;
	if (p1.up_level != p2.up_level) return false;
	return true;
};

//Функция добавления пересечения в массив пар пересечений
inline void AddIntersection(TPairVector &pair_vector, const unsigned long index, bool up_level, bool ascend);

//Аппроксимирующая функция s(t) = A*sin(t)
inline void ApproximatingFunction (FA * X, FA * params, FA * f, FA * pder)
{
	fp A,/*B,*/T,X0;
	int N = X->size(), NP = 1;
  FA mx(N), pd(N); 

	(*pder).resize(NP*N);
	(*f).resize(N);
	A = (*params)[0];
	//B = (*params)[1];
	X0 = (*X)[0];
	T = (*X)[N-1] - X0;

	mx = ((*X)-X0) - T/2.;
	pd = sin(mx/T * 2. *pi);
	*f = A * pd/*+B*/;

	//Частные производные по параметрам
	(*pder)[slice(0,N,1)] = pd;
	//(*pder)[slice(N,N,1)] = 1.;
};

//класс, реализующий метод выделения события из фона и определения основных характеристик
//акустического импульса по критерию аппроксимируемости функции
class TApproximabilityEventRecognizer : public TBaseEventRecognizer {

public:
	TApproximabilityEventRecognizer(const bool write_debug_info);
	~TApproximabilityEventRecognizer();

	//Вычисляем статистические характеристики ряда
	void CalculateStatisticCharacteristics (
		FA * pudata,			//Входные данные звуковая дорожка с АЦП, приведенная к типу FA
		FA * pdata,				//Отцентрованная дорожка
		fp * pmean,							//Среднее ряда
		fp * pstddev						//Среднеквадратичное отклонение ряда
	);

private:
	//Собираем пары точек пересечения с уровнями +/-SigmaLevel*PStdDev в один проход
	TPairVector * GetPairVector(
		FA * pdata,
		const fp stddev,
		const fp sigma_level
	);

	//Вырезаем кусок данных длиной MAX_D/ADC_UNIT_TIME, предположительно содержащий биполярный импульс
	FA * GetMaxDLengthSlice(
		FA * pdata, 
		int pulse_begin_index, 
		int pulse_end_index,
		int & d
	);

	//Минимизируем величину ошибки, определяем характеристики импульса
	TBipolarPulse GetBipolarPulse(
		FA * max_d_length_slice,
		const fp stddev,
		const fp norm_chi_square_threshold
	);

	//возвращает вектор из биполярных импульсов 
	//(их координат и характеристик) на данном отрезке
	TBipolarPulseVector GetBipolarPulseVector(
		FA * pdata,
		const fp stddev,
		TPairVector * pair_vector
	);

public:
	//Только для визуализации:
	string * debuginfo;

	//Сюда можно подавать данные как знаковые, так и беззнаковые, как отцентрованные, так и не отцентрованные
	void Recognize(		
		FA * pudata,
		TBipolarPulseVector &bipolar_pulse_vector
	); //Перекрываем родительский метод
};

/************************************************/

class TFourierEventRecognizer : public TBaseEventRecognizer {
public:
	TFourierEventRecognizer(const bool write_debug_info);
};

/************************************************/

class TWaveletEventRecognizer : public TBaseEventRecognizer {
public:
	TWaveletEventRecognizer(const bool write_debug_info);
};

/************************************************/

class TWaveletWithNeuroEventRecognizer : public TBaseEventRecognizer {
public:
	TWaveletWithNeuroEventRecognizer(const bool write_debug_info);
};

/************************************************/

// Фабрика производных от класса TBaseEventRecognizer объектов
class TGetNewEventRecognizer {
public:
	TBaseEventRecognizer * operator () (TenumRecognitionMethod recognition_method, const bool write_debug_info)
	{
		switch (recognition_method)
		{
			case irmApproximability:
				return new TApproximabilityEventRecognizer(write_debug_info);
			case irmFourier:
				return new TFourierEventRecognizer(write_debug_info);
			case irmWavelet:
				return new TWaveletEventRecognizer(write_debug_info);
			case irmWaveletWithNeuro:
				return new TWaveletWithNeuroEventRecognizer(write_debug_info);
			default : throw(TException("Incorrect event recognition method!"));
		};
		return 0;
	};
};

#endif
