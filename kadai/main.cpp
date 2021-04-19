#include<Windows.h>
#include<iostream>
#include <stdio.h>
#include <tchar.h>
#include <WinNT.h>
#include <iomanip>
#include <string>
#include <psapi.h>
#include <strsafe.h>
#include <bitset>
#include <fstream>

using namespace std;

constexpr int BUFSIZE{ 512 };

// std::coutによる出力とofstreamオブジェクトへの書き込みを一度に行うためのクラス
// 引数にostreamオブジェクト2つをとり、右辺値をそのまま2つに入れる
class dout
{
private:
	ostream& os1, & os2;
public:
	dout(ostream& _os1, ostream& _os2) :os1(_os1), os2(_os2) {};

	template<typename T>
	dout& operator<<(const T& rhs) {
		os1 << rhs;
		os2 << rhs;
		return *this;
	};
	dout& operator<< (std::ostream& (*__pf)(std::ostream&)) {
		__pf(os1); __pf(os2);
		return *this;
	}
};

void register_condition(DEBUG_EVENT& , CONTEXT& ,ofstream&);
void format_error_message();
BOOL GetFileNameFromHandle(HANDLE,DWORD,ofstream&);//Microsoftが提供しているfilehandleからファイルのフルパスを出力する関数
BOOL binary(unsigned int eflags_cpy,ofstream&);



