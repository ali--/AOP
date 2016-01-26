
#pragma once
#include <libs/logger/Logger.hpp>
#include <jsoncpp/json/json.h>
#include <fstream>
#include <atomic>
#include <string>
#include <sstream>
#include <memory>
#include <utility>
#include <algorithm>
#include <boost/optional.hpp>



namespace momentum {

class RPCServerBase;


/// ParameterBase class for parameters that can be
/// serialized and deserialized into json strings.
/// A parameter has:
///		- name, and description.
/// 	- "editable" property, a boolean that if true the
/// 		parameter cannot be modified at runtime
/// 		but can be read from a file.
/// 	- optionally an owner (of class Parameters)
/// 		representing a container of related parameters.
/// 	- to_json(), from_json() methods
/// 	- dump() method for human-readable info
///
class ParameterBase
{

public:

	/// The name of the parameter.
	/// @return name
	std::string name() const {return _name;}

	/// The description of the parameter.
	/// @return description
	std::string description() const {return _description;}

	/// Is the parameter editable at runtime?
	/// @return true if parameter is editable.
	bool editable() const { return _editable; }

	/// Human-readable information of the parameter.
	/// @return string representation of parameter for debugging
	virtual std::string dump() const = 0;

	/// Structured, detailed information of the parameter.
	/// Provides json containing the attributes of the parameter.
	/// @param [in] output_attributes list of desired attributes to be returned
	/// @return j representation of parameter in json form
	virtual std::string json_string(std::vector<std::string> output_attributes = {}) const = 0;

	/// Structured, detailed information of the parameter.
	/// Provides schema of the parameter.
	/// @return detailed representation of parameter in json form
	virtual bool from_json_string(std::string in) = 0;

	/// Type of the parameter.
	/// Provides a string that contains the "type" of the parameter
	/// e.g "float", "bool", "choice", etc...
	/// @return the parameter's type
	virtual std::string type() const = 0 ;

	std::function<void(const std::string &)> on_error{[](const std::string & msg) {
		MMLogger("UnregisteredParameter").error(msg);
	}};

	std::function<void(const std::string &)> on_info{[](const std::string & msg) {
		MMLogger("UnregisteredParameter").verbose(0,msg);
	}};


	enum class Flags {
		DEFAULT = 0,
		STATIC,
		DYNAMIC,
	};
protected:

	/// Default Constructor.
	/// @param [in] name the name of the parameter
	/// @param [in] description the description of the parameter
	/// @param [in] editable true if the parameter should be modifiable at runtime.
	/// @warning name should NEVER contain a forward slash "/"
	///
	ParameterBase(std::string name, std::string description, Flags editable = Flags::DYNAMIC)
	:_name(name)
	,_description(description)
	,_editable(editable == Flags::DYNAMIC ? true : false)
	{}

	virtual ~ParameterBase() {};

	/// Log an error message.
	/// Used when unable to assign value for any reason.
	/// Will automatically provide name of parameter in logged message.
	/// May be used when:
	/// 	- attempting to convert from invalid json object
	/// 	- attempting to assigning invalid value (i.e out of bound)
	/// 	- attempting to assign non-editable parameter
	/// @param [in] msg The error message.
	void ERROR(const std::string & msg) const
	{
		std::string error_msg = "[ERROR]["+name()+"] " + msg;
		on_error(error_msg);
	}

	/// Log an info message.
	/// Used when correctly assigning parameter to new value.
	/// @param [in] msg The info message.
	void INFO(const std::string & msg) const
	{
		std::string info_msg = "["+name()+"] " + msg;
		on_info(info_msg);
	}

 	virtual std::map<std::string,std::string> attributes() const {
 		return {{"description",Json::valueToQuotedString(description().c_str())}
 			   ,{"editable",Json::valueToString(editable())}
 			   ,{"type",Json::valueToQuotedString(type().c_str())}
 		};
	}



private:

    ParameterBase( const ParameterBase& other ) = delete; // non construction-copyable
    ParameterBase& operator=( const ParameterBase& ) = delete; // non copyable

