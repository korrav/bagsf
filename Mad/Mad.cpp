/*
 * Mad.cpp
 *
 *  Created on: 08.05.2013
 *      Author: andrej
 */

#include "Mad.h"
#include "Center.h"
#include <strings.h>
#include <cstring>
#include <signal.h>

#define  CONTROL_PORT_BAG 31002 //порт бэга, через который осуществляется взаимодействие с управляющим каналом мада
#define SYNC_PERIOD 300//период передачи пакетов сихронизации (в секундах)
#define MAX_COUNT_FREQ_OUT 10 //количество быстрых пакетов данных, по достижению которого отключается передача данных на БЦ
#define MIN_PER_OUT 1 //минимальный промежуток времени поступления входных данных без инкрементирования счётчика переполнения
namespace mad_n {
int Mad::__sockData = -1;
int Mad::__sockMon = -1;
int Mad::__sockCon = -1;
sockaddr_in Mad::__madAddr_brod;
int Mad::__buf[17000];
time_t Mad::__timeSync = static_cast<time_t>(0);
unsigned int Mad::__numMad = 0;
unsigned int Mad::__len = 0;
bool Mad::__enabMesMonit = false;
bool Mad::__enabMesData = false;
const int Mad::SIZE_P = -1;
center_n::Center* Mad::__pcenter = nullptr;

//СТРУКТУРА MONITOR
#define ID_MONITOR 4	//код, идентифицирующий блок монитор
struct Monitor {
	int ident; //идентификатор блока данных
	unsigned int id_MAD; //идентификатор МАДа
	int dispersion[4]; //величина дисперсии для каждого канала
	int math_ex[4]; //величина математического ожидания для каждого канала
	int num_sampl; //количество отсчётов, при котором производится анализ даннных
};

//ГЛОБАЛЬНАЯ СТРУКТУРА СОСТОЯНИЯ МОДУЛЯ МАД
struct status_MAD {
	int ident; //идентификатор блока данных (COM_GET_STATUS_MAD)
	int gain[4]; //текущий коэффициент усиления (в абсолютных значениях)
	int modeData_aq; //текущий режим сбора данных МАД
	int NoiseThreshold; //текущий шумовой порог алгоритма распознавания
	int wp; //количество отсчётов до события; используется в режиме DETECT1
	int wa; //количество отсчётов после события; используется в режиме DETECT1
};

void hand_SIGALRM(int);

Mad::Mad(unsigned int i, char* cip, unsigned int pD, unsigned int pM,
		unsigned int pC, bool isET) :
		__id(i), __curTimeOut(0), __countFreqOut(0), __isEnableTrans(isET), __alg1(
				&center_n::Center::trans, i), __gasik(&center_n::Center::trans,
				i) {
	if (__pcenter == nullptr) {
		std::cerr << "Класс Mad не инициализирован\n";
		exit(1);
	}
	__wFile.isWrite = false;
	__wFile.numSampl = 0;
	__wFile.count = 0;
	//инициализация управляющего канала мада
	bzero(&__madAddr, sizeof(__madAddr));
	__madAddr.sin_family = AF_INET;
	__madAddr.sin_port = htons(pC);
	inet_pton(AF_INET, cip, &__madAddr.sin_addr);
	//создание сокетов
	if (__sockData == -1) {
		__sockData = socket(AF_INET, SOCK_DGRAM, 0); //сокет приёма данных
		if (__sockData == -1) {
			std::cerr << "socket __sockData not create\n";
			exit(1);
		}
		sockaddr_in hostDataAdd;
		bzero(&hostDataAdd, sizeof(hostDataAdd));
		hostDataAdd.sin_family = AF_INET;
		hostDataAdd.sin_port = htons(pD);
		//inet_pton(AF_INET, "192.168.203.30", &hostDataAdd.sin_addr);
		hostDataAdd.sin_addr.s_addr = htonl(INADDR_ANY);
		if (bind(__sockData, reinterpret_cast<sockaddr*>(&hostDataAdd),
				sizeof(hostDataAdd))) {
			std::cerr << "socket __sockData not bind\n";
			exit(1);
		}
	}

	if (__sockMon == -1) {
		__sockMon = socket(AF_INET, SOCK_DGRAM, 0); //сокет приёма мониторограмм
		if (__sockMon == -1) {
			std::cerr << "socket __sockMon not create\n";
			exit(1);
		}
		sockaddr_in hostMonAdd;
		bzero(&hostMonAdd, sizeof(hostMonAdd));
		hostMonAdd.sin_family = AF_INET;
		hostMonAdd.sin_port = htons(pM);
		//inet_pton(AF_INET, "192.168.203.30", &hostMonAdd.sin_addr);
		hostMonAdd.sin_addr.s_addr = htonl(INADDR_ANY);
		if (bind(__sockMon, reinterpret_cast<sockaddr*>(&hostMonAdd),
				sizeof(hostMonAdd))) {
			std::cerr << "socket __sockMon not bind\n";
			exit(1);
		}
	}

	if (__sockCon == -1) {
		__sockCon = socket(AF_INET, SOCK_DGRAM, 0); //сокет управления
		if (__sockCon == -1) {
			std::cerr << "socket __sockCon not create\n";
			exit(1);
		}
		sockaddr_in hostConAdd;
		bzero(&hostConAdd, sizeof(hostConAdd));
		hostConAdd.sin_family = AF_INET;
		hostConAdd.sin_port = htons(CONTROL_PORT_BAG);
		//inet_pton(AF_INET, "192.168.203.30", &hostConAdd.sin_addr);
		hostConAdd.sin_addr.s_addr = htonl(INADDR_ANY);
		if (bind(__sockCon, reinterpret_cast<sockaddr*>(&hostConAdd),
				sizeof(hostConAdd))) {
			std::cerr << "socket __sockCon not bind\n";
			exit(1);
		}
		const int opt = 1;
		setsockopt(__sockCon, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt)); //включение поддержки широковещательной передачи
		//инициализация широковещательного адреса
		char add[16];
		std::strncpy(add, cip, 12);
		add[12] = '\0';
		std::strcat(add, "255");
		bzero(&__madAddr_brod, sizeof(__madAddr_brod));
		__madAddr_brod.sin_family = AF_INET;
		__madAddr_brod.sin_port = htons(pC);
		inet_pton(AF_INET, add, &__madAddr_brod.sin_addr);
		//инициализация обработчика сигнала SIGALRM
		signal(SIGALRM, hand_SIGALRM);
		//синхронизация мадов
		synchronization();
	}
	//изменение количества мадов в системе
	__numMad++;
	//временный кусок кода
	if (__id == 1)
		for (int &i : __gain)
			i = 41;
	else if (__id == 3) {
		__gain[0] = __gain[1] = __gain[3] = 51;
		__gain[2] = 41;
	}
	comChangeGain(false, __gain);
	__noise = 6;
	comChangeNoise(false, __noise);
	__alg1.open_follow();
	__mode = ALGORITHM1;
	comChangeMode(false, __mode);
	__pcenter->addMad(__id, this);
	return;
}

