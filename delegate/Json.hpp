#include <stdio.h>
#include <string>
#include <iostream>
#include <jsonrpccpp/server.h>
#include <jsonrpccpp/server/connectors/httpserver.h>
#include <jsonrpccpp/server/connectors/unixdomainsocketserver.h>

// ------------- convert Json::Value to fundamental type---
template <class T> T as(const Json::Value & v);

template<> bool as<bool>(const Json::Value & v) { return v.asBool(); };
template<> int as<int>(const Json::Value & v) { return v.asInt(); };
template<> float as<float>(const Json::Value & v) { return v.asFloat(); };
template<> double as<double>(const Json::Value & v) { return v.asDouble(); };
template<> std::string as<std::string>(const Json::Value & v) { return v.asString(); };

// ---------------- compare Json:Value to fundamental type ---
template <class T> bool is(const Json::Value & v);

template<> bool is<bool>(const Json::Value & v) { return v.isBool(); };
template<> bool is<int>(const Json::Value & v) { return v.isInt(); };
template<> bool is<double>(const Json::Value & v) { return v.isDouble() || v.isInt(); };
template<> bool is<float>(const Json::Value & v) { return is<double>(v); };
template<> bool is<std::string>(const Json::Value & v) { return v.isString(); };

// ---------------- convert fundamental type's type to fundamental string ---
template <class T> std::string type();

template<> std::string type<bool>() { return "bool"; };
template<> std::string type<int>() { 	return "int"; };
template<> std::string type<float>() { return "float"; };
template<> std::string type<double>() { return "double"; };
template<> std::string type<std::string>() { return "string"; };

// ------- compare Json::Value array to tuple of fundamental types
template<std::size_t> struct int_{};

// tuple_element_t appears in C++14.  defined here for use in C++11
namespace std {
    template<std::size_t I, class T>
    using tuple_element_t = typename tuple_element<I, T>::type;
}

template <typename Tuple, size_t Pos>
bool is_json_tuple2(const Json::Value& mV, Tuple & mX, int_<Pos>)
{
	auto val = mV[Json::ArrayIndex(std::tuple_size<Tuple>::value - Pos)];
	if(!is<std::tuple_element_t<std::tuple_size<Tuple>::value - Pos,Tuple>>(val)) {

		std::stringstream ss; ss << type<std::tuple_element_t<std::tuple_size<Tuple>::value - Pos,Tuple>>();
		std::cout << ("Invalid Argument " + std::to_string(std::tuple_size<Tuple>::value - Pos) + " Json: " + val.toStyledString() + " Expected: " + ss.str() );
		return false;
	}
    return is_json_tuple2(mV, mX, int_<Pos-1>());
}

template <typename Tuple>
bool is_json_tuple2(const Json::Value & mV, Tuple & mX, int_<1>)
{
	auto val = mV[Json::ArrayIndex(std::tuple_size<Tuple>::value - 1)];
	if(!is<std::tuple_element_t<std::tuple_size<Tuple>::value - 1,Tuple>>(val)) {

		std::stringstream ss; ss << type<std::tuple_element_t<std::tuple_size<Tuple>::value - 1,Tuple>>();
		std::cout << ("Invalid Argument " + std::to_string(std::tuple_size<Tuple>::value - 1) + " Json: " + val.toStyledString() + " Expected: " + ss.str() );
		return false;
	}
	return true;
}


template <typename... Args>
bool is_tuple(const Json::Value& mV, std::tuple<Args...>& mX)
{
    return (is_json_tuple2(mV, mX, int_<sizeof...(Args)>{}));
}


// ------- convert Json::Value array to tuple of fundamental types
template <typename Tuple, size_t Pos>
bool json_to_tuple2(const Json::Value& mV, Tuple & mX, int_<Pos>)
{
	auto val = mV[Json::ArrayIndex(std::tuple_size<Tuple>::value - Pos)];
	if(!is<std::tuple_element_t<std::tuple_size<Tuple>::value - Pos,Tuple>>(val)) {

		std::stringstream ss; ss << type<std::tuple_element_t<std::tuple_size<Tuple>::value - Pos,Tuple>>();
		std::cout << ("Invalid Argument " + std::to_string(std::tuple_size<Tuple>::value - Pos) + " Json: " + val.toStyledString() + " Expected: " + ss.str() );
		return false;
	}
	std::get<std::tuple_size<Tuple>::value - Pos>(mX) = as<std::tuple_element_t<std::tuple_size<Tuple>::value - Pos,Tuple>>(val);
    return json_to_tuple2(mV, mX, int_<Pos-1>());
}

