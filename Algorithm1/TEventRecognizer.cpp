/************************************************/
//Delirium
/************************************************/

#include "TEventRecognizer.h"

/************************************************/
//Триггерная функция для отдельной ячейки детектора, возвращает true, если
//собирается по одному импульсу из каждого канала в пределах временного окна
//Действует по принципу "жадного" алгоритма, может работать неадекватно при 
//большой плотности импульсов
bool CellTrigger(
				vector <TBipolarPulseVector *> &bipolar_pulse_vectors, 
				void * /*TBaseReconstructor * */reconstructor,
				void * /*TSourceParametersVector * */ source_parameters_vector
				)
{
	bool	result = false,
				complete = true,
				reconstruct = false;
	
	//Выходим из функции, если хотя бы в одном из каналов нет импульса	
	int i, 
			channel_number = bipolar_pulse_vectors.size();
	for (i = 1; i < channel_number; i++)
		if (bipolar_pulse_vectors[i]->size() == 0) complete = false;
	if (!complete) return false;

	int	i_min,
			i_max,
			current_pos = 0,
			* indices = new int[channel_number];
	TBipolarPulse * pulses = new TBipolarPulse[channel_number];
	//Инициализируем нулями
	memset(indices, 0, channel_number * sizeof(int));
	memset(pulses, 0, channel_number * sizeof(TBipolarPulse));
	#ifdef RECONSTRUCTION_H
	reconstruct = (reconstructor != 0) && (source_parameters_vector != 0);
	TSourceParameters * temp_source_parameters;
	#else
	if ((reconstructor != 0) || (source_parameters_vector != 0))
		throw TException("Incorrect CellTrigger parameter!");
	#endif

	try {
		//цикл до тех пор пока в каналах есть импульсы
		do {
			//Действуем "жадным образом" выбираем ближайщий импульс, не последний в своем канале
			i_min = -1;
			current_pos = 0x0FFFFFFF;
			for (i = 0; i < channel_number; i++)
				if (
					(indices[i] < int(bipolar_pulse_vectors[i]->size()))	&&
					((*bipolar_pulse_vectors[i])[indices[i]].begin < current_pos)  						
				)	{
					i_min = i;
					current_pos = (*bipolar_pulse_vectors[i_min])[indices[i_min]].begin;
				};
			
			if (i_min != -1) //Если импульсы в каналах не закончились
			{
				//добавляем его в позицию массива pulses, соответствующую каналу 
				pulses[i_min] = (*bipolar_pulse_vectors[i_min])[indices[i_min]];
				
				indices[i_min]++;	//Инкрементируем счетчик канала

				//Проверяем "комплектность" массива импульсов и принадлежность временному окну
				complete = true;
				i_min = pulses[0].begin;
				i_max = pulses[0].begin;
				for (i = 1; i < channel_number; i++)
				{
					if ((pulses[i].begin == 0) && (pulses[i].end == 0)) complete = false;
					if (pulses[i].begin < i_min) i_min = pulses[i].begin;
					if (pulses[i].begin > i_max) i_max = pulses[i].begin;
				};
				// если все ок, то выходим из цикла и процедуры
				if ((complete) && (i_max - i_min < TIME_WINDOW))
				{
					if (!reconstruct) {
						i_min = -1;
						result = true;
					} else {
						#ifdef RECONSTRUCTION_H
						temp_source_parameters = ((TBaseReconstructor *)(reconstructor))->GetSourceParameters(pulses);
						
						//Если ошибка невелика, значит параметры сигнала удовлетворяют геометрии антенны
						//Включаем в результат
/*						if ((temp_source_parameters->phi_accuracy <= ANGLE_RECONSTRUCTION_ACCURACY) &&
								(temp_source_parameters->thet_accuracy <= ANGLE_RECONSTRUCTION_ACCURACY)
								)*/	((TSourceParametersVector *)(source_parameters_vector))->push_back(temp_source_parameters);
						
						temp_source_parameters = 0;
						#endif
					};
				};
				//temp_source_params = Reconstructor->GetSourceParameters(pulses);
			}; 
		} while (i_min != -1);
	} catch (exception &e) { 
		delete [] indices;
		delete [] pulses;
		throw e;
	};
	delete [] indices;
	delete [] pulses;

	return result;
};

/************************************************/
//TBaseEventRecognizer
/************************************************/

/************************************************/
//TApproximabilityEventRecognizer
/************************************************/