	std::string _name{"UNNAMED"};
	std::string _description{"UNKNOWN"};
	bool _editable{true};

};

typedef ParameterBase::Flags Flags;

//-------------------------------------
// callbacks to check if new value is valid
//  returns msg if not valid
//-------------------------------------
template<typename T>
using ChangeIsValidFunction = std::function<bool(T, std::string &)>;

template<typename T>
static bool ChangeIsValidDefault(T t, std::string & msg){return true;};

//-------------------------------------
// callbacks to convert value to string
//  returns string representation of value
//-------------------------------------
template<typename T>
using ValueToStringFunction = std::function<std::string(T)>;

template<typename T>
static std::string ValueToStringDefault(T t){return Json::valueToString(t);};

//-------------------------------------
// callbacks to convert value from string
//  returns true if representation was valid
//  changes the value of val
//-------------------------------------
/// Convert from string
template<typename T>
using ValueFromStringFunction = std::function<bool(std::string i, T & val)>;

template<typename T>
static bool ValueFromStringDefault(const std::string i , T & val) throw(std::logic_error) {
	Json::Value json;
	Json::Reader reader;
	if (!reader.parse(i,json)) {
		throw std::logic_error("[value_from_string] Unable to parse string as json: " + i );
		return false;
	}

	if (json.isDouble()) {
		val = json.asDouble();
	} else if (json.isInt()) {
		val = json.asInt();
	} else if (json.isBool()) {
		val = json.asBool();
	} else {
		throw std::logic_error("[value_from_string] Unknown Data Type for json value: " + json.toStyledString() );
		return false;
	}
	return true;
}

template <typename Type>
class Parameter : public ParameterBase
{

public:

    /// Initialized constructor
    Parameter(std::string name, Type initial_value, std::string description, Flags flags = Flags::DEFAULT
    		,ChangeIsValidFunction<Type> change_is_valid = ChangeIsValidDefault<Type>
    		,ValueToStringFunction<Type> value_to_string = ValueToStringDefault<Type>
    		,ValueFromStringFunction<Type> value_from_string = ValueFromStringDefault<Type>
    		)
    :ParameterBase(name,description,flags)
	,_change_is_valid(change_is_valid)
	,_value_to_string(value_to_string)
	,_value_from_string(value_from_string)
    {
   	 if (!change_value(initial_value)) {
   		 throw std::logic_error("[Invalid Initialization]["+ParameterBase::name()+"] Attempt to initialize to a invalid value: " + value_to_string(initial_value));
   	 }
    }

    /// Uninitialized constructor
    Parameter(std::string name, std::string description, Flags flags = Flags::DEFAULT
    		,ChangeIsValidFunction<Type> change_is_valid = ChangeIsValidDefault<Type>
    		,ValueToStringFunction<Type> value_to_string = ValueToStringDefault<Type>
			,ValueFromStringFunction<Type> value_from_string = ValueFromStringDefault<Type>)
    :ParameterBase(name,description,flags)
    ,_change_is_valid(change_is_valid)
    ,_value_to_string(value_to_string)
	,_value_from_string(value_from_string)
    {}


	 /// Convert to Type
	operator Type() const { return value(); }

	/// Check if value is equal
	template<typename OtherType>
	bool operator==(const OtherType &other) const {
		return value() == Type(other);
	}

    virtual std::string dump() const override
    {
    	auto attrs = attributes();
    	std::string dump_string;
    	for (auto & attr : attrs) {
    		dump_string += "|" + attr.first + " = " + attr.second;
    	}
 		return "["+ParameterBase::name()+"]" + dump_string;
 	}

    virtual std::string json_string(std::vector<std::string> output_attributes = {}) const override
    {
    	auto attrs = attributes();
    	Json::Value json_out;

    	for (auto & attr : attrs)
    	{
    		// if caller requested specific attributes, only provide requested ones
    		if (output_attributes.size()
    				&& (std::find(output_attributes.begin()
    						,output_attributes.end(),attr.first)
    					== output_attributes.end())) {
    			continue;
    		}
         	Json::Value json;
         	Json::Reader reader;

         	if (!reader.parse(attr.second,json)) {
         		throw std::runtime_error("[ERROR] Unable to parse string to json: " + attr.second);
         	}

    		json_out[attr.first] = json;
    	}
    	Json::Value named_json;
    	named_json[ParameterBase::name()] = json_out;
 		return named_json.toStyledString();
 	}

