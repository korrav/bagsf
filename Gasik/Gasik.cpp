/*
 * Gasik.cpp
 *
 *  Created on: 24 нояб. 2013 г.
 *      Author: andrej
 */

#include "Gasik.h"
#include "Mad.h"

namespace mad_n {

Gasik::Gasik(void (*trans)(void *buf, size_t size)) :
		ptrans(trans), isRunThread__(false) {
}

Gasik::~Gasik() {
	close();
	mut__.unlock();
}
void Gasik::pass(void* buf, size_t& size) {
	if (isRunThread__ && size > sizeof(DataUnitPlusTime)) {
		int8_t* lbuf = new int8_t[size];
		std::copy(reinterpret_cast<int8_t*>(buf),
				reinterpret_cast<int8_t*>(buf) + size, lbuf);
		reinterpret_cast<DataUnitPlusTime*>(lbuf)->mode = mad_n::Mad::GASIK;
		mut__.lock();
		fifo__.push_back(std::unique_ptr<int8_t[]>(lbuf));
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
	a.close();
	this->fifo__ = std::move(a.fifo__);
	this->isRunThread__ = a.isRunThread__;
	this->ptrans = a.ptrans;
	return;
}

Gasik& Gasik::operator =(Gasik&& a) {
	if (this == &a)
		return *this;
	a.close();
	this->fifo__ = std::move(a.fifo__);
	this->isRunThread__ = a.isRunThread__;
	this->ptrans = a.ptrans;
	return *this;

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
		DataUnitPlusTime* data =
				reinterpret_cast<DataUnitPlusTime*>(fifo__.front().get());
		ptrans(&fifo__.front(),
				sizeof(DataUnitPlusTime) + data->amountCount * 4 * sizeof(int));
		fifo__.pop_front();
		mut__.unlock();
	}
	fifo__.clear();
}

} /* namespace mad_n */