void Mad::synchronization(void) {
	/*sendto(__sockCon, 0, 0, 0, reinterpret_cast<sockaddr*>(&__madAddr_brod),
	 sizeof(__madAddr_brod));*/
	sockaddr_in mad1Addr, mad3Addr;
	bzero(&mad1Addr, sizeof(mad1Addr));
	mad1Addr.sin_family = AF_INET;
	mad1Addr.sin_port = htons(31000);
	inet_pton(AF_INET, "192.168.127.31", &mad1Addr.sin_addr);
	bzero(&mad3Addr, sizeof(mad3Addr));
	mad3Addr.sin_family = AF_INET;
	mad3Addr.sin_port = htons(31000);
	inet_pton(AF_INET, "192.168.127.33", &mad3Addr.sin_addr);
	sendto(__sockCon, 0, 0, 0, reinterpret_cast<sockaddr*>(&mad1Addr),
			sizeof(mad1Addr));
	sendto(__sockCon, 0, 0, 0, reinterpret_cast<sockaddr*>(&mad3Addr),
			sizeof(mad3Addr));
	time(&__timeSync);
	alarm(SYNC_PERIOD);
	printf("Отправлен синхроимпульс\n");
	return;
}

void receiptCon(Mad* pmad, int num) {
	sockaddr_in recAddr;
	size_t size = sizeof(recAddr);
	Mad::__len = recvfrom(Mad::__sockCon, reinterpret_cast<void *>(Mad::__buf),
			sizeof(Mad::__buf), 0, reinterpret_cast<sockaddr *>(&recAddr),
			&size);
	int id = -1;
	std::string reIp(inet_ntoa(recAddr.sin_addr));
	for (int i = 0; i < num; i++) {
		if (reIp == inet_ntoa(pmad[i].__madAddr.sin_addr)) {
			id = pmad[i].__id;
			break;
		}
	}
	if (id == -1 || Mad::__len < sizeof(int))
		return;
//декодирование сообщения
	switch (pmad->__buf[0]) {
	case Mad::COM_SET_GAIN:
		if (Mad::__len != 2 * sizeof(int))
			return;
		std::cout << "Мад " << id << ": команда изменить коэффициент усиления"
				<< (pmad->__buf[1] ? " успешно выполнена" : " потерпела неудачу")
				<< std::endl;
		break;
	case Mad::COM_SET_NOISE:
		if (Mad::__len != 2 * sizeof(int))
			return;
		std::cout << "Мад " << id << ": команда изменить шумовой коэффициент"
				<< (pmad->__buf[1] ? " успешно выполнена" : " потерпела неудачу")
				<< std::endl;
		break;
	case Mad::COM_GET_STATUS_MAD: {
		if (Mad::__len != sizeof(status_MAD))
			return;
		status_MAD* pbuf = reinterpret_cast<status_MAD*>(Mad::__buf);
		std::string smode;
		if (pmad[id - 1].__mode == Mad::ALGORITHM1) {
			if (pbuf->modeData_aq == Mad::CONTINUOUS)
				smode = "алгоритм 1";
			else
				smode = "несоотвествия с режимом БЭга";
		}
		if (pmad[id - 1].__mode == Mad::GASIK) {
			if (pbuf->modeData_aq == Mad::DETECTION1)
				smode = "Гасик";
			else
				smode = "несоотвествия с режимом БЭга";
		} else if (pbuf->modeData_aq != pmad[id - 1].__mode)
			smode = "несоотвествия с режимом БЭга";
		else if (pbuf->modeData_aq == Mad::CONTINUOUS)
			smode = "непрерывный";
		else if (pbuf->modeData_aq == Mad::DETECTION1)
			smode = "фильтрованный";
		else if (pbuf->modeData_aq == Mad::SILENCE)
			smode = "молчания";
		else
			smode = "неизвестный";
		std::cout << "Мад " << id << ": режим " << smode
				<< " коэффициенты усиления каналов: " << pbuf->gain[0] << " "
				<< pbuf->gain[1] << " " << pbuf->gain[2] << " " << pbuf->gain[3]
				<< " пороговый шум " << pbuf->NoiseThreshold
				<< "\nколичество отсчётов до события(DET1) " << pbuf->wp
				<< "\nколичество отсчётов после события(DET1) " << pbuf->wp
				<< std::endl;
	}
		break;
	case Mad::COM_SET_MODE:
		if (Mad::__len != 2 * sizeof(int))
			return;
		std::cout << "Мад " << id << ": команда изменить режим работы"
				<< (pmad->__buf[1] ? " успешно выполнена" : " потерпела неудачу")
				<< std::endl;
		break;
	case Mad::COM_SET_WPWA:
		if (Mad::__len != 2 * sizeof(int))
			return;
		std::cout << "Мад " << id
				<< ": команда изменить размерности пакета данных в режиме DETECTION1"
				<< (pmad->__buf[1] ? " успешно выполнена" : " потерпела неудачу")
				<< std::endl;
		break;
	}
	return;
}

