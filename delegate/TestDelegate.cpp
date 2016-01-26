
#define CATCH_CONFIG_MAIN
#include <libs/catch/catch.hpp>
#include <libs/delegate/Delegate.hpp>
#include <libs/delegate/Json.hpp>


int int_string_function(int a, std::string b)
{
	return a + b.size();
}

struct dummy {
	int d(int a, std::string b) {return a + b.size() + c;}
	int d_const(int a, std::string b) const {return a + b.size() + c;}

	int c = 5;
};
dummy dum;


template <class F, class... Args>
void for_each_argument(F f, Args&&... args) {
    (void)(int[]){(f(std::forward<Args>(args)), 0)...};
}

SCENARIO( "A delegate can be made from a member function", "[member_function]" ) {

	int mm = 8;

	auto member_int_string = make_delegate(&dum,&dummy::d);

	auto member_int_string_const = make_delegate(&dum,&dummy::d_const);

	REQUIRE(member_int_string(2,"bye") == 10) ;
	REQUIRE(member_int_string_const(2,"bye") == 10) ;

}


SCENARIO( "A delegate can be made from lambda expression", "[lambda]" ) {

	int mm = 8;
	auto lambda_int_string = make_delegate([mm](int a, std::string b) {
		return a + mm;
	});

	REQUIRE(lambda_int_string(2,"bye") == 10) ;
}


SCENARIO( "A delegate can be made from free function", "[free_function]" ) {

	int mm = 8;
	auto free_int_string = make_delegate(&int_string_function);
	REQUIRE(free_int_string(2,"bye") == 5) ;
}

SCENARIO( "A json_function can be made from free lambda", "[json_function]" ) {

	auto test_function = make_delegate([](int i, std::string s, bool b, float f,double ss) {
		std::string out = std::to_string(i) + " " + s + (b ? " true " : " false ") + std::to_string(f) + " " + std::to_string(ss);
		return out;
	});

	Json::Value in;
	in.append(8);
	in.append("Monkey");
	in.append(true);
	in.append(4.2);
	in.append(8.4);
	auto json_test_function = make_json_function(test_function);

	REQUIRE(json_test_function(in).asString() == "8 Monkey true 4.200000 8.400000");
}

template<typename ostream>
class  streamer
{
public:
	streamer(ostream & o) :_ostream(o){}

    template<typename R, typename... Args>
    delegate<R(Args...)> operator () (delegate<R(Args...)> f) const
    {
    	return [this,f](Args... args)
    	{
    		std::cout << std::endl << ">>>> Streaming(cout) " << std::endl;
			_ostream << std::make_tuple(args...);
			auto ret = f(args...);
			_ostream <<" -> " << ret;
			std::cout << std::endl << "<<<< Streaming";
			return ret;
    	};
	};
    ostream & _ostream;
};

class  repeater
{
public:
	repeater(size_t n) :_ntimes(n) {}
    template<typename R, typename... Args>
    delegate<R(Args...)> operator () (delegate<R(Args...)> f) const
    {
    	return [this,f](Args... args)
    	{
			std::cout <<  std::endl <<  ">>>> Repeating(" << std::to_string(_ntimes) << ") ";
			if(_ntimes > 10) {
				throw std::runtime_error("Too many times");
			}
			for(auto i = 0; i < _ntimes; i++) {
			    f(args...);
			}
			std::cout <<  std::endl <<  "<<< Repeating ";
			return f(args...);
    	};
    }
    size_t _ntimes;
};


class  counter
{
public:
    template<typename R, typename... Args>
    delegate<R(Args...)> operator () (delegate<R(Args...)>  f)
    {
    	return [this,f](Args... args)
    	{
			std::cout <<  std::endl << ">>>> Counting(" << std::to_string(_count) << ") ";
			_count++;
			auto r = f(args...);

			std::cout <<  std::endl << "<<<< Counting ";
			return r;
    	};
    }
    int _count{0};
    counter(){std::cout<< std::endl << "ALLOCATED" << std::endl;}
    ~counter(){std::cout << std::endl << "DALLOCATED" << std::endl;}
};


