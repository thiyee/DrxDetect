#include <Windows.h>
#include <stdio.h>
#include <intrin.h>

void DR0Func() {}
void DR1Func() {}
void DR2Func() {}
void DR3Func() {}
int Dr0Handler(PEXCEPTION_POINTERS Ex) {
	if (!(Ex->ContextRecord->Dr0 == DR0Func && (Ex->ContextRecord->Dr6 & (1 << 0)) && (Ex->ContextRecord->Dr7 & (1 << 0)))) {
		printf("Invalid Dr0 Exception info Ex->ContextRecord->Dr0:%llx Ex->ContextRecord->Dr6:%llx\n", Ex->ContextRecord->Dr0, Ex->ContextRecord->Dr6);
	}
	return EXCEPTION_EXECUTE_HANDLER;
}
int Dr1Handler(PEXCEPTION_POINTERS Ex) {
	if (!(Ex->ContextRecord->Dr1 == DR1Func && (Ex->ContextRecord->Dr6 & (1 << 1)) && (Ex->ContextRecord->Dr7 & (1 << 2)))) {
		printf("Invalid Dr1 Exception info Ex->ContextRecord->Dr1:%llx Ex->ContextRecord->Dr6:%llx\n", Ex->ContextRecord->Dr1, Ex->ContextRecord->Dr6);
	}
	return EXCEPTION_EXECUTE_HANDLER;

}
int Dr2Handler(PEXCEPTION_POINTERS Ex) {
	if (!(Ex->ContextRecord->Dr2 == DR2Func && (Ex->ContextRecord->Dr6 & (1 << 2)) && (Ex->ContextRecord->Dr7 & (1 << 4)))) {
		printf("Invalid Dr2 Exception info Ex->ContextRecord->Dr2:%llx Ex->ContextRecord->Dr6:%llx\n", Ex->ContextRecord->Dr2, Ex->ContextRecord->Dr6);
	}
	return EXCEPTION_EXECUTE_HANDLER;

}
int Dr3Handler(PEXCEPTION_POINTERS Ex) {
	if (!(Ex->ContextRecord->Dr3 == DR3Func && (Ex->ContextRecord->Dr6 & (1 << 3)) && (Ex->ContextRecord->Dr7 & (1 << 6)))) {
		printf("Invalid Dr3 Exception info Ex->ContextRecord->Dr3:%llx Ex->ContextRecord->Dr6:%llx\n", Ex->ContextRecord->Dr3, Ex->ContextRecord->Dr6);
	}
	return EXCEPTION_EXECUTE_HANDLER;

}

BOOLEAN DrxDetectByGetThreadContext() {
	CONTEXT Context = { .ContextFlags = CONTEXT_DEBUG_REGISTERS };
	GetThreadContext(GetCurrentThread(), &Context);
	BOOLEAN bDetect = TRUE;
	if (Context.Dr0 != 0) { printf("Invalid Dr0:%llx\n", Context.Dr0); bDetect = TRUE; }
	else if (Context.Dr1 != 0) { printf("Invalid Dr1:%llx\n", Context.Dr1); bDetect = TRUE; }
	else if (Context.Dr2 != 0) { printf("Invalid Dr2:%llx\n", Context.Dr2); bDetect = TRUE; }
	else if (Context.Dr3 != 0) { printf("Invalid Dr3:%llx\n", Context.Dr3); bDetect = TRUE; }
	else if (Context.Dr6 != 0) { printf("Invalid Dr6:%llx\n", Context.Dr6); bDetect = TRUE; }
	else if (Context.Dr7 != 0) { printf("Invalid Dr7:%llx\n", Context.Dr7); bDetect = TRUE; }
	else  bDetect = FALSE;
}
int ExceptionContextHandler(PEXCEPTION_POINTERS Ex) {
	BOOLEAN bDetect = TRUE;
	if (Ex->ContextRecord->Dr0 != 0) { printf("Invalid Dr0:%llx\n", Ex->ContextRecord->Dr0); bDetect = TRUE; }
	else if (Ex->ContextRecord->Dr1 != 0) { printf("Invalid Dr1:%llx\n", Ex->ContextRecord->Dr1); bDetect = TRUE; }
	else if (Ex->ContextRecord->Dr2 != 0) { printf("Invalid Dr2:%llx\n", Ex->ContextRecord->Dr2); bDetect = TRUE; }
	else if (Ex->ContextRecord->Dr3 != 0) { printf("Invalid Dr3:%llx\n", Ex->ContextRecord->Dr3); bDetect = TRUE; }
	else if (Ex->ContextRecord->Dr6 != 0) { printf("Invalid Dr6:%llx\n", Ex->ContextRecord->Dr6); bDetect = TRUE; }
	else if (Ex->ContextRecord->Dr7 != 0) { printf("Invalid Dr7:%llx\n", Ex->ContextRecord->Dr7); bDetect = TRUE; }
	else  bDetect = FALSE;

	if (bDetect) return EXCEPTION_EXECUTE_HANDLER;
	else {
		Ex->ContextRecord->Rip += 1;
		Ex->ContextRecord->Dr0 = 0x111111;
		Ex->ContextRecord->Dr1 = 0x222222;
		Ex->ContextRecord->Dr2 = 0x333333;
		Ex->ContextRecord->Dr3 = 0x444444;
		Ex->ContextRecord->Dr7 = 0x55;
		Ex->ContextRecord->ContextFlags |= CONTEXT_DEBUG_REGISTERS;
		return EXCEPTION_CONTINUE_EXECUTION;
	}
}
BOOLEAN DrxDetectByExceptionContext() {
	__try {
		__debugbreak();
		CONTEXT Context = { .ContextFlags = CONTEXT_DEBUG_REGISTERS };
		GetThreadContext(GetCurrentThread(), &Context);
		if ((Context.Dr0 != 0x111111) ||
			(Context.Dr1 != 0x222222) ||
			(Context.Dr2 != 0x333333) ||
			(Context.Dr3 != 0x444444)) {
			printf("Drx Set By Exception Handler Failed Dr0:%llx Dr1:%llx Dr2:%llx Dr3:%llx\n",Context.Dr0,Context.Dr1,Context.Dr2,Context.Dr3);
			return TRUE;
		}
		Context.Dr0 = 0;
		Context.Dr1 = 0;
		Context.Dr2 = 0;
		Context.Dr3 = 0;
		Context.Dr6 = 0;
		Context.Dr7 = 0;
		SetThreadContext(GetCurrentThread(), &Context);
		return FALSE;
	}
	__except (ExceptionContextHandler(GetExceptionInformation())) {
		printf("Exception Not Processed By Exception Handler\n");
		return TRUE;
	}
	return TRUE;
}

