/*
 * Center.cpp
 *
 *  Created on: 08.05.2013
 *      Author: andrej
 */

#include "Center.h"

namespace center_n {

int Center::__buf[20];
int Center::__sock = -1;
sockaddr_in Center::__centerAddr;
std::mutex Center::__mut;

Center::Center(char *cip, unsigned int p) {
	//инициализация управляющего канала БЦ
	bzero(&__centerAddr, sizeof(__centerAddr));
	__centerAddr.sin_family = AF_INET;
	__centerAddr.sin_port = htons(p);
	inet_pton(AF_INET, cip, &__centerAddr.sin_addr);
	sockaddr_in addr;
	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(p);
	addr.sin_addr.s_addr = htonl(INADDR_ANY );
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
	}
	return;
}

void Center::trans(void* buf, size_t size) {
	std::lock_guard<std::mutex> guard(__mut);
	sendto(__sock, buf, size, 0, reinterpret_cast<sockaddr*>(&__centerAddr),
			sizeof(__centerAddr));
	return;
}

int Center::receipt(void* buf, size_t* size) {
	*size = recvfrom(__sock, buf, *size, 0, NULL, NULL);
	return 1;
}

Center::Center(const Center& a) {
	this->__centerAddr = a.__centerAddr;
	return;
}

Center::~Center() {
	close(__sock);
	return;
}

} /* namespace center_n */