	/// Type of the parameter.
	/// Provides a string that contains the "type" of the parameter
	/// e.g "float", "bool", "choice", etc...
	/// @return the parameter's type
	virtual std::string type() const override { return "UNKNOWN"; };

	std::string value_to_string() const {
		return _value_to_string(value());
	}

    /// Convert from string
    bool value_from_string(const std::string& i ) throw(std::logic_error) {
    	Type new_value;
    	if(!_value_from_string(i,new_value)) {
    		std::string msg = "[Read] Unable to convert from string: " + i;
    		ERROR(msg);
    		throw std::logic_error(msg);
    		return false;
    	}
		std::string message = "[Read] read value from " + i;
		INFO(message);
		change_value(new_value);
    	return true;
    }


	/// Structured, detailed information of the parameter.
	/// Provides schema of the parameter.
	/// @return detailed representation of parameter in json form
	virtual bool from_json_string(std::string in) override {
			Json::Value json;
         	Json::Reader reader;
         	if (!reader.parse(in,json)) {
         		std::string msg("[from_json_string][FAILED] Unable to parse string to json: " + in);
         		ERROR(msg);
         		return false;
         	}

        	Json::Value named_section = static_cast<const Json::Value>(json)[name()];

        	if (named_section.isNull())
        	{
        		std::string msg("[from_json_string][FAILED] No section for " + name() + " found in " + json.toStyledString());
        		ERROR(msg);
        		return false;
        	}

           	Json::Value value = static_cast<const Json::Value>(named_section)["value"];

			if (value.isNull())
			{
				std::string msg("[from_json_string][FAILED] No value for " + name() + " found in " + json.toStyledString());
				ERROR(msg);
				return false;
			}

			return value_from_string(value.toStyledString());
	};

    // called when value changes
    std::function<void(Type)> on_change{[](Type t){return;}};


	virtual ~Parameter() {};

protected:

     /// Assignment operator.
     template<typename OtherType>
     Parameter & operator=(const OtherType &other) {
    	 change_value( Type(other) );
    	 return *this;
     }


  	virtual std::map<std::string,std::string> attributes() const override {
  		auto attributes = ParameterBase::attributes();
  		if (initialized()) {
  	 		attributes["default"] = _value_to_string(_initial_value.get());
  	  		attributes["value"] = _value_to_string(value());
  		}
  		return attributes;
 	}

     Parameter& operator=( const Parameter& ) = delete; // non copyable

private:

     Parameter( const Parameter& other ) = delete; // non construction-copyable

     bool initialized() const { return _initial_value.is_initialized(); }

     Type value() const throw(std::logic_error) {
    	 if (!initialized()) {
    		 throw std::logic_error("[Uninitialized Value]["+name()+"] Attempt to access uninitialized value");
    	 }
		return _value;
     }


     bool change_value(const Type & to)
     {
    	 std::string msg("Unknown");
    	 if (!_change_is_valid(to,msg)) {
			 std::string message = "[Change Ignored] to " + _value_to_string(to) + " Reason: " + msg;

			 ERROR(message);
        	 return false;
    	 }

		 if (!initialized()) {

			 std::string message = "[Initialize] to " + _value_to_string(to);
			 _value = to;
			 _initial_value = to;

			 INFO(message);

		 } else if (to == value()) {
			 // Do nothing if new value is same

		 } else {

			 std::string message = "[Changed] from " + _value_to_string(value())
			 + " to " + _value_to_string(to);
			 _value = to;

			 INFO(message);
		 }

		 on_change(to);

		 return true;

     }



     /// This function will be called before any change to the value of this
     /// attribute happens. If it returns false, no change occurs. */
     ChangeIsValidFunction<Type> _change_is_valid;			// checks if change is valid
     ValueToStringFunction<Type> _value_to_string;			// converts value to string
     ValueFromStringFunction<Type> _value_from_string;		// converts string to value

     std::atomic<Type> _value;
     boost::optional<Type> _initial_value{boost::none};
};


class MMBool : public Parameter<bool>
{
public:
	 using Parameter<bool>::operator=;
	 using Parameter<bool>::Parameter;

	std::string type() const override { return "bool"; }
};


template <typename T>
class MMNumber : public Parameter<T> {
public:
	 using Parameter<T>::operator=;

