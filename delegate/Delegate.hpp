#pragma once
#include <cassert>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>
#include <tuple>
#include <iostream>



//---------------------------------------------------------------------------------
/// delegate class
/// encapsulates a callable function and its arguments
/// delegate::tuple() method returns a tuple matching the function's argument types
/// delegate should be constructed using the make_delegate functions
/// from adapted from http://codereview.stackexchange.com/questions/14730/impossibly-fast-delegate-in-c11
//---------------------------------------------------------------------------------
template <typename T> class delegate;

template<class R, class ...A>
class delegate<R (A...)>
{
  using stub_ptr_type = R (*)(void*, A&&...);

public:
  delegate(void* const o, stub_ptr_type const m) noexcept :
    object_ptr_(o),
    stub_ptr_(m)
  {
  }
  static std::tuple<A...> tuple()
  {
	  return std::tuple<A...>();
  }

  delegate() = default;

  delegate(delegate const&) = default;

  delegate(delegate&&) = default;

  delegate(::std::nullptr_t const) noexcept : delegate() { }

  template <class C, typename =
    typename ::std::enable_if< ::std::is_class<C>{}>::type>
  explicit delegate(C const* const o) noexcept :
    object_ptr_(const_cast<C*>(o))
  {
  }

  template <class C, typename =
    typename ::std::enable_if< ::std::is_class<C>{}>::type>
  explicit delegate(C const& o) noexcept :
    object_ptr_(const_cast<C*>(&o))
  {
  }

  template <class C>
  delegate(C* const object_ptr, R (C::* const method_ptr)(A...))
  {
    *this = from(object_ptr, method_ptr);
  }

  template <class C>
  delegate(C* const object_ptr, R (C::* const method_ptr)(A...) const)
  {
    *this = from(object_ptr, method_ptr);
  }

  template <class C>
  delegate(C& object, R (C::* const method_ptr)(A...))
  {
    *this = from(object, method_ptr);
  }

  template <class C>
  delegate(C const& object, R (C::* const method_ptr)(A...) const)
  {
    *this = from(object, method_ptr);
  }

  template <
    typename T,
    typename = typename ::std::enable_if<
      !::std::is_same<delegate, typename ::std::decay<T>::type>{}
    >::type
  >
  delegate(T&& f) :
    store_(operator new(sizeof(typename ::std::decay<T>::type)),
      functor_deleter<typename ::std::decay<T>::type>),
    store_size_(sizeof(typename ::std::decay<T>::type))
  {
    using functor_type = typename ::std::decay<T>::type;

    new (store_.get()) functor_type(::std::forward<T>(f));

    object_ptr_ = store_.get();

    stub_ptr_ = functor_stub<functor_type>;

    deleter_ = deleter_stub<functor_type>;
  }

  delegate& operator=(delegate const&) = default;

  delegate& operator=(delegate&&) = default;

  template <class C>
  delegate& operator=(R (C::* const rhs)(A...))
  {
    return *this = from(static_cast<C*>(object_ptr_), rhs);
  }

  template <class C>
  delegate& operator=(R (C::* const rhs)(A...) const)
  {
    return *this = from(static_cast<C const*>(object_ptr_), rhs);
  }

  template <
    typename T,
    typename = typename ::std::enable_if<
      !::std::is_same<delegate, typename ::std::decay<T>::type>{}
    >::type
  >
  delegate& operator=(T&& f)
  {
    using functor_type = typename ::std::decay<T>::type;

    if ((sizeof(functor_type) > store_size_) || !store_.unique())
    {
      store_.reset(operator new(sizeof(functor_type)),
        functor_deleter<functor_type>);

      store_size_ = sizeof(functor_type);
    }
    else
    {
      deleter_(store_.get());
    }

    new (store_.get()) functor_type(::std::forward<T>(f));

    object_ptr_ = store_.get();

    stub_ptr_ = functor_stub<functor_type>;

    deleter_ = deleter_stub<functor_type>;

    return *this;
  }

