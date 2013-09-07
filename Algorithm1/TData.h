/************************************************/
//TData.h

//Содержит классы для загрузки данных из файлов, содержащих двоичные данные (TReadBinaryFileData) и 
//данные с разделителем " " (comma separated) (TReadCVSFileData)
//Функции перевода данных из контейнера в буффер (VA2DATA) и обратно (DATA2VA)
//Класс для поиска файлов и последовательного доступа к ним TDataLoader
/************************************************/

//Example TReadCVSFileData:
//
//vector <Double_t>  * vh = new vector <Double_t>;
//vector <Double_t>  * vBT = new vector <Double_t>;
//TReadCVSFileData *FD = new TReadCVSFileData("Baikal.CSV");
//(*FD)(2,vh,vBT);

//Example TReadBinaryFileData:
//
//VA * Impulse = new VA;
//unsigned long brick = 0; //Обязательно передавать инициализированные переменные!!!
//TReadBinaryFileData * TestReadBinary = new TReadBinaryFileData("D:/Data/sinhro_old_compare/1148914969.sin");
//
////Загружаем данные из файла
//(*TestReadBinary)(Impulse,brick);

#ifndef TDATA_H
#define TDATA_H

//#include "RIOStream.h"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <stdarg.h> //vararg - unix
#include <algorithm>
#include <vector>
#include <string>
#include <valarray>
#include <list>
#include <complex>

#ifdef __unix__
#include <cstring>
#endif

#ifdef _WIN32
#include "io.h"
#endif

//#include "TRootApp.h"
using namespace std;

//TDataContainer...[], size(), resize()
//TFileData...

typedef double fp;
typedef complex <fp> TCmplx;

#define TN 4							//количество дорожек в файле данных
#define TConst 0.000005	//Дискретизация по времени

typedef vector <fp> TDataVector;
typedef TDataVector * PDataVector;
typedef valarray <unsigned long> VA;
typedef valarray <long> SA;
typedef valarray <fp> FA;
typedef valarray <TCmplx> CA;

enum TEnumFileDataType {idfBinary, idfCSV};
//enum TEnumOperation {ioStream, ioBuffered};
static const fp pi = 3.1415926535897932384626433832795;

/************************************************/
//Базовый класс чтения из файла
class TBaseReadFileData {
protected:
	char * FileName;
public:
	TBaseReadFileData(const char * FileName)	{ SetFileName(FileName);};
	virtual ~TBaseReadFileData() {};

	void SetFileName( const char * FileName) { this->FileName = new char [strlen(FileName)]; strcpy( this->FileName, FileName);};

	virtual void operator () (const int NV, ...) {}; 
};

#ifdef _WIN32
/************************************************/
//чтение данных с разделителем " " (comma separated)
class TReadCVSFileData : public TBaseReadFileData {
public:
	TReadCVSFileData( const char * FileName ):TBaseReadFileData(FileName) {};
	//NV - количество векторов vector <Double_t> *, далее их список через запятую
	void operator () (const int NV, ...); 
};


/************************************************/
//данные из файлов, содержащих двоичную информацию
class TReadBinaryFileData : public TBaseReadFileData {
public:
	TReadBinaryFileData( const char * FileName ):TBaseReadFileData(FileName) {};
	
	//От Data требуется Resize(), []
	template <class TDataContainer, class TDataBrick>
		void operator () (TDataContainer & DataContainer, TDataBrick DataBrick)
			{
				filebuf FBuffer;
				istream InStream(&FBuffer);
				unsigned long BufSize;
				TDataBrick *PBuffer;
				try {
					FBuffer.open(FileName, ios::in|ios::binary|ios::ate);//ios::out|ios::trunc); 

					BufSize = InStream.tellg();
					int SizeOfArray = BufSize/sizeof(TDataBrick);
					
					//Читаю файл
					PBuffer = new TDataBrick[SizeOfArray];  
					InStream.seekg(0);
					InStream.read((char *) PBuffer, BufSize);
					DataContainer->resize(SizeOfArray);
					
					for (int i = 0; i < SizeOfArray; i++) (*DataContainer)[i] = *(PBuffer+i);
				} catch (exception &e) {
					delete [] PBuffer;
					InStream.clear(); //сброс флагов eofbit и failbit
					FBuffer.close(); //закрыть файл
					throw (exception ("TReadBinaryFileData  :: Error while reading file!"));
				};
				delete [] PBuffer;
				InStream.clear(); //сброс флагов eofbit и failbit
				FBuffer.close(); //закрыть файл
			}; 
};
#endif

///************************************************/
//// Фабрика производных от класса TBaseReadFileData объектов
//class TGetNewReadFileData
//{
//	public:
//		TBaseReadFileData * operator () ( char * FileName, TEnumFileDataType FileDataType)
//		{
//			switch (FileDataType)
//			{
//				case idfBinary:
//					return new TReadBinaryFileData(FileName);
//				case idfCSV:
//					return new TReadCVSFileData(FileName);
//			};
//		};
//};

/************************************************/
//Здесь можно сделать через интервал {() }()...

//Перемещение данных из массива значений в буфер
//Длина передается в size(), под буффер уже выделена память
template <class TDataContainer, class TDataBrick>
	inline void VA2DATA(const TDataContainer &container, TDataBrick * buffer) 
	{
		int idv, 
				sizeof_array = container->size();
		for (idv = 0; idv < sizeof_array; idv++)  buffer[idv] = (*container)[idv];
	};	// of VA2DATA

//Перемещение данных из буфера в контейнер
//Под буфер уже выделена память
template <class TDataContainer, class TDataBrick>
	inline void DATA2VA(const TDataBrick * buffer, const unsigned long sizeof_array, TDataContainer * container) 
	{
		int idv;
		if (container->size() != sizeof_array) container->resize(sizeof_array);
		for (idv = 0; idv < sizeof_array; idv++) (*container)[idv] = *(buffer + idv);
	};	// of DATA2VA

//Перемещение данных из комплексного массива значений в два буфера
//Длина передается в size(), под буферы уже выделена память
template <class TComplexContainer, class TDataBrick>
	inline void CA2DATA(const TComplexContainer &container, TDataBrick * re_buffer, TDataBrick * im_buffer) 
	{
		int idv, sizeof_array = container->size();
		for (idv = 0; idv < sizeof_array; idv++) 
		{
			re_buffer[idv] = (*container)[idv].real();
			im_buffer[idv] = (*container)[idv].imag();
		};
	};	// of VA2DATA

/************************************************/

#ifdef _WIN32
//Класс для поиска файлов и последовательного доступа к ним
class TDataLoader : public list <string> {
protected:			
	void GetSubDirs( const char * PDataDirS, list <string> * PDataDirList );
	void GetFileList( const list <string> * PDataDirVector, const char * PFileMask );			
public:
	//constructor
	TDataLoader( const char * PDataDir, const char * PFileMask, const bool SearchSubDirs );
	//destructor
	~TDataLoader() {};
};
#endif

/************************************************/

#endif