class  printer
{
public:
    template<typename R, typename... Args>
    delegate<R(Args...)> operator () (delegate<R(Args...)> f) const
    {
    	return [f](Args... args)
    	{
			auto ret = f(args...);
			std::cout << "[" << ret << "]";
			return ret;
    	};
    }
};



SCENARIO( "A API can be made from json_function", "[API]" ) {


	auto test_function1 = make_delegate([](int i, std::string s,double ss) {
		return i + s.size() + ss;
	});

	auto test_function2 = make_delegate([](int i, std::string s, bool b, float f,double ss) {
		std::string out = std::to_string(i) + " " + s + (b ? " true " : " false ") + std::to_string(f) + " " + std::to_string(ss);
		return out;
	});

	auto test_function3 = make_delegate(&dum,&dummy::d);

	GIVEN("A json function") {

		auto int_to_string = [](int a) { return std::to_string(a); };
		auto json_int_to_string = make_json_function(int_to_string);

		WHEN("json function is called with invalid arguements") {
			THEN("the call is ignored") {
				Json::Value json_in;
				json_in.append("helloo");
				auto json_out = json_int_to_string(json_in);
				REQUIRE(json_out.isNull());
			}
		}
	}

	JsonFunctions funcs;
	funcs.add_function("test1",test_function1);
	funcs.add_function("test2",test_function2);
	funcs.add_function("int_string",&int_string_function);
	funcs.add_function("lambda1",[](std::string s, double d) {
		return d + s.size();
	});
	funcs.add_function("int_string_member",test_function3);
	funcs.add_function("int_string_member2",&dum,&dummy::d);
	funcs.add_function("int_string_member2_const",&dum,&dummy::d_const);

	auto json_out = funcs.call_from_string("test1",R"([9,"abcd",1.5])");
	REQUIRE(json_out.asFloat() == 14.5);

	json_out = funcs.call_from_string("test2",R"([9,"abcd",true,1.5,2.3])");
	REQUIRE(json_out.asString() == "9 abcd true 1.500000 2.300000");

	json_out = funcs.call_from_string("int_string_member2",R"([2,"bye"])");
	REQUIRE(json_out.asInt() == 10);

	auto api = API(funcs);

}


SCENARIO( "A tuple of functions can be iterated on", "[function_tuple]" ) {


	auto test_function1 = make_delegate([](int i, std::string s,double ss) {
		return i + s.size() + ss;
	});

	auto test_function2 = make_delegate([](int i, std::string s, bool b, float f,double ss) {
		std::string out = std::to_string(i) + " " + s + (b ? " true " : " false ") + std::to_string(f) + " " + std::to_string(ss);
		return out;
	});

	auto test_function3 = make_delegate(&dum,&dummy::d);

	auto test_functions = std::make_tuple(
			test_function1
			,test_function2
			,test_function3
			,make_delegate(&int_string_function)
	);

	for_each(test_functions,streamer<std::ostream>(std::cout));
	for_each(test_functions,counter());
	//TODO: fix me for_each(test_functions,repeater(4));
	auto r = repeater(3);
	for_each(test_functions,r);

	//std::get<0>(test_functions) = c(std::get<0>(test_functions));
	//std::get<1>(test_functions) = c(std::get<1>(test_functions));
	//std::get<2>(test_functions) = c(std::get<2>(test_functions));

	for_each(test_functions,printer());
	std::cout << std::endl << "START------------ " << std::endl;
	std::get<0>(test_functions)(2,"kill",4.0);
	std::get<1>(test_functions)(1,"a",true,2.0,4.5);
	std::get<2>(test_functions)(1,"a");
	std::cout << std::endl << "FINISH -----------" << std::endl;
/*
	std::cout << std::endl << "START------------ " 					<< std::endl;
	std::cout << std::get<0>(test_functions)(2,"kill",4.0) 			<< std::endl;
	std::cout << std::get<1>(test_functions)(1,"a",true,2.0,4.5)	<< std::endl;
	std::cout << std::get<2>(test_functions)(1,"a")					<< std::endl;
	std::cout << std::endl << "FINISH -----------" 					<< std::endl;
*/

}

