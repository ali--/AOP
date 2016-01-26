#include <libs/config/Parameters.hpp>
#define CATCH_CONFIG_MAIN
#include <libs/catch/catch.hpp>
#include <libs/easylogging++/easylogging++.h>
INITIALIZE_EASYLOGGINGPP

using namespace momentum;

enum class Enum1 { A, B, C, D };

SCENARIO( "Numeric parameter can be created and reassigned", "[Numeric]" ) {

    GIVEN( "An uninitialized numeric parameter" ) {

    	MMNumber<float> float_1("uninitialized_float_1","uninitialized float 1");

        WHEN( "an uninitialized numeric value is accessed" ) {
            THEN( "a exception is thrown" ) { REQUIRE_THROWS_AS(float_1 == 1.0,std::logic_error); }
        }

        WHEN( "numeric is changed to valid value" ) { float_1 = 1;
            THEN( "the value is changed" ) { REQUIRE( float_1 == 1.0 ); }
        }

        WHEN( "a on_change callback is given and numeric is changed to valid value" ) {
        		bool callback_called = false;
        		float_1.on_change = [&](float val){
							callback_called = true;
							std::cout << "VALUE CHANGED TO : " << val << std::endl;
						};

        		float_1 = 1;

        		THEN( "the value is changed and callback is called" ) {
        			REQUIRE( callback_called );
                	REQUIRE( float_1 == 1.0 );

                }
        }
    }

    GIVEN( "An initialized numeric parameter" ) {

    	MMNumber<float> float_1("initialized_float_1",2,"initialized float 1");

    	REQUIRE(float_1 == 2);

        WHEN( "an initialized numeric value is accessed" ) {
            THEN( "no exception is thrown" ) { REQUIRE_NOTHROW(float_1 == 1); }
        }

        WHEN( "numeric is changed to valid value" ) { float_1 = 1;
            THEN( "the value is changed" ) { REQUIRE( float_1 == 1.0 ); }
        }
        WHEN( "numeric is read from string" ) {
        	float_1.value_from_string("17");
            THEN( "the value is changed" ) { REQUIRE( float_1 == 17.0 ); }
        }
    }

    GIVEN( "A bounded initialized numeric parameter" ) {

    	MMNumber<float> float_1("float_1",2.0,"a float 1",1,5);


        WHEN( "an initialized numeric value is accessed" ) {
            THEN( "no exception is thrown" ) { REQUIRE_NOTHROW(float_1 == 1.0); }
        }

        WHEN( "numeric is changed to valid value" ) { float_1 = 1;
            THEN( "the value is changed" ) { REQUIRE( float_1 == 1.0 ); }
        }

        WHEN( "numeric is changed to invalid value" ) { float_1 = 6;
            THEN( "the value is not changed" ) { REQUIRE( float_1 == 2 ); }
        }
    }

    GIVEN( "A bounded wrongly initialized numeric parameter" ) {
        WHEN( "The parameter is created" ) {
            THEN( "an exception is thrown" ) {
            	REQUIRE_THROWS_AS( MMNumber<float> float_bad("float_bad",0,"a bad float", 1, 5), std::logic_error);
            }
        }
    }
}


SCENARIO( "Boolean parameter can be created and reassigned", "[Boolean]" ) {

    GIVEN( "An uninitialized Boolean parameter" ) {

    	MMBool bool_1("uninitialized_bool_1","uninitialized bool 1");

        WHEN( "an uninitialized boolean value is accessed" ) {
            THEN( "a exception is thrown" ) { REQUIRE_THROWS_AS(bool_1 == true,std::logic_error); }
        }

        WHEN( "boolean is changed to valid value" ) { bool_1 = true;
            THEN( "the value is changed" ) { REQUIRE( bool_1 == true ); }
        }

        WHEN( "boolean is read from string" ) {
        	bool_1.value_from_string("true");
            THEN( "the value is changed" ) { REQUIRE( bool_1 == true ); }
        }

    }
}