	MMNumber(std::string name, T value, std::string description, T min, T max, Flags flags = Flags::DEFAULT)
	:Parameter<T>(name,value,description,flags,
					[min,max](T val,std::string & msg){ return change_is_valid(val, min,max,msg); })
	,_min(min)
	,_max(max)
	{
		if (min > max )
		{
			std::string msg = "["+Parameter<T>::name()+"] minimum is larger than maximum.";
			throw std::logic_error(msg);
		}
	}


	MMNumber(std::string name, std::string description, T min, T max, Flags flags = Flags::DEFAULT)
	:Parameter<T>(name,description,flags,
			[min,max](T val,std::string & msg){ return change_is_valid(val, min,max,msg); })
	,_min(min)
	,_max(max)
	{
		if (min > max )
		{
			std::string msg = "["+Parameter<T>::name()+"] minimum is larger than maximum.";
			throw std::logic_error(msg);
		}
	}


	MMNumber(std::string name, T value ,std::string description, Flags flags = Flags::DEFAULT)
	:Parameter<T>(name,value,description,flags)
	,_min(boost::none)
	,_max(boost::none)
	{

	}


	MMNumber(std::string name ,std::string description, Flags flags = Flags::DEFAULT)
	:Parameter<T>(name,description,flags)
	,_min(boost::none)
	,_max(boost::none)
	{

	}

	/// Type of the parameter.
	/// Provides a string that contains the "type" of the parameter
	/// e.g "float", "bool", "choice", etc...
	/// @return the parameter's type
	std::string type() const override { return "number"; };

 	std::map<std::string,std::string> attributes() const override {
 		auto attributes = Parameter<T>::attributes();
 		if (_min.is_initialized()) {
 	 		attributes["min"] = Json::valueToString(_min.get());
 		}
 		if (_max.is_initialized()) {
 			attributes["max"] = Json::valueToString(_max.get());
 		}
 		return attributes;
	}

private:

	static bool change_is_valid(T val, T min, T max, std::string & msg) {
		if ((val >= min) && (val <= max)) {
			return true;
		}
		msg = "New value " + std::to_string(val) + " is out of range (" + std::to_string(min) + "," + std::to_string(max) + ")";
		return false;
	}
	boost::optional<T> _min{boost::none}, _max{boost::none};
};

typedef MMNumber<double> MMDouble;
typedef MMNumber<int32_t> MMInt;


template <typename Enum>
class MMChoice : public Parameter<Enum>
{
public:
	 using Parameter<Enum>::operator=;


	MMChoice(std::string name, std::string description, std::map<Enum,std::string> choices, Flags flags = Flags::DEFAULT)
	:Parameter<Enum>(name,description, flags
			,[choices](Enum val, std::string & msg) { return change_is_valid(val, choices, msg);}
			,[choices](Enum val) { return to_string(val, choices); }
			,[choices](std::string i, Enum & val) { return from_string(i,val,choices);})
	,_choices(choices)
	{
		if (choices.size() == 0) {
			std::string msg = "["+Parameter<Enum>::name()+"] Missing list of possible choice at initialization";
			throw std::logic_error(msg);
		}
	}

	MMChoice(std::string name, Enum value, std::string description, std::map<Enum,std::string> choices, Flags flags = Flags::DEFAULT)
	:Parameter<Enum>(name,value, description, flags
			,[choices](Enum val, std::string & msg) { return change_is_valid(val, choices, msg);}
			,[choices](Enum val) { return to_string(val, choices); }
			,[choices](std::string i, Enum & val) { return from_string(i,val,choices);})
	,_choices(choices)
	{
		if (choices.size() == 0) {
			std::string msg = "["+Parameter<Enum>::name()+"] Missing list of possible choice at initialization";
			throw std::logic_error(msg);
		}
	}

	std::string type() const override { return "choice"; }

private:


 	std::map<std::string,std::string> attributes() const override
	{
 		auto attributes = Parameter<Enum>::attributes();
 		attributes["choices"] = choices_to_string(_choices);
 		return attributes;
	}

	static bool change_is_valid(Enum val, const std::map<Enum,std::string> & choices, std::string & msg) {

		for (auto & v : choices)
		{
			if (val == v.first) {
				return true;
			}
		}

		// we didn't find the choice. provide error message with possible choices.
		msg = "Choice number " + std::to_string(int(val)) + " not found in list of choices. Expected one of "+ choices_to_string(choices);

		return false;
	}