template <typename Tuple>
bool json_to_tuple2(const Json::Value & mV, Tuple & mX, int_<1>)
{
	auto val = mV[Json::ArrayIndex(std::tuple_size<Tuple>::value - 1)];
	if(!is<std::tuple_element_t<std::tuple_size<Tuple>::value - 1,Tuple>>(val)) {

		std::stringstream ss; ss << type<std::tuple_element_t<std::tuple_size<Tuple>::value - 1,Tuple>>();
		std::cout << ("Invalid Argument " + std::to_string(std::tuple_size<Tuple>::value - 1) + " Json: " + val.toStyledString() + " Expected: " + ss.str() );
		return false;
	}
	std::get<std::tuple_size<Tuple>::value - 1>(mX) = as<std::tuple_element_t<std::tuple_size<Tuple>::value - 1,Tuple>>(val);
	return true;
}


template <typename... Args>
bool json_to_tuple(const Json::Value& mV, std::tuple<Args...>& mX)
{
	if(!is_tuple(mV,mX)) {
		std::cout << std::endl << "json_to_tuple(from: " << mV << "to: " << mX << ") failed.  Json array does not match tuple types" << std::endl;
		return false;
	}

    return json_to_tuple2(mV, mX, int_<sizeof...(Args)>{});
}


// ------- convert tuple of fundamental types to Json::Value array

template <class Tuple, size_t Pos>
bool json_tuple_out(Json::Value & out, const Tuple& t, int_<Pos> ) {
  out.append(std::get< std::tuple_size<Tuple>::value-Pos >(t));
  return json_tuple_out(out, t, int_<Pos-1>());
}

template <class Tuple>
bool json_tuple_out(Json::Value& out, const Tuple& t, int_<1> ) {
   out.append(std::get<std::tuple_size<Tuple>::value-1>(t));
   return true;
}

template <class... Args>
bool tuple_to_json(const std::tuple<Args...>& t,Json::Value & out) {
  return json_tuple_out(out, t, int_<sizeof...(Args)>());
}


//------print tuple ---
template <class Tuple, size_t Pos>
std::ostream& print_tuple_out(std::ostream& out, const Tuple& t, int_<Pos> ) {
  out << std::get< std::tuple_size<Tuple>::value-Pos >(t) << ',';
  return print_tuple_out(out, t, int_<Pos-1>());
}

template <class Tuple>
std::ostream& print_tuple_out(std::ostream& out, const Tuple& t, int_<1> ) {
  return out << std::get<std::tuple_size<Tuple>::value-1>(t);
}

template <class... Args>
std::ostream& operator<<(std::ostream& out, const std::tuple<Args...>& t) {
  out << '(';
  print_tuple_out(out, t, int_<sizeof...(Args)>());
  return out << ')';
}



// ------- apply function to tuple -------
/*
// TODO Use this apply method after upgrading to C++14
// This is adapted from the C++14 draft using index_sequence.
template<typename F, typename Tuple, size_t ...S >
auto apply_tuple_impl(F&& fn, Tuple&& t, std::index_sequence<S...>)
{
	return std::forward<F>(fn)(std::get<S>(std::forward<Tuple>(t))...);
}
template<typename F, typename Tuple>
auto apply(F&& fn, Tuple&& t)
{
	std::size_t constexpr tSize
		= std::tuple_size<typename std::remove_reference<Tuple>::type>::value;
	return apply_tuple_impl(std::forward<F>(fn),
	                        std::forward<Tuple>(t),
	                        std::make_index_sequence<tSize>());
}
*/
// Helper to emulate C++14's std::index_sequence
// from http://vitiy.info/templates-as-first-class-citizens-in-cpp11/#more-524
namespace fn_detail {

        template<int ...>
        struct int_sequence {};

        template<int N, int ...S>
        struct gen_int_sequence : gen_int_sequence<N-1, N-1, S...> {};

        template<int ...S>
        struct gen_int_sequence<0, S...> {
            typedef int_sequence<S...> type;
        };

        template <typename F, typename... Args, int... S>
        inline auto fn_tuple_apply(int_sequence<S...>, const F& f, const std::tuple<Args...>& params) -> decltype( f((std::get<S>(params))...) )
        {
            return f((std::get<S>(params))...);
        }

}

template <typename F, typename... Args>
inline auto apply(const F& f, const std::tuple<Args...>& params) -> decltype( f(std::declval<Args>()...) )
{
    return fn_detail::fn_tuple_apply(typename fn_detail::gen_int_sequence<sizeof...(Args)>::type(), f, params);
}



// ------- convert function to function that takes Json::Value and returns Json::Value ----

template <typename F>
auto make_json_function(F && f)
{
	return [f](const Json::Value & json_in) {
		auto args_tuple = make_delegate(f).tuple();
		if(!json_to_tuple(json_in,args_tuple)) {
			std::stringstream ss; ss << "[apply_json] invalid arguments: json = " << json_in << " tuple = " << args_tuple;
			//throw std::invalid_argument( ss.str());
			std::cout << ss.str();
			return Json::Value();
		}
		auto ret = apply(f,args_tuple);
		return Json::Value(ret);
	};
}