  template <R (* const function_ptr)(A...)>
  static delegate from() noexcept
  {
    return { nullptr, function_stub<function_ptr> };
  }

  template <class C, R (C::* const method_ptr)(A...)>
  static delegate from(C* const object_ptr) noexcept
  {
    return { object_ptr, method_stub<C, method_ptr> };
  }

  template <class C, R (C::* const method_ptr)(A...) const>
  static delegate from(C const* const object_ptr) noexcept
  {
    return { const_cast<C*>(object_ptr), const_method_stub<C, method_ptr> };
  }

  template <class C, R (C::* const method_ptr)(A...)>
  static delegate from(C& object) noexcept
  {
    return { &object, method_stub<C, method_ptr> };
  }

  template <class C, R (C::* const method_ptr)(A...) const>
  static delegate from(C const& object) noexcept
  {
    return { const_cast<C*>(&object), const_method_stub<C, method_ptr> };
  }

  template <typename T>
  static delegate from(T&& f)
  {
    return ::std::forward<T>(f);
  }

  static delegate from(R (* const function_ptr)(A...))
  {
    return function_ptr;
  }

  template <class C>
  using member_pair =
    ::std::pair<C* const, R (C::* const)(A...)>;

  template <class C>
  using const_member_pair =
    ::std::pair<C const* const, R (C::* const)(A...) const>;

  template <class C>
  static delegate from(C* const object_ptr,
    R (C::* const method_ptr)(A...))
  {
    return member_pair<C>(object_ptr, method_ptr);
  }

  template <class C>
  static delegate from(C const* const object_ptr,
    R (C::* const method_ptr)(A...) const)
  {
    return const_member_pair<C>(object_ptr, method_ptr);
  }

  template <class C>
  static delegate from(C& object, R (C::* const method_ptr)(A...))
  {
    return member_pair<C>(&object, method_ptr);
  }

  template <class C>
  static delegate from(C const& object,
    R (C::* const method_ptr)(A...) const)
  {
    return const_member_pair<C>(&object, method_ptr);
  }

  void reset() { stub_ptr_ = nullptr; store_.reset(); }

  void reset_stub() noexcept { stub_ptr_ = nullptr; }

  void swap(delegate& other) noexcept { ::std::swap(*this, other); }

  bool operator==(delegate const& rhs) const noexcept
  {
    return (object_ptr_ == rhs.object_ptr_) && (stub_ptr_ == rhs.stub_ptr_);
  }

  bool operator!=(delegate const& rhs) const noexcept
  {
    return !operator==(rhs);
  }

  bool operator<(delegate const& rhs) const noexcept
  {
    return (object_ptr_ < rhs.object_ptr_) ||
      ((object_ptr_ == rhs.object_ptr_) && (stub_ptr_ < rhs.stub_ptr_));
  }

  bool operator==(::std::nullptr_t const) const noexcept
  {
    return !stub_ptr_;
  }

  bool operator!=(::std::nullptr_t const) const noexcept
  {
    return stub_ptr_;
  }

  explicit operator bool() const noexcept { return stub_ptr_; }

  R operator()(A... args) const
  {
//  assert(stub_ptr);
    return stub_ptr_(object_ptr_, ::std::forward<A>(args)...);
  }


private:
  friend struct ::std::hash<delegate>;

  using deleter_type = void (*)(void*);

  void* object_ptr_;
  stub_ptr_type stub_ptr_{};

  deleter_type deleter_;

  ::std::shared_ptr<void> store_;
  ::std::size_t store_size_;

  template <class T>
  static void functor_deleter(void* const p)
  {
    static_cast<T*>(p)->~T();

    operator delete(p);
  }

  template <class T>
  static void deleter_stub(void* const p)
  {
    static_cast<T*>(p)->~T();
  }