    /// Convert to string
    static std::string to_string(Enum val, const std::map<Enum,std::string> & choices)  {
    	for (auto & v : choices) {
    		if (val == v.first) {
    			return Json::valueToQuotedString(v.second.c_str());
    		}
    	}
    	return "Not found in list of choices " + choices_to_string(choices);
    }


    /// Convert from string
    static bool from_string(std::string in, Enum & val, const std::map<Enum,std::string> & choices)  {
    	for (auto & v : choices) {
    		if (in == v.second) {
    			val = v.first;
    			return true;
    		}
    	}
    	return false;
    }


	static std::string choices_to_string(const std::map<Enum,std::string> & choices)
	{
		Json::Value json;

		for (auto & choice : choices)
		{
			json.append(choice.second);
		}
		return json.toStyledString();
	}

	const std::map<Enum,std::string> _choices;
};



class ParameterSet : public ParameterBase
{
public:
	ParameterSet(momentum::ParameterSet&&);
	ParameterSet(std::string name, std::string description = "")
	:ParameterBase(name,description == "" ? name + " parameters" : description)
	,_logger(std::unique_ptr<MMLogger>(new MMLogger(name)))
	,_parameters()
	{
		on_error = [this](const std::string & msg) { _logger->error(msg); };
		on_info = [this](const std::string & msg) { _logger->verbose(0,msg); };
	}

	std::string dump() const override
	{
		std::string dump_string;
		for (auto & p : _parameters) {
			dump_string += p.second.get().name() + " |";
		}
		return "[" + ParameterBase::name()+ "] " + dump_string;
	}

	std::string json_string(std::vector<std::string> output_attributes = {}) const override
	{
		Json::Value json_out;

		for (auto & p : _parameters)
		{
			Json::Value json;
			Json::Reader reader;
			reader.parse(p.second.get().json_string(),json);
			json_out[this->name()][p.second.get().name()] = json[p.second.get().name()];
		}
		return json_out.toStyledString();
	}

	bool from_json_string(std::string in) override { return from_string(in,true); }

	std::string type() const override { return "set"; }

	void insert(ParameterBase & parameter) throw(std::logic_error)
	{
		if(!_parameters.insert(std::pair<std::string,std::reference_wrapper<ParameterBase> >(parameter.name(),parameter)).second)
		{
		  std::string message = "[Register][FAILED] " + parameter.name() + " is already registered";
		  ERROR(message);
		  throw std::logic_error(message);
		}
		parameter.on_error = on_error;
		parameter.on_info = on_info;
		INFO("[Register] " + parameter.dump());
	}

	bool contains (std::string name) const {
		return _parameters.find(name) != _parameters.end();
	}

	/// Structured, detailed information of the parameter set.
	/// Provides schema of the parameter.
	/// @return detailed representation of parameter in json form
	std::string info() const { return json_string(); }


	/// Convert to json value
	/// @return string of json representing the parameter set
	std::string to_string() const { return json_string(); }

