/*******************************************************************************
Copyright (c) 2014, Jan Koester jan.koester@gmx.net
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
	* Redistributions of source code must retain the above copyright
	  notice, this list of conditions and the following disclaimer.
	* Redistributions in binary form must reproduce the above copyright
	  notice, this list of conditions and the following disclaimer in the
	  documentation and/or other materials provided with the distribution.
	* Neither the name of the <organization> nor the
	  names of its contributors may be used to endorse or promote products
	  derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

#ifndef CTRLHANDlER_H
#define CTRLHANDLER_H


namespace libhttppp {
	class CtrlHandler {
	public:
		CtrlHandler();
		~CtrlHandler();
		
		virtual void CTRLCloseEvent()=0;
		virtual void CTRLBreakEvent()=0;
		static BOOL WINAPI CtrlEventHandler(DWORD eventin);
		static void		  *CTRLPtr;
	};
};

#endif



/*
SIGHUP	1	Logoff
SIGINT	2	Benutzer-Interrupt (ausgelöst durch [Strg]+[C])
SIGQUIT	3	Benutzeraufforderung zum Beenden (ausgelöst durch [Strg)+[\])
SIGFPE	8	Fließkommafehler, beispielsweise Null-Division
SIGKILL	9	Prozess killen
SIGUSR1	10	Benutzerdefiniertes Signal
SIGSEGV	11	Prozess hat versucht, auf Speicher zuzugreifen, der ihm nicht zugewiesen war
SIGUSR2	12	Weiteres benutzerdefiniertes Signal
SIGALRM	14	Timer (Zeitgeber), der mit der Funktion alarm() gesetzt wurde, ist abgelaufen
SIGTERM	15	Aufforderung zum Beenden
SIGCHLD	17	Kindprozess wird aufgefordert, sich zu beenden
SIGCONT	18	Nach einem SIGSTOP- oder SIGTSTP-Signal fortfahren
SIGSTOP	19	Den Prozess anhalten
SIGTSTP	20 Prozess suspendiert, ausgelöst durch [Strg)+[Z].
*/