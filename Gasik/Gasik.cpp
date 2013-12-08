/*
 * Gasik.cpp
 *
 *  Created on: 24 нояб. 2013 г.
 *      Author: andrej
 */

#include "Gasik.h"

namespace mad_n {

Gasik::Gasik(void (*trans)(void *buf, size_t size), unsigned const& id) :
		ptrans(trans), isRunThread__(false) {
}

Gasik::~Gasik() {
	close();
	mut__.unlock();
}
void Gasik::pass(const int* buf, unsigned size) {
	if (isRunThread__ && size > sizeof(DataUnitPlusTime) / sizeof(int)) {
		int* lbuf = new int[size];
		std::copy(buf, buf + size, lbuf);
		mut__.lock();
		fifo__.push_back(std::unique_ptr<int[]>(lbuf));
		mut__.unlock();
	}
	return;
}
void Gasik::open(void) {
	isRunThread__ = true;
	end__ = std::async(std::launch::async, &Gasik::algorithm, this);
}

void Gasik::close(void) {
	if (isRunThread__) {
		mut__.lock();
		isRunThread__ = false;
		mut__.unlock();
		end__.get();
	}
}

Gasik::Gasik(Gasik&& a) {
	this->fifo__ = std::move(a.fifo__);
	this->isRunThread__ = a.isRunThread__;
	this->ptrans = a.ptrans;
	return;
}

void Gasik::algorithm(void) {
	for (;;) {
		if (!isRunThread__)
			break;
		mut__.lock();
		if (fifo__.empty()) {
			mut__.unlock();
			continue;
		}
		DataUnitPlusTime* data = reinterpret_cast<DataUnitPlusTime*>(fifo__.front().get());
		ptrans(&fifo__.front(),
				sizeof(DataUnitPlusTime) + data->amountCount * 4 * sizeof(int));
		fifo__.pop_front();
		mut__.unlock();
	}
	fifo__.clear();
}

} /* namespace mad_n */
