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

namespace center_n {

/*
 *
 */
class Center {
	//ЗАКРЫТЫЕ ЧЛЕНЫ
	static int __buf[20];	//буфер принятых данных
	static int __sock;	//сокет обмена информацией с БЦ
	static sockaddr_in __centerAddr;	//адрес БЦ
	static std::mutex __mut;//мьютекс, используемый для защиты сокета передачи в береговой центр
public:
	//ОТКРЫТЫЕ ФУНКЦИИ
	static void trans(void* buf, size_t size);	//передача пакета на БЦ
	static int receipt(void* buf, size_t* size);	//приём данных от БЦ
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