int main() {
	int enter_key{};
	bool file_check{false};
	bool txt_check{ false };
	bool roop_key{ true };
	WCHAR  exe_file_path[256];
	string text_file_path{};
	int stop{};


	while (roop_key) {
		//system("cls");
		cout << "以下の項目から選んでください" << endl;
		cout << "\nデバッガの実行----Enter\'1\'" << endl;
		cout << "デバッグ対象の実行ファイルを指定する----Enter\'2\'" << endl;
		cout << "デバッグ結果の出力先ファイルを指定する----Enter\'3\'" << endl;
		cout << "デバッガを閉じる----Enter\'4\'\n" << endl;

		cout << "Enter:";

		cin >> enter_key;

		switch (enter_key)
		{
		case 1:
			if (file_check&&txt_check) {
				cout << "デバッガを実行します" << endl;
				roop_key = false;
				break;
			}
			cout << "実行ファイルと出力先ファイルを指定してください\n" << endl;
			break;
		case 2:
			cout << "実行ファイルを絶対PATHで指定してください： ";
			wscanf_s(L"%s", exe_file_path,256);

			file_check = true;

			break;
		case 3:
			cout << "出力先ファイルを絶対PATHで指定してください： ";
			cin >> text_file_path;
			text_file_path += "\\outputtxt.txt";
			txt_check = true;

			break;

		case 4:
			return 0;
			break;

		default:
			break;
		}

	}
	//createprocess関数に渡すURLの準備
	wstring exe_file_path_cpy = L"";
	exe_file_path_cpy.append(exe_file_path);
	LPWSTR true_exe_file_path = &exe_file_path_cpy[0];

	// 指定したファイルパスからテキストファイルを読み込む。無ければ作る
	ofstream out_file{ text_file_path,std::ios::out };

	if (!out_file.is_open()) {
		cout << "ファイルが開けません" << endl;
		return 0;
	}

	STARTUPINFO si;
	//構造体。CreateProcess関数の実行後、デバック対象プログラムのプロセスIDとスレッドIDを取得できる
	PROCESS_INFORMATION pi;

	memset(&pi, 0, sizeof(pi));
	memset(&si, 0, sizeof(si));
	si.cb = sizeof(STARTUPINFO);
	//wstring filename(L"c:\\windows\\syswow64\\notepad.exe");

	BOOL creationResult = CreateProcess(NULL,
		true_exe_file_path,
		NULL,
		NULL,
		FALSE,
		NORMAL_PRIORITY_CLASS|DEBUG_ONLY_THIS_PROCESS,
		NULL,
		NULL,
		&si,
		&pi);

	if (!creationResult) {
		cout << "実行ファイルを読み取れませんでした。" << endl;
		return 0;
	}



	CONTEXT regis; //レジスタの情報を入れるための構造体
	memset(&regis, 0, sizeof(regis)); //構造体の初期化
	regis.ContextFlags = CONTEXT_FULL;//レジスタの情報を入れるための構造体のフラグの設定
	DEBUG_EVENT de; //DEBUG_EVENT構造体の宣言
	DWORD dwContinueStatus = DBG_CONTINUE;

	while (1) {

		//CreateProcess関数によって起動されたプログラムは、WaitForDebugEvent関数が呼び出されるまで停止している。
		if (!WaitForDebugEvent(&de, INFINITE)) {
			break;
		}

		dwContinueStatus = DBG_CONTINUE;//スレッドで例外発生時に例外処理を無視し処理を続ける設定


		switch (de.dwDebugEventCode)//waitfordebugevent関数で補足した情報をDEBUG__EVENT構造体から取り出す。dwDebugEventCodeの値によって共用体uの値が決まる。
		{
		case CREATE_PROCESS_DEBUG_EVENT:
			dout(cout,out_file) << "*******************************************************************" << endl;
			dout(cout, out_file) << "プロセスが生成されました。\nスレッドの開始アドレスは: 0x" << std::hex<<std::setfill('0') << std::right << std::setw(8) << *de.u.CreateProcessInfo.lpStartAddress << endl;
			GetFileNameFromHandle(de.u.CreateProcessInfo.hFile, de.dwDebugEventCode,out_file);
			dout(cout, out_file) << "開始時のプロセスのIDは: 0x" << std::setfill('0') << std::right << std::setw(8) << de.dwProcessId << endl;
			dout(cout, out_file) << "開始時のスレッドのIDは: 0x" << std::setfill('0') << std::right << std::setw(8) << de.dwThreadId << endl;
			register_condition(de, regis,out_file);
			dout(cout, out_file) << "*******************************************************************\n" << endl;
			
			break;
		case EXIT_PROCESS_DEBUG_EVENT:
			dout(cout, out_file) << "\n*******************************************************************" << endl;
			dout(cout, out_file) << "プロセスが終了しました。\nプロセスの終了コードは:" << de.u.ExitProcess.dwExitCode << endl;;
			dout(cout, out_file) << "終了時のプロセスのIDは:" << de.dwProcessId << endl;
			dout(cout, out_file) << "終了時のスレッドのIDは:" << de.dwThreadId << endl;
			register_condition(de, regis,out_file);
			dout(cout, out_file) << "*******************************************************************\n" << endl;

		case EXCEPTION_DEBUG_EVENT:
			if (de.u.Exception.ExceptionRecord.ExceptionCode != EXCEPTION_BREAKPOINT) {
				dwContinueStatus = DBG_EXCEPTION_NOT_HANDLED;
				break;
			}
		case CREATE_THREAD_DEBUG_EVENT:
			dout(cout, out_file) << "スレットが作成されました" << endl;
			break;
		case EXIT_THREAD_DEBUG_EVENT:
			dout(cout, out_file) << "スレッドが終了しました" << endl;
			break;
		case LOAD_DLL_DEBUG_EVENT:
			if (!GetFileNameFromHandle(de.u.LoadDll.hFile,de.dwDebugEventCode,out_file)) 
			{
				format_error_message();
				break;
			};
			break;
		case UNLOAD_DLL_DEBUG_EVENT:
			if (!GetFileNameFromHandle(de.u.LoadDll.hFile, de.dwDebugEventCode,out_file))
			{
				format_error_message();
				break;
			};
			break;
		case OUTPUT_DEBUG_STRING_EVENT:
			dout(cout, out_file) << "OutputDebugString関数が呼び出されました。" << endl;
				break;
		case RIP_EVENT:
			dout(cout, out_file) << "システムデバッグエラーが発生しました。" << endl;
			break;

		default:
			break;
		}

		if (de.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT) {
			break;
		}

		/* デバッグイベントの発生によってWaitForDebugEventがコールされる
		コールされるとデバック対象のプログラム（デバッギ）は停止するので、対応する処理をした後この関数で再開させる*/
		ContinueDebugEvent(de.dwProcessId, de.dwThreadId, dwContinueStatus);

	}

	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
	out_file.close();
	return 0;
}


