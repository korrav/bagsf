/*
 * mad_com.cpp
 *
 *  Created on: 21.05.2013
 *      Author: andrej
 */
#include "Mad.h"

namespace mad_n {
void Mad::comChangeGain(bool isBrod, int * gain) {
	int command[5] = { COM_SET_GAIN };
	for (int i = 1; i < 5; i++)
		command[i] = gain[i - 1];
	if (isBrod)
		sendto(__sockCon, reinterpret_cast<void*>(command), sizeof(command), 0,
				reinterpret_cast<sockaddr*>(&__madAddr_brod),
				sizeof(__madAddr_brod));
	else
		sendto(__sockCon, reinterpret_cast<void*>(command), sizeof(command), 0,
				reinterpret_cast<sockaddr*>(&__madAddr), sizeof(__madAddr));
}

void Mad::comChangeNoise(bool isBrod, int& noise) {
	int command[2] = { COM_SET_NOISE, noise };
	if (isBrod)
		sendto(__sockCon, reinterpret_cast<void*>(command), sizeof(command), 0,
				reinterpret_cast<sockaddr*>(&__madAddr_brod),
				sizeof(__madAddr_brod));
	else
		sendto(__sockCon, reinterpret_cast<void*>(command), sizeof(command), 0,
				reinterpret_cast<sockaddr*>(&__madAddr), sizeof(__madAddr));
}

void Mad::comChangeMode(bool isBrod, int& mode) {
	//закрытие предыдущего режима
	if (__mode == ALGORITHM1)
		__alg1.close();
	//открытие нового режима
	__mode = mode;
	if (__mode == ALGORITHM1) {
		__alg1.open();
		__isEnableTrans = true;
		mode = CONTINUOUS;
	}
	//передача команды
	int command[2] = { COM_SET_MODE, mode };
	if (isBrod)
		sendto(__sockCon, reinterpret_cast<void*>(command), sizeof(command), 0,
				reinterpret_cast<sockaddr*>(&__madAddr_brod),
				sizeof(__madAddr_brod));
	else
		sendto(__sockCon, reinterpret_cast<void*>(command), sizeof(command), 0,
				reinterpret_cast<sockaddr*>(&__madAddr), sizeof(__madAddr));

}

void Mad::comGetStatus(bool isBrod) {
	int command = COM_GET_STATUS_MAD;
	if (isBrod)
		sendto(__sockCon, reinterpret_cast<void*>(&command), sizeof(command), 0,
				reinterpret_cast<sockaddr*>(&__madAddr_brod),
				sizeof(__madAddr_brod));
	else
		sendto(__sockCon, reinterpret_cast<void*>(&command), sizeof(command), 0,
				reinterpret_cast<sockaddr*>(&__madAddr), sizeof(__madAddr));
}

}/* namespace mad_n */