BOOLEAN DrxUsableDetect() {
	BOOLEAN Detect = TRUE;
	CONTEXT Context = { .ContextFlags = CONTEXT_DEBUG_REGISTERS };
	Context.Dr0 = DR0Func;
	Context.Dr1 = DR1Func;
	Context.Dr2 = DR2Func;
	Context.Dr3 = DR3Func;
	Context.Dr7 = (1 << 0) | (1 << 2) | (1 << 4) | (1 << 6);
	SetThreadContext(GetCurrentThread(), &Context);

	__try {
		DR0Func();//这里应该触发断点
		Detect = TRUE;//没有触发断点 说明dr0寄存器被占用
		printf("Dr0 Unusable\n");
	}
	__except (Dr0Handler(GetExceptionInformation())) {
		__try {
			DR1Func();//这里应该触发断点
			Detect = TRUE;//没有触发断点 说明dr1寄存器被占用
			printf("Dr1 Unusable\n");

		}
		__except (Dr1Handler(GetExceptionInformation())) {
			__try {
				DR2Func();//这里应该触发断点
				Detect = TRUE;//没有触发断点 说明dr2寄存器被占用
				printf("Dr2 Unusable\n");

			}
			__except (Dr2Handler(GetExceptionInformation())) {
				__try {
					DR3Func();//这里应该触发断点
					Detect = TRUE;//没有触发断点 说明dr3寄存器被占用
					printf("Dr3 Unusable\n");
				}
				__except (Dr3Handler(GetExceptionInformation())) {
					CONTEXT Context1 = { .ContextFlags = CONTEXT_DEBUG_REGISTERS };
					Context1.Dr0 = 0;
					Context1.Dr1 = 0;
					Context1.Dr2 = 0;
					Context1.Dr3 = 0;
					Context1.Dr6 = 0;
					Context1.Dr7 = 0;
					SetThreadContext(GetCurrentThread(), &Context1);
					Detect = FALSE;
				}
			}
		}
	}
	return Detect;

}

DWORD WINAPI DrDetectThread(LPVOID lpThreadParameter) {

	printf("DrxDetectByGetThreadContext %s\n", DrxDetectByGetThreadContext() ? "Found" : "Not Fount");
	printf("DrxDetectByExceptionContext %s\n", DrxDetectByExceptionContext() ? "Found" : "Not Fount");
	printf("DrxUsableDetect             %s\n", DrxUsableDetect() ? "Found" : "Not Fount");
	printf("IsDebuggerPresent:%08x  %s\n",*(DWORD*)IsDebuggerPresent, IsDebuggerPresent() ? "Found" : "Not Fount");
	printf("DrxUsableDetect             %s\n", DrxUsableDetect() ? "Found" : "Not Fount");
	printf("DrxDetectByExceptionContext %s\n", DrxDetectByExceptionContext() ? "Found" : "Not Fount");
	printf("DrxDetectByGetThreadContext %s\n", DrxDetectByGetThreadContext() ? "Found" : "Not Fount");


}






void main() {

	printf("%d\n", GetCurrentProcessId());
	while (getchar()) {
		CreateThread(0, 0x1000, DrDetectThread, 0, 0, 0);
	}


}