void Mad::recM(void) {
	__pcenter->trans(__buf, __len);
	if (__enabMesMonit)
		std::cout << "Переданны мониторограммы в Центр\n";
	return;
}

void receiptMon(Mad* pmad) {
	Monitor* buf_m = reinterpret_cast<Monitor*>(Mad::__buf + 1);
	Mad::__len = recvfrom(Mad::__sockMon, reinterpret_cast<void *>(buf_m),
			sizeof(Monitor), 0, NULL, NULL);
	if (Mad::__len == sizeof(Monitor) && buf_m->ident == ID_MONITOR
			&& buf_m->id_MAD <= Mad::__numMad) {
		Mad::__buf[0] = static_cast<unsigned int>(time(NULL));
		Mad::__len += sizeof(int);
		std::cout << "мад " << buf_m->id_MAD << ": принята мониторограмма"
				<< std::endl;
		if (pmad->__enabMesMonit)
			for (int i = 0; i < 4; i++)
				std::cout << "канал " << i + 1 << ": СКО "
						<< buf_m->dispersion[i] << "\tсреднее "
						<< buf_m->math_ex[i] << std::endl;
		pmad[(buf_m->id_MAD) - 1].recM();
	} else
		std::cout << "Принятая мониторограмма не прошла проверку\n";
	Mad::__len = 0;
	return;
}

