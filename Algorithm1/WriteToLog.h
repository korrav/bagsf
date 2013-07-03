#ifndef WRITE_TO_LOG_H
#define WRITE_TO_LOG_H

//Нужно установить одну директиву из SHOW_USING_QT_DIALOG, SHOW_USING_CERR, WRITE_LOG_IN_FILE

//Использование:	
//  throw exception("Exception text");
	
//  try {
//	  ...
//  } catch (exception &e) {
//	  WriteToLog(e.what());
//		//throw e;
//  };

//Основная концепции исключений C++: «деструкторы локальных объектов вызываются всегда, 
//независимо от способа возврата из функции (с помощью return или в связи с выбросом исключения)».

using std::string;
using std::exception;

class TException: public exception {
protected:
  string Text;
public:
  TException(const string &text)
  {
	  Text = text;
  };
  
  ~TException() throw(){};

  virtual const char* what()
  {
    return Text.c_str();
  }
};

//Отображаем текст исключения, используя диалог
#ifdef SHOW_USING_QT_DIALOG
#include <QObject>
#include <QMessageBox>
#endif

//Выводим текст исключения в поток
#ifdef SHOW_USING_CERR
#include <iostream>
using std::cerr;
#endif

//Записываем в лог-файл
#ifdef WRITE_LOG_TO_FILE
#endif

#include <stdexcept>
#include <string>

inline void WriteToLog(const char * text){	
	//Отображаем текст исключения, используя диалог
	#ifdef SHOW_USING_QT_DIALOG
	QMessageBox::critical(	0, QObject::tr("Ошибка"),	//warning
													QObject::tr(text),
													QMessageBox::Ok);					//QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
	#endif

	//Выводим текст исключения в поток
	#ifdef SHOW_USING_CERR
	cerr<<text<<std::endl;
	#endif

	//Записываем в лог-файл
	#ifdef WRITE_LOG_IN_FILE
	#endif
};

#endif// WRITE_TO_LOG_H
