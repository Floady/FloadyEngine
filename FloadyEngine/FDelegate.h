#pragma once

class FDelegate
{
public:
	FDelegate()
		: object_ptr(0)
		, stub_ptr(0)
	{}

	template < class T, void (T::*TMethod)() >
	static FDelegate from_method(T* object_ptr)
	{
		FDelegate d;
		d.object_ptr = object_ptr;
		d.stub_ptr = &delegate_stub_t< T, TMethod >::method_stub; // #1

		return d;
	}

	void operator()() const
	{
		return (*stub_ptr)(object_ptr);
	}

private:
	typedef void(*stub_type)(void* object_ptr);

	void* object_ptr;
	stub_type stub_ptr;

	template < class T, void (T::*TMethod)() >
	struct delegate_stub_t
	{
		static void method_stub(void* object_ptr)
		{
			T* p = static_cast< T* >(object_ptr);
			return (p->*TMethod)(); // #2

		}
	};
};