  template <R (*function_ptr)(A...)>
  static R function_stub(void* const, A&&... args)
  {
    return function_ptr(::std::forward<A>(args)...);
  }

  template <class C, R (C::*method_ptr)(A...)>
  static R method_stub(void* const object_ptr, A&&... args)
  {
    return (static_cast<C*>(object_ptr)->*method_ptr)(
      ::std::forward<A>(args)...);
  }

  template <class C, R (C::*method_ptr)(A...) const>
  static R const_method_stub(void* const object_ptr, A&&... args)
  {
    return (static_cast<C const*>(object_ptr)->*method_ptr)(
      ::std::forward<A>(args)...);
  }

  template <typename>
  struct is_member_pair : std::false_type { };

  template <class C>
  struct is_member_pair< ::std::pair<C* const,
    R (C::* const)(A...)> > : std::true_type
  {
  };

  template <typename>
  struct is_const_member_pair : std::false_type { };

  template <class C>
  struct is_const_member_pair< ::std::pair<C const* const,
    R (C::* const)(A...) const> > : std::true_type
  {
  };

  template <typename T>
  static typename ::std::enable_if<
    !(is_member_pair<T>{} ||
    is_const_member_pair<T>{}),
    R
  >::type
  functor_stub(void* const object_ptr, A&&... args)
  {
    return (*static_cast<T*>(object_ptr))(::std::forward<A>(args)...);
  }

  template <typename T>
  static typename ::std::enable_if<
    is_member_pair<T>{} ||
    is_const_member_pair<T>{},
    R
  >::type
  functor_stub(void* const object_ptr, A&&... args)
  {
    return (static_cast<T*>(object_ptr)->first->*
      static_cast<T*>(object_ptr)->second)(::std::forward<A>(args)...);
  }
};

namespace std
{
  template <typename R, typename ...A>
  struct hash<::delegate<R (A...)> >
  {
    size_t operator()(::delegate<R (A...)> const& d) const noexcept
    {
      auto const seed(hash<void*>()(d.object_ptr_));

      return hash<typename ::delegate<R (A...)>::stub_ptr_type>()(
        d.stub_ptr_) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
  };
}

/**
 * function to deduce template parameters from call-context
 */
template <typename C, typename R, typename... A>
delegate<R(A...)> make_delegate(C* const object_ptr, R (C::* const method_ptr)(A...))
{
    return delegate<R(A...)>(object_ptr,method_ptr);
}

/**
 * function to deduce template parameters from object method
 */
template <typename C, typename R, typename... A>
delegate<R(A...)> make_delegate(C* const object_ptr, R (C::* const method_ptr)(A...) const)
{
    return delegate<R(A...)>(object_ptr,method_ptr);
}



/**
 * function to deduce template parameters from function pointer
 */
template <class R, class... Args>
delegate<R(Args...)> make_delegate(R(*func)(Args...))
{
    return delegate<R(Args...)>(func);
}


/**
 * function to deduce template parameters from std::function
 */
template <class R, class... Args>
delegate<R(Args...)> make_delegate(std::function<R(Args...)> func )
{
    return delegate<R(Args...)>(func);
}
/**
 * function to deduce template parameters from call-context
 *
 */

// Helpers to convert lambda into std::function
//(https://gist.github.com/VictorLaskin/1b973014b1c3aeab228e)

template <typename Function>
struct function_traits
: public function_traits<decltype(&Function::operator())>
{};

template <typename ClassType, typename ReturnType, typename... Args>
struct function_traits<ReturnType(ClassType::*)(Args...) const>
{
	typedef ReturnType (*pointer)(Args...);
	typedef std::function<ReturnType(Args...)> function;
};

template <typename Function>
typename function_traits<Function>::function
to_function (Function& lambda)
{
	return static_cast<typename function_traits<Function>::function>(lambda);
}

template <class F>
auto make_delegate(F const & func )
{
	// convert lambda to std::function
    return make_delegate(to_function(func));
}