//Процедура добавлении пересечения:
//Темп - последняя запись вектора
//если темп пуст, то добавляем пересечение в темп,
//если темп полон, то 
//	если пересечение обратно по знаку уровня, то заменяем темп,
//	если пересечение совпадает по знаку уровня то
//		если пересечение и темп обратны по восхождению, формируем пару, заменяем темп
//		если пересечение и темп одинаковы по восхождению, то заменяем темп
void AddIntersection(TPairVector &pair_vector, const unsigned long index, bool up_level, bool ascend)
{
	TIntersecPoint new_point;
	new_point.ascend = ascend;
	new_point.index = index;
	new_point.up_level = up_level;

	if (pair_vector.empty()) 
	{
		pair_vector.reserve(2);
		pair_vector.push_back(EMPTY_POINT_PAIR);
	};
	
	//Если темп не пуст И пересечение совпадпет по знаку уровня И обратно по восхождению, то добавляем пару
	if (	(!(pair_vector[pair_vector.size()-1].first == EMPTY_INTERSEC_POINT)) &&
				(pair_vector[pair_vector.size()-1].first.up_level == new_point.up_level) &&
				(pair_vector[pair_vector.size()-1].first.ascend != new_point.ascend)
			)
	{
		//новая пара
		pair_vector[pair_vector.size()-1].second = new_point;
		//выделение памяти с запасом
		if (pair_vector.size() == pair_vector.capacity()) pair_vector.reserve( 3*pair_vector.size()/2 );
		//Новый темп
		pair_vector.push_back(EMPTY_POINT_PAIR);
	};	
	//В любом случае заменяем темп
	pair_vector[pair_vector.size()-1].first = new_point;
};

TApproximabilityEventRecognizer :: TApproximabilityEventRecognizer(const bool write_debug_info) : TBaseEventRecognizer(write_debug_info)
{
	//WriteDebugInfo = write_debug_info; в базовом классе
	if (WriteDebugInfo)	debuginfo = new string;
};

TApproximabilityEventRecognizer :: ~TApproximabilityEventRecognizer()
{
	if (WriteDebugInfo)	delete debuginfo;
};

//Вычисляем статистические характеристики ряда
void TApproximabilityEventRecognizer :: CalculateStatisticCharacteristics (
	FA * pudata,				//Входные данные звуковая дорожка с АЦП, приведенная к типу Vector
	FA * pdata,					//Отцентрованная дорожка
	fp * pmean,							//Среднее ряда
	fp * pstddev)						//Среднеквадратичное отклонение ряда
{
	if (pudata->size() == 0)
		throw TException("CalculateStatisticCharacteristics :: No input data!");
	//Вычисляем статистическое среднее
	*pmean = pudata->sum()/pudata->size();
	//Центрируем
	pdata->resize(pudata->size());
	*pdata = *pudata - *pmean;
	//Вычисляем среднеквадратичное отклонение
	*pstddev = sqrt( pow(*pdata,2.).sum()/pdata->size() );
};

//Собираем пары точек пересечения с уровнями +/-SigmaLevel*PStdDev в один проход
TPairVector * TApproximabilityEventRecognizer :: GetPairVector(
	FA * pdata,
	const fp stddev,
	const fp sigma_level)
{
	int i;
	//char buffer[_CVTBUFSIZE];
	TPairVector * result = new TPairVector;

	fp Level = sigma_level*stddev, temp;
	//if (WriteDebugInfo) {
	//_gcvt( Level, 12, buffer ); 
	//*debuginfo+=string("Level ")+buffer+string("<BR>");
	//};

	//Собираем пары(в один проход):
	//Движемся по дорожке ищем точки превышающие предел по модулю. Наткнувшись на такую смотрим, 
	//если предыдущая точка меньше по модулю 
	//то добавить возрастающее пересечение со знаком уровня, соотв знаку значения точки
	//если также последующая точка меньше по модулю 
	//то также добавить убывающее пересечение со знаком уровня, соотв знаку значения точки
	//Обрабаты
	
	for (i = 1; i <= int(pdata->size()-2); i++)
	{
		temp = (*pdata)[i];
		if (abs(temp) < Level) continue;
		if ((abs((*pdata)[i-1]) < Level) || ( ((*pdata)[i-1] >= 0) != (temp >= 0)))
		{
			AddIntersection(*result, i-1, temp > 0, true);
		};
		if ((abs((*pdata)[i+1]) < Level) || ( ((*pdata)[i+1] >= 0) != (temp >= 0)))
		{
			AddIntersection(*result, i, temp > 0, false);
		};
	};
	//Очищаем последнюю темповую запись
  if (result->size() != 0)
  {
    TPairVector::iterator pvi = result->end();
	  result->erase(--pvi);
  };

	return result;
};