// ------- maintains a mapping from function name to json_function ----

struct JsonFunctions
{
	using json_function = delegate<Json::Value(const Json::Value&)>;
	std::map<std::string,json_function> _functions;
	std::map<std::string,std::string> _parameters;

	template<typename F>
	void add_function(std::string name, F && f ) {
		_functions[name] = make_json_function(f);
		Json::Value tuple_json;
		auto parameters_tuple = make_delegate(f).tuple();
		tuple_to_json(parameters_tuple,tuple_json);
		_parameters[name] = tuple_json.toStyledString();
	}

	template<typename C, typename F>
	void add_function(std::string name, C* const obj, F && f ) {
		add_function(name,make_delegate(obj,f));
	}

	bool contains(std::string name) const {
		return _functions.find(name) != _functions.end();
	}

	Json::Value call_from_string(std::string name, std::string args) const {
		Json::Value val;
		Json::Reader reader;
		if(!reader.parse(args,val)) {
			throw std::invalid_argument("Could not parse arguments as Json array: " + args);
		}
		return call(name,val);
	}

	Json::Value call(std::string name, const Json::Value & args) const {
		//std::cout << std::endl << "CALLING " << name << " ARGS " << args << std::endl;
		return _functions.at(name)(args);
	}

	Json::Value functions() const {
		Json::Value out;
		for (auto & f : _functions) {
			out[f.first] = _parameters.at(f.first);
		}
		return out;
	}
};

// --------------JsonRPCServer that host's json_functions--------------------
class JsonFunctionServer : public jsonrpc::AbstractServer<JsonFunctionServer>
{
    public:
        JsonFunctionServer(jsonrpc::HttpServer &server,JsonFunctions & funcs)
        :jsonrpc::AbstractServer<JsonFunctionServer>(server)
		,_json_funcs(funcs)
        {
            this->bindAndAddMethod(jsonrpc::Procedure("envoke", jsonrpc::PARAMS_BY_NAME, jsonrpc::JSON_STRING, "function", jsonrpc::JSON_STRING, NULL), &JsonFunctionServer::call);
            this->bindAndAddMethod(jsonrpc::Procedure("functions", jsonrpc::PARAMS_BY_NAME, jsonrpc::JSON_STRING, NULL), &JsonFunctionServer::functions);
        }

        //method
        void call(const Json::Value& request, Json::Value& response)
        {
        	std::cout << "JsonFunctionServer::call REQ: " << request << " RES: " << response << std::endl;
        	auto func = static_cast<const Json::Value>(request)["__args"][0].asString();

        	if(!_json_funcs.contains(func)) {
        		std::cout << ("[CALL][IGNORED] Function named "+ func +" not found")  << std::endl;
        		return;
        	}

        	auto parameters = static_cast<const Json::Value>(request)["function"].asString();
        	response = _json_funcs.call_from_string(func,parameters);
        	std::cout << "JsonFunctionServer::call REQ: " << request << " RES: " << response << std::endl;
        }

        void functions(const Json::Value & request, Json::Value& response)
        {
        	response = _json_funcs.functions().toStyledString();
        }
        JsonFunctions & _json_funcs;
};

class API {

public:
	API(JsonFunctions & functions, std::string name = "NoName",int port = 8383)
	:_http_server(port)
	,_json_server(_http_server,functions)
	{
		if(_json_server.StartListening())
	    {
	        std::cout << "Server started successfully" << std::endl;
	        getchar();
	        _json_server.StopListening();
	    }
	    else
	    {
	        std::cout << "Error starting Server" << std::endl;
	    }
	}

	~API() {_json_server.StopListening();}

private:
	jsonrpc::HttpServer _http_server{8383};
	JsonFunctionServer _json_server;
};



template<typename TupleType, typename FunctionType>
void for_each(TupleType&&, FunctionType &&
            , std::integral_constant<size_t, std::tuple_size<typename std::remove_reference<TupleType>::type >::value>)
{
	//std::cout << std::endl << "in foreach3" << std::endl;
}

template<std::size_t I, typename TupleType, typename FunctionType
       , typename = typename std::enable_if<I!=std::tuple_size<typename std::remove_reference<TupleType>::type>::value>::type >
void for_each(TupleType&& t, FunctionType && f, std::integral_constant<size_t, I>)
{
    std::get<I>(t) = f(std::get<I>(t));
	//std::cout << std::endl << "in foreach2 " << I << std::endl;
    for_each(std::forward<TupleType>(t), f, std::integral_constant<size_t, I + 1>());
}

template<typename TupleType, typename FunctionType>
void for_each(TupleType&& t, FunctionType && f)
{
	//std::cout << std::endl << "in foreach1" << std::endl;
    for_each(std::forward<TupleType>(t), f, std::integral_constant<size_t, 0>());
}
