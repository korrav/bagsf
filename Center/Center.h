/*
 * Center.h
 *
 *  Created on: 08.05.2013
 *      Author: andrej
 */

#ifndef CENTER_H_
#define CENTER_H_
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <strings.h>
#include <mutex>
#include <map>

#define CONTROL_CENTER 7	 //код, идентифицирующий блок данных сигнала Гасик

namespace mad_n {
class Mad;
}
namespace center_n {
/*
 *команды от БЦ имеют следующий формат
 *1)int приниматель (-1 - БЭГ, 0 - все МАД БЭГ, 1,2,3.... - идентификатор МАД
 *2)int код команды
 *3)int...... - аргументы команды
 *
 *В ответе:
 *1)int префикс CONTROL_GASIC
 *2) - n-1) - код принятой команды
 *n)int результат выполнения команды: 0 - не выполнена, 1 - выполнена
 */
class Center {
	//ЗАКРЫТЫЕ ЧЛЕНЫ
	typedef std::map<unsigned int, mad_n::Mad*> MadMap;
	enum destination__ {
		BAG = -1, MADS
	}; //адресаты команды; БЭГ и все отслеживаемые Мад, соответственно
	enum idCommand__ {
		TO_BEG_STOP_GASIC, TO_BEG_START_GASIC
	}; //идентификаторы команд БЭГ
	enum resultCommand {
		NO = 0, YES
	}; // Результат выполнения команды
	struct {
		int identif;
		union {
			int buf[100];	//буфер принятых данных
			struct {
				int dest; //получатель
				int command; // идентификатор команды
			} com;
		};
	} __stat_buf;
	static int __sock;	//сокет обмена информацией с БЦ
	static sockaddr_in __centerAddr;	//адрес БЦ
	static std::mutex __mut;//мьютекс, используемый для защиты сокета передачи в береговой центр
	static unsigned int __lenRecCom; //длина принятой команды
	MadMap __mads; //ассоциативный контейнер, содержащий в качестве ключа номер мада, значения - указатель на объект Mad
	void sendAnswer(const enum resultCommand& result); // отправить ответ БЦ
public:
	//ОТКРЫТЫЕ ФУНКЦИИ
	void addMad(const unsigned int& id, mad_n::Mad* mad); // добавление отслеживаемого Мад
	void remMad(const unsigned int& id); // удаление Мад из области отслеживания
	static void trans(void* buf, size_t size);	//передача пакета на БЦ
	void receipt();	//приём данных от БЦ
	static int getSock(void);	//возвращает сокет БЦ
	Center(char *cip = "192.168.1.156", unsigned int p = 3011);
	Center(const Center&);
	virtual ~Center();
};

inline int Center::getSock(void) {
	return __sock;
}

} /* namespace center_n */
#endif /* CENTER_H_ */