//Вырезаем кусок данных длиной MAX_D/ADC_UNIT_TIME, предположительно содержащий биполярный импульс
FA * TApproximabilityEventRecognizer :: GetMaxDLengthSlice(
	FA * pdata, 
	int pulse_begin_index, 
	int pulse_end_index,
	int & d
) {
	FA * result = 0;
	//Количество отчетов, соответствующее максимальной длине импульса
	int max_d_length = MAX_D/ADC_UNIT_TIME/1e6;
	//Количество отчетов, которое будет нарощено с каждой стороны интервала
	d = (max_d_length - (pulse_end_index - pulse_begin_index)) / 2;
	//Слишком длинный импульс
	if (d < 0) return 0;
	//Импульс выходит за пределы данных
	if ((pulse_begin_index < d) || ((pulse_end_index + d) > pdata->size())) return 0;
	
	result = new FA(max_d_length);
	//Выбираем max_d_length число эл-ов, начиная с pulse_begin_index, находящихся на расстоянии 1
	*result = (*pdata)[std::slice(pulse_begin_index - d, max_d_length, 1)];

#ifdef _WIN32
	if (WriteDebugInfo) {
		char buffer[_CVTBUFSIZE];
		itoa(pulse_begin_index - d,buffer,10);
		*debuginfo+="cutting from "+string(buffer);
		itoa(pulse_begin_index - d+max_d_length,buffer,10);
		*debuginfo+=" to "+string(buffer)+"<BR>";
	};
#endif


	return result;
};

//Передвигая по отрезку max_d_length_slice нашу синусойду и масштабируя ее по высоте(коэффициент в приближении)
//и по длительности в пределах возможных длительностей (пробуем разную ширину окна)
//минимизируем сумму (квадратов) среднеквадратичных отклонений отнесенную к (квадрату) количеству точек
TBipolarPulse TApproximabilityEventRecognizer :: GetBipolarPulse(
	FA * max_d_length_slice,
	const fp stddev,
	const fp norm_chi_square_threshold
) {
	TPairVector * temp_pair_vector;
	TBipolarPulse result = EmptyBipolarPulse;
	fp norm_chi_square, min_norm_chi_square = 0, chisq;
	int max_d_length = MAX_D/ADC_UNIT_TIME/1e6, 
			i, l, delta;
	FA	x, y, weights, params, y_fit;
#ifdef _WIN32
	char buffer[_CVTBUFSIZE];
#endif

	params.resize(1);

	//оптимальнее минимизировать chisq методом минимзации, добавив в нашу функцию параметры delta и l
	//а не тупо перебирать все длительности и сдвиги
	l = max_d_length;
	while (l >= (MIN_D/ADC_UNIT_TIME/1e6)) 
	{
		x.resize(l);
		y.resize(l);
		weights.resize(l);

		for (delta = 0; delta <= (max_d_length - l); delta+=2) //Положений на 1 больше, чем разность длин
		{
			params = 1.;
			weights = 1.;
			for (i = 0; i < l; i++) x[i] = fp(i);
			y = FA((*max_d_length_slice)[slice(delta, l, 1)]);

			y_fit = Fit(&x, &y, &weights, &params, &chisq, ApproximatingFunction);

			//Будем рассматривать величину chisq (сумма квадратов разностей точек приближения и приближаемой функции, отнесенная к количеству точек)
			//Нормированную на средний квадрат приближаемой функции (чтобы избавится от зависимости от амплитуды)
			norm_chi_square =	chisq/(FA(pow(y,2.)).sum()/fp(y.size()));
			
			if (((min_norm_chi_square == 0) || (norm_chi_square < min_norm_chi_square)) && (norm_chi_square < norm_chi_square_threshold))
			{
				//линейно интерполируем по двум ближайшим точкам пересечения x = x0 + (x1-x0) * (y-y0) / (y1-y0)
				//Сначала находим пересечения с уровнем сигма (по нашему опрделению длительности)
				temp_pair_vector = GetPairVector(&y_fit, stddev, 1.0);

#ifdef _WIN32
				if (WriteDebugInfo)
				{
					*debuginfo+="<BR>norm_chi_square ";
					_gcvt( norm_chi_square, 12, buffer );
					*debuginfo+=string(buffer);

					*debuginfo+="y";
					for (i = 0; i < y.size(); i++) 
					{
						_gcvt( y[i], 12, buffer );
						*debuginfo+=string(buffer)+" ";
					};
					*debuginfo+="<BR>";

					*debuginfo+="y_fit";
					for (i = 0; i < y_fit.size(); i++) 
					{
						_gcvt( y_fit[i], 12, buffer );
						*debuginfo+=string(buffer)+" ";
					};
					*debuginfo+="<BR>";
				}
#endif

				if ((temp_pair_vector->size() == 2) && ((min_norm_chi_square == 0) || (norm_chi_square < min_norm_chi_square)))
				{
					min_norm_chi_square = norm_chi_square;	//Ищем минимум!

					result.begin =	delta;
					result.end = delta + y_fit.size();
					result.amplitude = y_fit.max() - y_fit.min();
					result.duration = result.end - result.begin;
				};

				//try {
				//	if (temp_pair_vector->size() != 2) throw(exception("Unexpected result!"));

				//} catch (exception &e) {
				//	delete temp_pair_vector;
				//	throw e;
				//};
				delete temp_pair_vector;
			};
		};
		l -= 1;
	};

#ifdef _WIN32
	if (WriteDebugInfo)
	{
		*debuginfo+="min_norm_chi_square ";
		_gcvt( min_norm_chi_square, 12, buffer );
		*debuginfo+=string(buffer)+"<BR>";
	};
#endif

	if (min_norm_chi_square > norm_chi_square_threshold) 
		return  EmptyBipolarPulse;

	return result;
};