void register_condition(DEBUG_EVENT& de, CONTEXT& regis,std::ofstream &out_file) {
	if (OpenThread(THREAD_ALL_ACCESS, true, de.dwThreadId) != NULL)//既存のスレッドを開く。開けなかった場合はエラーメッセージを返す。
	{
		dout(cout, out_file) << "\n現在のレジスタの状況は" << endl;
		GetThreadContext(OpenThread(THREAD_ALL_ACCESS, true, de.dwThreadId), &regis);
		dout(cout, out_file) << "RAX: 0x" << std::setfill('0') << std::right << std::setw(8) << regis.Rax << " ---演算結果を格納"<<endl;
		dout(cout, out_file) << "RCX: 0x" << std::setfill('0') << std::right << std::setw(8) << regis.Rcx << " ---ループの回数などのカウントを格納"<<endl;
		dout(cout, out_file) << "RDX: 0x" << std::setfill('0') << std::right << std::setw(8) << regis.Rdx << " ---演算に用いるデータを格納"<<endl;
		dout(cout, out_file) << "RBX: 0x" << std::setfill('0') << std::right << std::setw(8) << regis.Rbx << " ---アドレスの基点を格納"<<endl;
		dout(cout, out_file) << "RSP: 0x" << std::setfill('0') << std::right << std::setw(8) << regis.Rsp << " ---スタックの頂点を指すポインタ"<<endl;
		dout(cout, out_file) << "RBP: 0x" << std::setfill('0') << std::right << std::setw(8) << regis.Rbp << " ---実行中の関数のスタック領域の底"<<endl;
		dout(cout, out_file) << "RSI: 0x" << std::setfill('0') << std::right << std::setw(8) << regis.Rsi << " ---読み込み先のインデックスが入る"<< endl;
		dout(cout, out_file) << "RDI: 0x" << std::setfill('0') << std::right << std::setw(8) << regis.Rdi << " ---書き込み先のインデックスが入る"<<endl;
		dout(cout, out_file) << "\nRIP: 0x" << std::setfill('0') << std::left << std::setw(8) << regis.Rip << " ---次に実行する機械語命令へのポインタ"<<endl;
		dout(cout, out_file) << "Eflags:  " << std::setfill('0') << std::right << std::setw(8) << std::bitset <64>(regis.EFlags) << " ---現在のフラグ状況" << endl;
		binary(regis.EFlags,out_file);

	}
	else {
		format_error_message();
	}
}

void format_error_message() {
	DWORD errorcode = GetLastError();
	LPVOID lpMsgBuf = new LPVOID;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER  //      テキストのメモリ割り当てを要求する
		| FORMAT_MESSAGE_FROM_SYSTEM    //      エラーメッセージはWindowsが用意しているものを使用
		| FORMAT_MESSAGE_IGNORE_INSERTS,//      次の引数を無視してエラーコードに対するエラーメッセージを作成する
		NULL, errorcode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),//   言語を指定
		(LPTSTR)&lpMsgBuf,                          //      メッセージテキストが保存されるバッファへのポインタ
		0,
		NULL);

	MessageBox(0, (LPCTSTR)lpMsgBuf, _TEXT("エラー"), MB_OK | MB_ICONINFORMATION);
	delete lpMsgBuf;
}

