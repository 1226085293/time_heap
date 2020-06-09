#include "../time_heap.h"
std::unique_ptr<time_heap> time_heap::_instance;

time_heap::time_heap(uint32_t max_size_) :
	_death(false),
	_heap(new node[max_size_]{}),
	_max_size(max_size_),
	_size(0),
	_event(CreateEvent(NULL, FALSE, FALSE, NULL))
{
	// 初始化临界区
	InitializeCriticalSection(&_cri);
	std::thread work_thread([&] {
		DWORD wait_re, ms = INFINITE;
		std::shared_ptr<std::function<void()>> func;
		while (true) {
			wait_re = ms ? WaitForSingleObject(_event, ms) : WAIT_TIMEOUT;
			if (!_size) {
				ms = INFINITE;
				continue;
			}
			if (_death) {
				break;
			}
			auto current_ms = get_ms();
			raii::critical r1(&_cri);
			if (wait_re == WAIT_OBJECT_0) {		//事件
				ms = static_cast<DWORD>(current_ms > _heap[1].ms ? 0 : (_heap[1].ms - current_ms - (current_ms - _event_time)));
			}
			else if (wait_re == WAIT_TIMEOUT) {	//超时
				//printf("超时\n");
				_heap[1].info->_func();
				if (_heap[1].info->_count == INFINITE) {
					reset_timer(1);
				}
				else if (--_heap[1].info->_count == 0) {
					del_timer(1);
				}
				else {
					reset_timer(1);
				}
				ms = static_cast<DWORD>(_size > 0 ? _heap[1].ms - current_ms : INFINITE);
			}
			else if (wait_re == WAIT_FAILED) {	//失败
				printf("失败: %I64d\n", current_ms);
			}
		}
	});
	work_thread.detach();
}

time_heap::~time_heap() {
	_death = true;
	{
		raii::critical r1(&_cri);
		// 退出工作线程
		SetEvent(_event);
		for (; _size > 0; --_size) {
			delete _heap[_size].info;
		}
		delete[] _heap;
	}
	DeleteCriticalSection(&_cri);
}

void time_heap::add_node() {
	raii::critical r1(&_cri);
	uint32_t index1 = ++_size, index2 = index1 >> 1;
	while (index1 > 1 && _heap[index2].ms > _heap[index1].ms) {
		std::swap(_heap[index1], _heap[index2]);
		index1 = index2;
		index2 >>= 1;
	}
}

void time_heap::del_node(uint32_t index_) {
	raii::critical r1(&_cri);
	if (_size < index_) {
		return;
	}
	// 向下调整
	if (_heap[_size].ms > _heap[index_].ms) {
		uint32_t index1 = index_, index2 = index1 << 1;
		while (index2 < _size) {
			if (_heap[index2].ms > _heap[index2 + 1].ms) {
				++index2;
			}
			if (_heap[_size].ms > _heap[index2].ms) {
				_heap[index1] = _heap[index2];
				index1 = index2;
				index2 <<= 1;
			}
			else {
				break;
			}
		}
		_heap[index1] = _heap[_size];
		memset(&_heap[_size], 0, sizeof(node));
	}
	// 向上调整
	else if (_heap[_size].ms < _heap[index_].ms) {
		uint32_t index1 = index_ >> 1, index2 = _size;
		while (_heap[index1].ms > _heap[index2].ms) {
			std::swap(_heap[index1], _heap[_size]);
			index2 = index1;
			index1 >>= 1;
		}
		_heap[index_] = _heap[_size];
		memset(&_heap[_size], 0, sizeof(node));
	}
	// 不做调整
	else {
		_heap[index_] = _heap[_size];
		memset(&_heap[_size], 0, sizeof(node));
	}
	if (--_size > 0) {
		_event_time = get_ms();
		SetEvent(_event);
	}
}

void time_heap::expansion() {
	auto size = sizeof(node) * _max_size;
	node* new_heap = new node[_max_size <<= 1]{};
	raii::critical r1(&_cri);
	memcpy_s(new_heap, size << 1, _heap, size);
	delete[] _heap;
	_heap = new_heap;
}

time_heap& time_heap::instance() {
	static std::once_flag s_flag;
	std::call_once(s_flag, [&]() {
		_instance.reset(new time_heap);
	});
	return *_instance;
}