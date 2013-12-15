/*
 * Center.cpp
 *
 *  Created on: 08.05.2013
 *      Author: andrej
 */

#include "Center.h"
#include "Mad.h"

#define MAX_SIZE_SAMPL_SEND 4100 //определяет длину буфера передачи сокета
namespace center_n {

int Center::__sock = -1;
unsigned int Center::__lenRecCom = 0;
sockaddr_in Center::__centerAddr;
std::mutex Center::__mut;

Center::Center(char *cip, unsigned int p) {
	__stat_buf.identif = CONTROL_CENTER;
	//инициализация управляющего канала БЦ
	bzero(&__centerAddr, sizeof(__centerAddr));
	__centerAddr.sin_family = AF_INET;
	__centerAddr.sin_port = htons(p);
	inet_pton(AF_INET, cip, &__centerAddr.sin_addr);
	sockaddr_in addr;
	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(p);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	//создание сокета
	if (__sock == -1) {
		__sock = socket(AF_INET, SOCK_DGRAM, 0);
		if (__sock == -1) {
			std::cerr << "socket for Center not create\n";
			exit(1);
		}
		if (bind(__sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr))) {
			std::cerr << "socket for Center not bind\n";
			exit(1);
		}
		int sizeSend = 2 * MAX_SIZE_SAMPL_SEND * 4 * sizeof(int);
		if (setsockopt(__sock, SOL_SOCKET, SO_SNDBUF, &sizeSend, sizeof(int))
				== -1) {
			std::cerr
					<< "Не поддерживается объём буфера передачи сокета Center в размере "
					<< sizeSend << " байт\n";
			exit(1);
		}
	}
	return;
}

void Center::trans(void* buf, size_t size) {
	std::lock_guard<std::mutex> guard(__mut);
	sendto(__sock, buf, size, 0, reinterpret_cast<sockaddr*>(&__centerAddr),
			sizeof(__centerAddr));
	return;
}

void Center::receipt() {
	__lenRecCom = recvfrom(__sock, __stat_buf.buf,
			sizeof(__stat_buf.buf) - sizeof(int), 0,
			NULL, NULL);
	if (__lenRecCom < 2 * sizeof(int))
		return;
	switch (__stat_buf.com.command) {
	case TO_BEG_START_GASIC:
		if (__stat_buf.com.dest == MADS) {
			for (auto & mad : __mads)
				mad.second->comChangeModeGasikInit();
			sendAnswer(YES);
		}
		break;
	case TO_BEG_STOP_GASIC:
		if (__stat_buf.com.dest == MADS) {
			for (auto & mad : __mads)
				mad.second->comChangeMode(false, mad_n::Mad::PREVIOUS);
			sendAnswer(YES);
		}
		break;
	default:
		std::cerr << "Принята неизвестная команда от Берегового центра";
		return;
	}
}

Center::Center(const Center& a) {
	this->__centerAddr = a.__centerAddr;
	return;
}

void Center::addMad(const unsigned int& id, mad_n::Mad* mad) {
	if (!__mads.insert(std::make_pair(id, mad)).second)
		std::cerr << "Попытка вставить объект Мад с идентификатором " << id
				<< " в контейнер отслеживаемых "
						" Мадов объектом берегового центра потерпела крах"
				<< std::endl;
	return;
}

void Center::remMad(const unsigned int& id) {
	__mads.erase(id);
	return;
}

void Center::sendAnswer(const enum resultCommand& result) {
	__stat_buf.buf[__lenRecCom / sizeof(int)] = result;
	sendto(__sock, reinterpret_cast<void*>(&__stat_buf),
			__lenRecCom + 2 * sizeof(int), 0,
			reinterpret_cast<sockaddr*>(&__centerAddr), sizeof(__centerAddr));
}

Center::~Center() {
	close(__sock);
	return;
}

} /* namespace center_n */
