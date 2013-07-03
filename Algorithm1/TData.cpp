/************************************************/
//TData.cpp
/************************************************/
#include "TData.h"

#ifdef _WIN32
/************************************************/
//NV - количество векторов vector <Double_t> *, далее их список через запятую
void TReadCVSFileData :: operator () (const int NV, ...)
{
	int i;
	va_list ArgList;
	double Value;
	ifstream DPStream;

	//va_start(ArgList, NV); 
	DPStream.open(FileName);
	
	while (DPStream.good()) {
		va_start(ArgList, NV); 
		for (i = 0; i < NV; i++)
		{
			DPStream >> Value;
			if (DPStream.fail()) break;
			va_arg(ArgList, PDataVector)->push_back(Value);	//<<<<<<<<<<<Оптимизировать вставку выделяя память с запасом!			
			if (!DPStream.good()) break;
		};
	};

	DPStream.close();
	va_end(ArgList); 
};

/************************************************/
//Класс TDataLoader для поиска файлов и доступа к ним

TDataLoader :: TDataLoader( const char * PDataDir, const char * PFileMask, const bool SearchSubDirs )
{
	list <string> DataDirList;
	string SDataDir (PDataDir);
	if (SearchSubDirs)
	{
		GetSubDirs( PDataDir, & DataDirList );
		GetFileList( & DataDirList, PFileMask );
	} else 
	{
		DataDirList.push_back(SDataDir);
		GetFileList( & DataDirList, PFileMask );
	};
};

//Ищет поддиректории в указанном пути
void TDataLoader :: GetSubDirs( const char * PDataDirS, list <string> * PDataDirList ) 
{
	string SDataDir(PDataDirS);
	string DirMask = SDataDir;
	string sDir;
	DirMask+="/*.*";
	struct _finddata_t _SearchRec;
	long hfile,iss;

	PDataDirList->push_back(SDataDir);
	
	_SearchRec.attrib = 0x00000010; //Directory
	hfile = _findfirst(DirMask.data(), &_SearchRec);
	if (hfile != -1)
	{	iss = 0;
		do{
			sDir.clear();
			sDir+=SDataDir;
			sDir+="/";
			sDir+=_SearchRec.name;
			PDataDirList->push_back(sDir);
			iss++;
		}while(_findnext(hfile, &_SearchRec) != -1);
	}
	_findclose(hfile);
};	// of GetSubDirs

//Ищет файлы в списке каталогов
void TDataLoader :: GetFileList( const list <string> * PDataDirList, const char * PFileMask ) 
{
	struct _finddata_t _SearchRec;
	string _DataFilesMask;
	string RootDir = *(PDataDirList->begin()); RootDir+="/..";
	string Root	=	*(PDataDirList->begin()); Root+="/.";
	string sFile;
	std::list <string> :: const_iterator CurrentListPosition;
	long hfile;
	_SearchRec.attrib = 0; //AnyFile

	for ( CurrentListPosition = PDataDirList->begin(); CurrentListPosition != PDataDirList->end(); ++CurrentListPosition )
	{	
			_DataFilesMask = (*CurrentListPosition);
			_DataFilesMask += "/";
			_DataFilesMask += PFileMask;//"/*.sin";
			hfile = _findfirst(_DataFilesMask.data(), &_SearchRec);
			if (hfile != -1)
				do{
					sFile.clear();
					sFile = (*CurrentListPosition);
					sFile += "/";
					sFile += _SearchRec.name;
					if ((sFile != Root) && (sFile != RootDir))
						this->push_back(sFile);
				}while(_findnext(hfile, &_SearchRec) != -1);
			_findclose(hfile);
	}
}		// of GetFileList
#endif //_WIN32


