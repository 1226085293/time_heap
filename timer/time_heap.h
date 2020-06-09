#pragma once
#include <future>
#include <functional>
#include "other/raii/critical.h"

class time_heap {
private:
	// 事件信息
	class event {
		friend class time_heap;
	private:
		intptr_t				_origin;	//源地址
		uint64_t				_ms;		//定时时长
		uint32_t				_count;		//执行数
		std::function<void()>	_func;		//回调函数
	protected:
	public:
		event(intptr_t origin_, uint64_t ms_, uint32_t count_, std::function<void()>&& func_);
		event(const event&) = delete;
		event& operator =(const event&) = delete;
		~event() = default;
		// 删除函数
		void del();
	};

	// 返回数据
	template <class T>
	class result_data {
		friend class time_heap;
	private:
		event*										_info;		//事件信息
		std::shared_ptr<std::packaged_task<T()>>	_pack_func;	//已包装的函数
	protected:
	public:
		bool					valid;		//有效性
		uint32_t				count;		//执行数


		result_data(uint32_t count_, std::shared_ptr<std::packaged_task<T()>> pack_func_);
		result_data(const result_data&) = delete;
		result_data& operator =(const result_data&) = delete;
		~result_data() = default;

		// 获取执行结果
		T get();
		// 删除定时器
		void del();
	};

	// 堆节点
	struct node {
		uint64_t	ms;
		event*		info;

		node() = default;
		node(const node&) = delete;
		node(node&& that_);
		node& operator =(const node& that_);
		node& operator =(node&& that_);
		~node() = default;
	};
	std::atomic<bool>					_death;			//结束
	CRITICAL_SECTION					_cri;			//临界区
	node*								_heap;			//最小堆
	uint32_t							_max_size;		//最大存储数
	uint32_t							_size;			//存储数
	uint64_t							_event_time;	//插入时间
	HANDLE								_event;			//事件
	static std::unique_ptr<time_heap>	_instance;		//实例指针

	// 添加节点
	void add_node();
	// 删除节点
	void del_node(uint32_t index_);
	// 删除指定下标定时器
	void del_timer(uint32_t index_);
	// 重置定时器
	void reset_timer(uint32_t index_);
	// 扩容
	void expansion();
protected:
public:
	time_heap(uint32_t max_size_ = 64);
	~time_heap();
	// 单例
	static time_heap& instance();
	// 获取距当前时间(毫秒)
	uint64_t get_ms();
	// 添加
	template<class Func, class... Args>
	auto add(double second_, uint32_t count_, Func&& func_, Args&& ... args_);
};

#include "source/time_heap.icc"