BOOL GetFileNameFromHandle(HANDLE hFile,DWORD dwDebugEventCode,ofstream &out_file)
{
	BOOL bSuccess = FALSE;
	TCHAR pszFilename[MAX_PATH + 1];
	HANDLE hFileMap;

	// Get the file size.
	DWORD dwFileSizeHi = 0;
	DWORD dwFileSizeLo = GetFileSize(hFile, &dwFileSizeHi);

	if (dwFileSizeLo == 0 && dwFileSizeHi == 0)
	{
		_tprintf(TEXT("Cannot map a file with a length of zero.\n"));
		return FALSE;
	}

	// Create a file mapping object.
	hFileMap = CreateFileMapping(hFile,
		NULL,
		PAGE_READONLY,
		0,
		1,
		NULL);

	if (hFileMap)
	{
		// Create a file mapping to get the file name.
		void* pMem = MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 1);

		if (pMem)
		{
			if (GetMappedFileName(GetCurrentProcess(),
				pMem,
				pszFilename,
				MAX_PATH))
			{

				// Translate path with device name to drive letters.
				TCHAR szTemp[BUFSIZE];
				szTemp[0] = '\0';

				if (GetLogicalDriveStrings(BUFSIZE - 1, szTemp))
				{
					TCHAR szName[MAX_PATH];
					TCHAR szDrive[3] = TEXT(" :");
					BOOL bFound = FALSE;
					TCHAR* p = szTemp;

					do
					{
						// Copy the drive letter to the template string
						*szDrive = *p;

						// Look up each device name
						if (QueryDosDevice(szDrive, szName, MAX_PATH))
						{
							size_t uNameLen = _tcslen(szName);

							if (uNameLen < MAX_PATH)
							{
								bFound = _tcsnicmp(pszFilename, szName, uNameLen) == 0
									&& *(pszFilename + uNameLen) == _T('\\');

								if (bFound)
								{
									// Reconstruct pszFilename using szTempFile
									// Replace device path with DOS path
									TCHAR szTempFile[MAX_PATH];
									StringCchPrintf(szTempFile,
										MAX_PATH,
										TEXT("%s%s"),
										szDrive,
										pszFilename + uNameLen);
									StringCchCopyN(pszFilename, MAX_PATH + 1, szTempFile, _tcslen(szTempFile));
								}
							}
						}

						// Go to the next NULL character.
						while (*p++);
					} while (!bFound && *p); // end of string
				}
			}
			bSuccess = TRUE;
			UnmapViewOfFile(pMem);
		}

		CloseHandle(hFileMap);
	}
	//coutはcharにしか対応していないのでTCHARのpszFilenameをstringに変換する
	wstring pszFilename_copy(pszFilename);
	string pszFilename_copy_string(pszFilename_copy.begin(), pszFilename_copy.end());
	switch (dwDebugEventCode)
	{
	case 6:
		dout(cout, out_file) << "Loaded DLL :" << pszFilename_copy_string << endl;
		_tprintf(TEXT("Loaded DLL: %s\n"), pszFilename);
		return (bSuccess);
		break;
	case 7:
		dout(cout, out_file) << "Unloaded DLL :" << pszFilename_copy_string << endl;
		_tprintf(TEXT("Unloaded DLL: %s\n"), pszFilename);
		return (bSuccess);
		break;
	default:
		break;
	}
	dout(cout, out_file) << "ファイルの名前は: " << pszFilename_copy_string << endl;;
	_tprintf(TEXT("%s\n"), pszFilename);
	return(bSuccess);
}

BOOL binary(unsigned int eflags_cpy,ofstream &out_file) {
	int bit_number{};
	while (true) {
		if (bit_number == 11) {
			dout(cout, out_file) << "OF: " << eflags_cpy % 2 << endl;
			return true;
			break;
		}
		switch (bit_number)
		{
		case 0:
			dout(cout, out_file) << "CF: " << eflags_cpy % 2 << endl;
			break;
		case 2:
			dout(cout, out_file) << "PF: " << eflags_cpy % 2 << endl;
			break;
		case 4:
			dout(cout, out_file) << "AF: " << eflags_cpy % 2 << endl;
			break;
		case 6:
			dout(cout, out_file) << "ZF: " << eflags_cpy % 2 << endl;
			break;
		case 7:
			dout(cout, out_file) << "SF: " << eflags_cpy % 2 << endl;
			break;
		case 8:
			dout(cout, out_file) << "TF: " << eflags_cpy % 2 << endl;
			break;
		case 9:
			dout(cout, out_file) << "IF: " << eflags_cpy % 2 << endl;
			break;
		case 10:
			dout(cout, out_file) << "DF: " << eflags_cpy % 2 << endl;
			break;

		default:
			break;
		}
		eflags_cpy /= 2;
		bit_number++;

	}

}