void Mad::recD(void) {
	DataUnit* buf_d = reinterpret_cast<DataUnit*>(Mad::__buf + 1);
	//запись в файл
	if (__wFile.isWrite) {
		if (__wFile.numSampl == SIZE_P) {
			__wFile.file.write(reinterpret_cast<char*>(buf_d->sampl),
					buf_d->amountCount * 4 * sizeof(int));
			closeWriteFile();
		} else {
			int num;
			if (__wFile.count - buf_d->amountCount < 0)
				num = __wFile.count;
			else
				num = buf_d->amountCount;
			__wFile.file.write(reinterpret_cast<char*>(buf_d->sampl),
					num * 4 * sizeof(int));
			if ((__wFile.count -= num) <= 0) {
				closeWriteFile();
			}
		}
	}
	if (__mode == DETECTION1 && buf_d->mode == DETECTION1)
		checkRate();
	if (__enabMesData)
		std::cout << "Данные переданы в Центр\n";
	if (__isEnableTrans) {
		if (__mode == ALGORITHM1)
			__alg1.pass(__buf, __len);
		else if (__mode == GASIK)
			__gasik.pass(__buf, __len);
		else
			__pcenter->trans(__buf, __len);
	}
}

void receiptData(Mad* pmad) {
	DataUnit* buf_d = reinterpret_cast<DataUnit*>(Mad::__buf + 1);
	Mad::__len = recvfrom(Mad::__sockData, reinterpret_cast<void *>(buf_d),
			sizeof(Mad::__buf) - sizeof(int), 0, NULL, NULL);
	if (Mad::__len >= sizeof(DataUnit) && buf_d->ident == SIGNAL_SAMPL
			&& buf_d->id_MAD <= Mad::__numMad) {
		Mad::__buf[0] = static_cast<unsigned int>(Mad::__timeSync);
		Mad::__len += sizeof(int);
		pmad[(buf_d->id_MAD) - 1].recD();
	} else
		std::cout << "Принятый блок данных не прошёл проверку\n";
	Mad::__len = 0;
	return;
}

void Mad::checkRate(void) {
	time_t st = time(NULL);
	if ((st - __curTimeOut) <= MIN_PER_OUT) {
		if (__countFreqOut < MAX_COUNT_FREQ_OUT)
			__countFreqOut++;
	} else
		__countFreqOut = 0;

	__isEnableTrans = (__countFreqOut == MAX_COUNT_FREQ_OUT) ? false : true;
	__curTimeOut = st;
	return;
}

Mad::Mad(Mad&& a) :
		__alg1(a.__alg1) {
	this->__gasik = std::move(a.__gasik);
	this->__id = a.__id;
	this->__curTimeOut = a.__curTimeOut;
	this->__countFreqOut = a.__countFreqOut;
	this->__isEnableTrans = a.__isEnableTrans;
	this->__pcenter = a.__pcenter;
	this->__madAddr = a.__madAddr;
	this->__mode = a.__mode;
	for (int i = 0; i < 4; i++)
		__gain[i] = a.__gain[i];
	this->__noise = a.__noise;
	return;
}

unsigned Mad::getNumMad(void) {
	return __numMad;
}

void Mad::closeWriteFile(void) {
	__wFile.isWrite = false;
	__wFile.numSampl = 0;
	__wFile.count = 0;
	__wFile.file.close();
	return;
}

void Mad::writeFile(int& numSampl, std::string &path) {
	__wFile.file.open(path, std::ios::out | std::ios::binary | std::ios::trunc);
	if (!__wFile.file.is_open()) {
		std::cout << "Нет возможности сосздать файл " << path << std::endl;
		return;
	} else {
		__wFile.isWrite = true;
		__wFile.numSampl = __wFile.count = numSampl;
	}
	return;
}

void Mad::releaseSettings(void) {
	comChangeMode(false, __duplicate.mode);
	comChangeGain(false, __duplicate.gain);
	return;
}

void Mad::initialize(center_n::Center& Center) {
	__pcenter = &Center;
	return;
}

Mad::~Mad() {
	if (!__numMad--) {
		close(__sockData);
		close(__sockMon);
		close(__sockCon);
	}
	__sockData = __sockMon = __sockCon = -1;
	__wFile.file.close();
	__pcenter->remMad(__id);
	return;
}

void hand_SIGALRM(int) {
	Mad::synchronization();
	return;
}

void Mad::copySettings(void) {
	__duplicate.mode = __mode;
	for (int i = 0; i < 4; i++)
		__duplicate.gain[i] = __gain[i];
	__duplicate.__noise = __noise;
	return;

}

} /* namespace mad_n */