	/// Convert from string of json value.
	/// Expects every parameter to appear in the string.
	/// Expects only registered parameters to appear in the string.
	/// @param [in] json_in the json string to read from
	/// @return true if successfully read from json object
	bool from_string( std::string in, bool check_complete = true ) {
		Json::Value json;
		Json::Reader reader;
		if (!reader.parse(in,json)) {
			ERROR("[from_string][FAILED] Unable to parse json: " + in);
			return false;
		}

		Json::Value values = static_cast<const Json::Value>(json)[name()];

		if (values.isNull())
		{
			ERROR("[from_string][FAILED] No section for " + name() + " found in " + json.toStyledString());
			return false;
		}

		// keep a vector of known parameter names to track what still need to be read.
		std::vector<std::string> needed;
		if (check_complete) {
			for (auto p : _parameters) {
				needed.push_back(p.first);
			}
		}
		// look through the name-value pairs, convert each value to a parameter.
		// return false if conversion fails.
		auto names = values.getMemberNames();
		for (auto n : names)
		{
			auto found = _parameters.find(n);
			if (found == _parameters.end())
			{
				// parameter is not a known parameter.  log list of expected parameters.
				std::string message = "[from_string][FAILED] Unexpected parameter \"" + n + "\" found in " + json.toStyledString()
									+"Expected one of ["+_parameters.begin()->first;
				for (auto expected : _parameters)
				{
					if (expected.first == _parameters.begin()->first)
					{
						continue;
					}
					message.append(", ").append(expected.first);
				}
				message.append("].");
				ERROR(message);
				return false;
			}

			// parameter is known, try to convert. TODO: rewrite this code
			Json::Value json;
			json[n] = values[n];
			bool success = found->second.get().from_json_string(json.toStyledString());
			if (!success) {
				std::string msg  ="[from_string][FAILED] Unable to convert " + n + " from " + values[n].toStyledString();
				ERROR(msg);
				return false;
			}

			// remove the found name from the list of parameters-to-find
			needed.erase(std::remove(needed.begin(),needed.end(),n),needed.end());
		}


		// log missing parameters and fail if any
		if (needed.size() != 0)
		{
			std::string message = "[from_string][FAILED] Missing parameters [" + *needed.begin();
			for (auto name : needed)
			{
				if (name == *needed.begin())
				{
					continue;
				}
				message.append(", ").append(name);
			}
			message.append("].");
			ERROR(message);
			return false;
		}

		INFO("[from_string][FINISH] configured parameters");
		return true;
	}

	/// Read parameter set from file.
	/// @throws std::runtime_error if unable to open file or parse json
	/// @note Should only be called from constructor.
	/// @note will throw if file has invalid or incomplete information
	/// @param [in] json_file path to json file
	void read_file(std::string json_file) throw(std::runtime_error)
	{
		if (_parameters.size() == 0)
		{
			std::string message = "[ParameterSet::read_file] No parameters registered!";
			ERROR(message);
			throw std::runtime_error(message);
			//return;
		}

		INFO("["+name()+"][ParameterSet::read_file] Reading from " + json_file);

		std::ifstream parameters_file(json_file);
		if (!parameters_file.is_open())
		{
			std::string message = "["+name()+"][Parameters::read_file] Unable to open parameters file: " + json_file;
			ERROR(message);
			throw std::runtime_error(message);
		}

		// read entire file into a string
		std::stringstream buffer;
		buffer << parameters_file.rdbuf();
		std::string parameters_json = buffer.str();

		bool success = from_string(parameters_json);
		if (!success)
		{
			std::string message = "["+name()+"][Parameters::read_file] Failed to set configuration from json: " + parameters_json;
			ERROR(message);
			throw std::runtime_error(message);
		}
	}

	/// Write parameter set to file.
	/// @param [in] out_file path to json file
	/// @return true of write was successful.
	bool write_file(std::string out_file) const
	{
		std::string json_out = to_string();

		INFO("[write_file] Writing to " + out_file);
		std::ofstream out(out_file);
		if (out.is_open()) {
			out << json_out;
			return true;
		}
		ERROR("[write_file][FAIL] Unable to write to " + out_file);
		return false;
	}

	void Register(ParameterBase & parameter) throw(std::logic_error) {
		insert(parameter);
	}


	void Register(std::vector<std::reference_wrapper<ParameterBase>> parameters) throw(std::logic_error) {
		for (auto p : parameters)
		{
			insert(p);
		}
	}

	/// Set a parameter using it's string representation.
	/// If value is "true" or "false" the named parameter is
	/// 	is expected to be a MMBool.
	/// If value is "8.9" or "89" the named parameter is
	/// 	expected to be a MMDouble or MMInt
	/// If value is "\"a choice\"" or any quoted string, the
	/// 	named parameter is expected to be a MMChoice
	/// @note if value is invalid, an error message is logged and
	/// 	the change is ignored.
	/// @note should not be used directly in C++ application.
	/// @param [in] name The name of the parameter to be set
	/// @param [in[ value The string representation of the new value.
	/// @return string containing information of success or failure.
	std::string set_parameter(std::string name, std::string value) {
		from_string(value,false);
		return json_string();
	}

private:

	std::unique_ptr<MMLogger> _logger;
	std::map<std::string,std::reference_wrapper<ParameterBase>> _parameters;

};

typedef ParameterSet Parameters;

inline std::ostream& operator<<(std::ostream& out, const ParameterSet& r){
    return out << r.dump();
}

} // namespace momentum