//возвращает вектор из биполярных импульсов 
//(их координат и характеристик) на данном отрезке
TBipolarPulseVector TApproximabilityEventRecognizer :: GetBipolarPulseVector(
	FA * pdata,	
	const fp stddev,
	TPairVector * pair_vector
) {	
	TBipolarPulseVector result;
	//result->reserve выделением памяти с запасом можно добиться ускорения
	TPairVector :: const_iterator vi;
	FA * max_d_length_slice;
	TBipolarPulse temp_pulse;

#ifdef _WIN32
	char buffer[_CVTBUFSIZE];
#endif
	
	//Пробегаем по вектору пар пересечений
	int i, d;
	if (pair_vector->size() < 2) return result;
	for (i = 0; i < int(pair_vector->size()-1); i++)
	{
#ifdef _WIN32
		if (WriteDebugInfo)
		{
			itoa((*pair_vector)[i].first.index,buffer,10);
			*debuginfo+=" (*pair_vector)[i].first.index "+string(buffer)+"<BR>";
			itoa((*pair_vector)[i+1].second.index,buffer,10);
			*debuginfo+=" (*pair_vector)[i+1].second.index "+string(buffer)+"<BR>";
		};
#endif

		max_d_length_slice = 0;
		//Если две ближайшие пары имеют разные знаки уровня
		if ((*pair_vector)[i].first.up_level != ((*pair_vector)[i+1].first.up_level))
		{
			d = 0;
			//Проверяем условие C*(W1+W2) > D. С учетом того, что если пересечение восходящее, то
			//хранится его первая точка, если нисходящее, то последняя
			if ( C * (((*pair_vector)[i].second.index - (*pair_vector)[i].first.index) + ((*pair_vector)[i+1].second.index - (*pair_vector)[i+1].first.index))
						 >= ((*pair_vector)[i+1].second.index - (*pair_vector)[i].first.index)
					) max_d_length_slice = GetMaxDLengthSlice(pdata, (*pair_vector)[i].first.index, (*pair_vector)[i+1].second.index, d);
					//Вырезаем кусок данных длиной MAX_D/ADC_UNIT_TIME, предположительно содержащий биполярный импульс
			
			if (max_d_length_slice)
			{
				if (!SimpleMethod) {
					//Минимизируем величину ошибки, определяем характеристики импульса
					temp_pulse = GetBipolarPulse(max_d_length_slice, stddev, NORM_CHI_SQUARE_THRESHOLD);
					//Определяем начало и конец импульса относительно начала трека и добавляем в вектор-результат 
					temp_pulse.begin = (*pair_vector)[i].first.index - d + temp_pulse.begin;
					temp_pulse.end = (*pair_vector)[i].first.index - d + temp_pulse.end;
				} else {
					//Считаем приближенно характеритики импульса, не ищем его в пределах отрезка
					temp_pulse.begin =	(*pair_vector)[i].first.index - 2;
					temp_pulse.end = (*pair_vector)[i+1].second.index + 2;
					temp_pulse.amplitude = max_d_length_slice->max() - max_d_length_slice->min();
					temp_pulse.duration = temp_pulse.end - temp_pulse.begin;
				};
				if (!temp_pulse.IsEmpty())
					if (temp_pulse.amplitude >= stddev * MIN_A) result.push_back(temp_pulse);
				delete max_d_length_slice;
				max_d_length_slice = 0;
			};

		};
	};

	return result;
};