SCENARIO( "Choice parameter can be created and reassigned", "[Choice]" ) {

    GIVEN( "An uninitialized choice parameter" ) {

    	MMChoice<Enum1> choice_1("uninitialized_choice_1","uninitialized choice 1",{
    			{ Enum1::A, "Choice A" },
    			{ Enum1::B, "Choice B" },
    			{ Enum1::C, "Choice C" }});

        WHEN( "an uninitialized choice value is accessed" ) {
            THEN( "a exception is thrown" ) { REQUIRE_THROWS_AS(choice_1 == Enum1::C,std::logic_error); }
        }

        WHEN( "choice is changed to valid value" ) { choice_1 = Enum1::C;
            THEN( "the value is changed" ) { REQUIRE( choice_1 == Enum1::C ); }
        }
    }
    GIVEN( "An initialized choice parameter" ) {

    	MMChoice<Enum1> choice_1("initialized_choice_1",Enum1::C,"initialized choice 1",{
    			{ Enum1::A, "Choice A" },
    			{ Enum1::B, "Choice B" },
    			{ Enum1::C, "Choice C" }});

        WHEN( "an initialized choice value is accessed" ) {
            THEN( "it is the initialized value" ) { REQUIRE(choice_1 == Enum1::C); }
        }

        WHEN( "choice is changed to valid value" ) { choice_1 = Enum1::A;
            THEN( "the value is changed" ) { REQUIRE( choice_1 == Enum1::A ); }
        }


        WHEN( "choice is changed to invalid value" ) { choice_1 = Enum1::D;
            THEN( "the value not changed" ) { REQUIRE( choice_1 == Enum1::C ); }
        }

        WHEN( "choice is read from string" ) {
        	choice_1.value_from_string("Choice A");
            THEN( "the value is changed" ) { REQUIRE( choice_1 == Enum1::A ); }
        }
    }

    GIVEN( "A wrongly initialized choice parameter" ) {
        WHEN( "The parameter is created" ) {
            THEN( "an exception is thrown" ) {
            	REQUIRE_THROWS_AS( MMChoice<Enum1> choice_bad("choice_bad",Enum1::B,"a bad choice",{
            			{Enum1::A,"Choice A"}
            	}), std::logic_error);
            }
        }
    }
}



SCENARIO( "ParameterSet can be created and reassigned", "[ParameterSet]" ) {


    GIVEN( "A parameter set" ) {


    	ParameterSet params("test_parameters","parameters for testing");

        WHEN( "a parameter is added" ) {
        	MMBool bool_1{"bool_1","a boolean 1"};
        	params.insert(bool_1);

            THEN( "the parameter set contains the parameter" ) {
            	REQUIRE(params.contains("bool_1"));
            }
        }
        WHEN( "a parameter set  contains another parameter set" ) {

        	MMBool bool_1{"bool_1",true,"a boolean 1"};
        	params.insert(bool_1);

        	ParameterSet sub_params("test_sub_parameters", "sub parameters");
        	MMNumber<float> float_1{"float_1",17,"a float 1"};
        	sub_params.insert(float_1);

        	params.insert(sub_params);
        	REQUIRE(params.contains("test_sub_parameters"));


		std::string result = R"({  "test_parameters" : {  "bool_1" : { 
                                                          "default" : true, "description" : "a boolean 1",
							 "editable" : false, "type" : "bool",
                                                         "value" : true },
						  "test_sub_parameters" : { "float_1" : {	
                                                                "default" : 17.0, "description" : "a float 1",	
                                                                "editable" : false, "type" : "number",
								"value" : 14.0  }  }  }})";

        	AND_WHEN("the parameter set is read from a string") {

				std::string new_parameter_values = R"({ "test_parameters" : {
									  "bool_1" : { "value" : true  },
									  "test_sub_parameters" : { "float_1" : {"value" : 14.0 }
								      } }})";

				THEN("the parameter set is changed from string") {
					REQUIRE(params.from_json_string(new_parameter_values));
					Json::Value left, right;
					Json::Reader reader;
					REQUIRE(reader.parse(params.json_string(),left));
					REQUIRE(reader.parse(result,right));
					REQUIRE(left == right);
				}


			AND_WHEN("a parameter set is changed from a json") {

				std::string new_value = R"({"test_parameters":{  "test_sub_parameters" : { "float_1" : {"value" : 14.0}}}})";
				Json::Value left, right;
				Json::Reader reader;
				std::string new_json_string = params.set_parameter("test_parameters",new_value);
				THEN("its values are changed") {
					REQUIRE(reader.parse(new_json_string,left));
					REQUIRE(reader.parse(result ,right));
					REQUIRE(left == right);
				}
			}
        	}
        }
    }
}
