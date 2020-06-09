#pragma once
#include <Windows.h>

namespace raii {
	class critical {
	private:
		CRITICAL_SECTION* _crit;
	public:
		critical(CRITICAL_SECTION* crit);
		critical(const critical&) = delete;
		critical& operator =(const critical&) = delete;
		~critical();
	};

#include "source/critical.icc"
}