//Сюда можно подавать данные как знаковые, так и беззнаковые, как отцентрованные, так и не отцентрованные
void TApproximabilityEventRecognizer :: Recognize(
	FA * pudata,
	TBipolarPulseVector &bipolar_pulse_vector
)		//Перекрываем родительский метод
{
#ifdef _WIN32
	char buffer[_CVTBUFSIZE];
	int i;
#endif

	FA data;
	fp mean, stddev;
	TPairVector * pair_vector = 0;
	
	//Вычисляем статистические характеристики ряда
	CalculateStatisticCharacteristics (pudata, &data, &mean, &stddev);
	//Собираем пары точек пересечения с уровнями +/-SigmaLevel*PStdDev в один проход	
	pair_vector = GetPairVector(&data, stddev, SIGMA_LEVEL);
	//возвращает вектор из биполярных импульсов 
	//(их координат и характеристик) на данном отрезке
	bipolar_pulse_vector = GetBipolarPulseVector(&data, stddev,	pair_vector);

#ifdef _WIN32
	if (WriteDebugInfo){
		*debuginfo+=string("pair_vector ")+string("<br>");
		for (i = 0; i < pair_vector->size(); i++)
		{
			itoa((*pair_vector)[i].first.ascend,buffer,10);
			*debuginfo+=string(buffer)+string(" ");
			itoa((*pair_vector)[i].first.index,buffer,10);
			*debuginfo+=string(buffer)+string(" ");
			itoa((*pair_vector)[i].first.up_level,buffer,10);
			*debuginfo+=string(buffer)+string("   ");
			itoa((*pair_vector)[i].second.ascend,buffer,10);
			*debuginfo+=string(buffer)+string(" ");
			itoa((*pair_vector)[i].second.index,buffer,10);
			*debuginfo+=string(buffer)+string(" ");
			itoa((*pair_vector)[i].second.up_level,buffer,10);
			*debuginfo+=string(buffer)+string("   ");
			*debuginfo+=string("<br>");
		};
		*debuginfo+=string("<br>");

		*debuginfo+=string("bipolar_pulse_vector ")+string("<br>");
		for (i = 0; i < bipolar_pulse_vector.size(); i++)	
		{
			_gcvt( bipolar_pulse_vector[i].begin, 12, buffer ); 
			*debuginfo+=string("begin ")+buffer+string("<BR>");
			_gcvt( bipolar_pulse_vector[i].end, 12, buffer ); 
			*debuginfo+=string("end ")+buffer+string("<BR>");
			_gcvt( bipolar_pulse_vector[i].amplitude, 12, buffer ); 
			*debuginfo+=string("amplitude ")+buffer+string("<BR>"); 
			_gcvt( bipolar_pulse_vector[i].duration, 12, buffer ); 
			*debuginfo+=string("duration ")+buffer+string("<BR>"); 
		};
		*debuginfo+=string("<br>");
	};
#endif
	
	//Не нужно освобождать, так как возвращаем вектор
	//for (i = 0; i < bipolar_pulse_vector->size(); i++) delete (*bipolar_pulse_vector)[i];
	//for (i = 0; i < pair_vector->size(); i++) delete (*pair_vector)[i];
	if (pair_vector) delete pair_vector;
};

//!!!РАЗНЕСТИ ПО МОДУЛЯМ!!!
/************************************************/
//TFourierEventRecognizer
/************************************************/
TFourierEventRecognizer :: TFourierEventRecognizer(const bool write_debug_info) : TBaseEventRecognizer(write_debug_info)
{
};

/************************************************/
//TWaveletEventRecognizer
/************************************************/
TWaveletEventRecognizer :: TWaveletEventRecognizer(const bool write_debug_info) : TBaseEventRecognizer(write_debug_info)
{
};

/************************************************/
//TWaveletWithNeuroEventRecognizer
/************************************************/
TWaveletWithNeuroEventRecognizer :: TWaveletWithNeuroEventRecognizer(const bool write_debug_info) : TBaseEventRecognizer(write_debug_info)
{
};
