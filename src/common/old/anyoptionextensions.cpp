#include "anyoptionextensions.h"
#include "anyoption.h"
#include <iostream>
#include <sstream>
#include <string>
#include <stdexcept>
#include <cstring>
#include <algorithm>
#include <cmath>


/**********************************************************************
NCPA::ParameterSet class methods here
**********************************************************************/


// Constructor.  Does nothing.
NCPA::ParameterSet::ParameterSet() {
	this->_parser = new AnyOption();
	
	this->_typemap.clear();
	this->_boolmap.clear();
	this->_intmap.clear();
	this->_floatmap.clear();
	this->_strmap.clear();
	this->_criteria.clear();
	this->_failed.clear();
	this->_useFile = false;
	this->_useArgs = false;
	this->_argsOverrideFile = true;
}

// Destructor.  Frees memory held by tests and clears out the criteria vector
NCPA::ParameterSet::~ParameterSet() {
	if (! this->_criteria.empty()) {
		std::vector< NCPA::OptionTest * >::iterator it;
		for ( it=this->_criteria.begin(); it != this->_criteria.end(); ++it ) {
			delete (*it);
		}
	}
	this->_typemap.clear();
	this->_criteria.clear();
	this->_failed.clear();
	this->_boolmap.clear();
	this->_intmap.clear();
	this->_floatmap.clear();
	this->_strmap.clear();
	delete this->_parser;
}


void NCPA::ParameterSet::addFlag( std::string key ) {
	this->_typemap[ key ] = OPTION_FLAG;
	this->_boolmap[ key ] = false;
	this->_parser->setFlag( key.c_str() );
}

void NCPA::ParameterSet::addIntegerOption( std::string key ) {
	this->addIntegerOption( key, 0 );
}

void NCPA::ParameterSet::addIntegerOption( std::string key, int default_value ) {
	this->_typemap[ key ] = OPTION_INTEGER;
	this->_intmap[ key ] = default_value;
	this->_parser->setOption( key.c_str() );
}

void NCPA::ParameterSet::addFloatOption( std::string key ) {
	this->addFloatOption( key, 0.0 );
}

void NCPA::ParameterSet::addFloatOption( std::string key, double default_value ) {
	this->_typemap[ key ] = OPTION_FLOAT;
	this->_floatmap[ key ] = default_value;
	this->_parser->setOption( key.c_str() );
}

void NCPA::ParameterSet::addStringOption( std::string key ) {
	this->addStringOption( key, "" );
}

void NCPA::ParameterSet::addStringOption( std::string key, std::string default_value ) {
	this->_typemap[ key ] = OPTION_STRING;
	this->_strmap[ key ] = default_value;
	this->_parser->setOption( key.c_str() );
}

bool NCPA::ParameterSet::getFlagValue( std::string key ) const {
	try {
		return this->_boolmap.at( key );
	} catch (const out_of_range &oob) {
		return false;
	}
}

int NCPA::ParameterSet::getIntegerValue( std::string key ) const {
	
	// will throw out_of_range if the key isn't present
	switch (this->_typemap.at( key )) {
		case OPTION_INTEGER:
			return this->_intmap.at( key );
		case OPTION_FLOAT:
			return (int)std::round( this->_floatmap.at( key ) );
		case OPTION_STRING:
			return std::stoi( this->_strmap.at( key ) );
		case OPTION_FLAG:
			return (int)this->_boolmap.at( key );
		default:
			throw std::out_of_range( "Unknown option " + key );
	}
}

double NCPA::ParameterSet::getFloatValue( std::string key ) const {
	// will throw out_of_range if the key isn't present
	switch (this->_typemap.at( key )) {
		case OPTION_INTEGER:
			return (double)this->_intmap.at( key );
		case OPTION_FLOAT:
			return this->_floatmap.at( key );
		case OPTION_STRING:
			return std::stof( this->_strmap.at( key ) );
		case OPTION_FLAG:
			return (double)this->_boolmap.at( key );
		default:
			throw std::out_of_range( "Unknown option " + key );
	}
}

std::string NCPA::ParameterSet::getStringValue( std::string key ) const {
	// will throw out_of_range if the key isn't present
	switch (this->_typemap.at( key )) {
		case OPTION_INTEGER:
			return std::to_string( this->_intmap.at( key ) );
		case OPTION_FLOAT:
			return std::to_string( this->_floatmap.at( key ) );
		case OPTION_STRING:
			return this->_strmap.at( key );
		case OPTION_FLAG:
			return this->_boolmap.at( key ) ? "true" : "false";
		default:
			throw std::out_of_range( "Unknown option " + key );
	}
}

void NCPA::ParameterSet::addUsageLine( std::string line ) {
	this->_parser->addUsage( line.c_str() );
}

void NCPA::ParameterSet::printUsage() {
	this->_parser->printUsage();
}

void NCPA::ParameterSet::setCommandLineArguments( int _argc, char **_argv ) {
	this->_nArgs = _argc;
	this->_args = _argv;
	this->_useArgs = true;
}

void NCPA::ParameterSet::setOptionFileName( std::string filename ) {
	this->_optionFileName = filename;
	this->_useFile = true;
}

void NCPA::ParameterSet::commandLineOverRidesFile( bool tf ) {
	this->_argsOverrideFile = tf;
}

// will throw invalid_argument if it can't do conversions
void NCPA::ParameterSet::getOptions() {
	
	// parse the options
	if (this->_argsOverrideFile) {
		if (this->_useFile) {
			this->_parser->processFile( this->_optionFileName.c_str() );
		}
		if (this->_useArgs) {
			std::cout << "Using " << this->_nArgs << " arguments." << std::endl;
			this->_parser->processCommandArgs( this->_nArgs, this->_args );
		}
	} else {
		if (this->_useArgs) {
			this->_parser->processCommandArgs( this->_nArgs, this->_args );
		}
		if (this->_useFile) {
			this->_parser->processFile( this->_optionFileName.c_str() );
		}
	}
	
	// Now feed them into the appropriate maps
	std::map< std::string, NCPA::OPTION_TYPE >::const_iterator it;
	std::string key;
	for (it = this->_typemap.begin(); it != this->_typemap.end(); ++it ) {
		key = it->first;
		char *cstr = new char[ key.length() + 1 ];
		std::strcpy(cstr, key.c_str());
		std::cout << "Putting " << cstr << " into ";
		switch (it->second) {
			case OPTION_FLAG:
				if (this->_parser->getFlag( cstr ) ) {
					this->_boolmap[ key ] = true;
					std::cout << "boolmap" << std::endl;
				}
				break;
			case OPTION_INTEGER:
				//std::cout << "intmap = " << this->_parser->getValue( cstr ) << std::endl;
				
				if (this->_parser->getValue( cstr ) != NULL) {
					this->_intmap[ key ] = std::stoi( 
						this->_parser->getValue( key.c_str() ) );
				}
				break;
			case OPTION_FLOAT:
				std::cout << "floatmap";
				if (this->_parser->getValue( cstr ) != NULL) {
					this->_floatmap[ key ] = std::stof( 
						this->_parser->getValue( key.c_str() ) );
					std::cout << "intmap = " << this->_floatmap[ key ] << std::endl;
				}
				break;
			case OPTION_STRING:
				std::cout << "strmap";
				if (this->_parser->getValue( cstr ) != NULL) {
					this->_strmap[ key ] = this->_parser->getValue( key.c_str() );
					std::cout << "intmap = " << this->_strmap[ key ] << std::endl;
				}
				break;
			default:
				throw std::out_of_range( "Unknown option " + key );
		}
		delete [] cstr;
	}
}


// Validates options, sets up the vector of failed options, and returns false if any options failed,
// true otherwise.  Empty criteria sets return true by default.
bool NCPA::ParameterSet::validateOptions() {
	
	// Return true if there are no criteria to satisfy
	if ( _criteria.empty() ) {
		return true;
	}
	
	AnyOption *opts = this->_parser;
	
	// Get ready
	_failed.clear();
	std::vector< NCPA::OptionTest * >::iterator it;
	for ( it=_criteria.begin(); it != _criteria.end(); ++it ) {
		std::cout << "Checking option: " << (*it)->description() << std::endl;
		try {
			if ( ! (*it)->validate( opts ) ) {
				_failed.push_back( (*it) );
				std::cout << "Failed!" << std::endl;
			} else {
				std::cout << "Passed!" << std::endl;
			}
		} catch (const std::invalid_argument& ia) {
			std::cerr << "Number formatting error: " << (*it)->optionName() << " = " 
				<< opts->getValue( (*it)->optionName().c_str() ) << " threw " 
				<< ia.what() << std::endl;
			_failed.push_back( (*it) );
	        } catch (const std::logic_error* le) {
	        	std::cerr << "Incomplete test setup: " << (*it)->description()
				<< " - parameters not fully defined." << std::endl;
			_failed.push_back( (*it) );
	        }
	}
	
	return _failed.empty();
}

// Returns any failure messages for processing by the user
std::vector< NCPA::OptionTest * > NCPA::ParameterSet::getFailedTests() const {
	return _failed;
}

// Prints any failure messages to the provided output stream
void NCPA::ParameterSet::printFailedTests( std::ostream *out ) const {
	if (_failed.empty()) {
		return;
	}
	
	std::vector< NCPA::OptionTest * >::const_iterator it;
	for ( it = _failed.begin(); it != _failed.end(); ++it ) {
		(*out) << (*it)->failureMessage() << std::endl;
	}
}

// Prints criteria descriptions to the provided output stream
void NCPA::ParameterSet::printTestDescriptions( std::ostream *out ) const {
	std::vector< NCPA::OptionTest * >::const_iterator it;
	for ( it=_criteria.begin(); it != _criteria.end(); ++it ) {
		(*out) << (*it)->description() << std::endl;
	}
}


// addOption(): adds a test to the queue and returns a pointer to the test
NCPA::OptionTest * NCPA::ParameterSet::addTest( const std::string option,
		OPTION_TEST_TYPE option_type ) {
			
	NCPA::OptionTest *crit;
	switch (option_type) {
		case OPTION_REQUIRED:
			crit = new NCPA::RequiredTest( option );
			break;
		case OPTION_REQUIRED_IF:
			crit = new NCPA::RequiredIfOtherIsPresentTest( option );
			break;
		case OPTION_RADIO_BUTTON:
			crit = new NCPA::RadioButtonTest( option );
			break;
		case OPTION_STRING_SET:
			crit = new NCPA::StringSetTest( option );
			break;
		case OPTION_INTEGER_POSITIVE:
			crit = new NCPA::IntegerGreaterThanTest( option );
			crit->addIntegerParameter( 0 );
			break;
		case OPTION_INTEGER_NEGATIVE:
			crit = new NCPA::IntegerLessThanTest( option );
			crit->addIntegerParameter( 0 );
			break;
		case OPTION_INTEGER_ZERO:
			crit = new NCPA::IntegerEqualToTest( option );
			crit->addIntegerParameter( 0 );
			_criteria.push_back( crit );
			break;
		case OPTION_INTEGER_NONZERO:
			crit = new NCPA::IntegerNotEqualToTest( option );
			crit->addIntegerParameter( 0 );
			break;
		case OPTION_FLOAT_POSITIVE:
			crit = new NCPA::FloatGreaterThanTest( option );
			crit->addFloatParameter( 0.0 );
			break;
		case OPTION_FLOAT_NEGATIVE:
			crit = new NCPA::FloatLessThanTest( option );
			crit->addFloatParameter( 0.0 );
			break;
		case OPTION_FLOAT_ZERO:
			crit = new NCPA::FloatEqualToTest( option );
			crit->addFloatParameter( 0.0 );
			break;
		case OPTION_FLOAT_NONZERO:
			crit = new NCPA::FloatNotEqualToTest( option );
			crit->addFloatParameter( 0.0 );
			break;
		case OPTION_INTEGER_GREATER_THAN:
			crit = new NCPA::IntegerGreaterThanTest( option );
			break;
		case OPTION_INTEGER_GREATER_THAN_OR_EQUAL:
			crit = new NCPA::IntegerGreaterThanOrEqualToTest( option );
			break;
		case OPTION_INTEGER_LESS_THAN:
			crit = new NCPA::IntegerLessThanTest( option );
			break;
		case OPTION_INTEGER_LESS_THAN_OR_EQUAL:
			crit = new NCPA::IntegerLessThanOrEqualToTest( option );
			break;
		case OPTION_INTEGER_EQUAL:
			crit = new NCPA::IntegerEqualToTest( option );
			break;
		case OPTION_INTEGER_NOT_EQUAL:
			crit = new NCPA::IntegerNotEqualToTest( option );
			break;
		case OPTION_FLOAT_GREATER_THAN:
			crit = new NCPA::FloatGreaterThanTest( option );
			break;
		case OPTION_FLOAT_GREATER_THAN_OR_EQUAL:
			crit = new NCPA::FloatGreaterThanOrEqualToTest( option );
			break;
		case OPTION_FLOAT_LESS_THAN:
			crit = new NCPA::FloatLessThanTest( option );
			break;
		case OPTION_FLOAT_LESS_THAN_OR_EQUAL:
			crit = new NCPA::FloatLessThanOrEqualToTest( option );
			break;
		case OPTION_FLOAT_EQUAL:
			crit = new NCPA::FloatEqualToTest( option );
			break;
		case OPTION_FLOAT_NOT_EQUAL:
			crit = new NCPA::FloatNotEqualToTest( option );
			break;
		case OPTION_STRING_MINIMUM_LENGTH:
			crit = new NCPA::StringMinimumLengthTest( option );
			break;
		case OPTION_STRING_MAXIMUM_LENGTH:
			crit = new NCPA::StringMaximumLengthTest( option );
			break;
		default:
			throw std::invalid_argument( "Undefined test requested" );
	}
	_criteria.push_back( crit );
	return crit;
}



/**********************************************************************
NCPA::OptionTest class methods here
**********************************************************************/

// Destructor for ABC, must not be pure virtual
NCPA::OptionTest::~OptionTest() { }

std::string NCPA::OptionTest::optionName() const {
	return _optName;
}

void NCPA::OptionTest::addIntegerParameter( int param ) { }
void NCPA::OptionTest::addFloatParameter( double param ) { }
void NCPA::OptionTest::addStringParameter( std::string param ) { }
bool NCPA::OptionTest::ready() const { return _ready; }



/**********************************************************************
NCPA::OptionTest derived class methods here
Each gets a constructor, a validate() method, a description, a failure message,
and optional add<Type>Parameter() method(s)
**********************************************************************/

NCPA::RequiredTest::RequiredTest( const std::string optionName ) {
	_optName = optionName;
	_ready = true;
}
std::string NCPA::RequiredTest::description() const {
	return _optName + " is present.";
}
std::string NCPA::RequiredTest::failureMessage() const {
	return _optName + " is not present.";
}
bool NCPA::RequiredTest::validate( AnyOption *opt )  {
	// Just check to see if it's been provided
	if ( opt->getFlag( _optName.c_str() ) || ( opt->getValue( _optName.c_str()) != NULL ) ) {
		return true;
	} else {
		return false;
	}
}
std::string NCPA::RequiredTest::valueString() const { return ""; }


NCPA::RequiredIfOtherIsPresentTest::RequiredIfOtherIsPresentTest( const std::string optionName ) {
	_optName = optionName;
	_ready = false;
}
std::string NCPA::RequiredIfOtherIsPresentTest::description() const {
	return _optName + " is present if one of " + this->valueString() 
		+ " is also present.";
}
std::string NCPA::RequiredIfOtherIsPresentTest::failureMessage() const {
	return "One of " + this->valueString() + " is set, but " + _optName
		+ " is not set.";
}
std::string NCPA::RequiredIfOtherIsPresentTest::valueString() const {
	ostringstream oss;
	oss << "{ ";
	for (std::vector<std::string>::const_iterator it = _prereqs.begin();
			it != _prereqs.end(); ++it) {
		if (it != _prereqs.begin()) {
			oss << ", ";
		}
		oss << *it;
	}
	oss << " }";
	return oss.str();
}
bool NCPA::RequiredIfOtherIsPresentTest::validate( AnyOption *opt )  {
	if (! this->ready() ) {
		throw new std::logic_error( _optName + ": no options defined." );
	}
	bool prereqs_met = false;
	
	// see if at least one prereq is present
	for (std::vector<std::string>::const_iterator it = _prereqs.begin();
			it != _prereqs.end(); ++it) {
		if (opt->getValue( it->c_str()) != NULL || opt->getFlag( it->c_str() ) ) {
			prereqs_met = true;
		}
	}
	
	// if prereq(s) there, check for option presence
	if (prereqs_met) {
		if (opt->getValue( _optName.c_str()) != NULL || opt->getFlag( _optName.c_str() ) ) {
			return true;
		} else {
			return false;
		}
	} else {
		return true;
	}
}
void NCPA::RequiredIfOtherIsPresentTest::addStringParameter( const std::string param ) {
	std::string str = param;
	_prereqs.push_back( str );
}
bool NCPA::RequiredIfOtherIsPresentTest::ready() const {
	return !( _prereqs.empty() );
}



NCPA::RadioButtonTest::RadioButtonTest( const std::string optionName ) {
	_buttons.clear();
	_optName = optionName;
	_matched.clear();
}
std::string NCPA::RadioButtonTest::description() const {
	return _optName + ": One and only one of " + this->valueString() + " must be present.";
}
std::string NCPA::RadioButtonTest::failureMessage() const {
	ostringstream oss;
	oss << _optName << ": " << _matched.size() << " of " << this->valueString()
		<< " are present; must be one and only one.";
	return oss.str();
}
std::string NCPA::RadioButtonTest::valueString() const {
	ostringstream oss;
	oss << "{ ";
	for (std::vector<std::string>::const_iterator it = _buttons.begin();
			it != _buttons.end(); ++it) {
		if (it != _buttons.begin()) {
			oss << ", ";
		}
		oss << *it;
	}
	oss << " }";
	return oss.str();
}
bool NCPA::RadioButtonTest::validate( AnyOption *opt )  {
	_matched.clear();
	if (! this->ready() ) {
		throw new std::logic_error( _optName + ": no options defined." );
	}
	
	for (std::vector<std::string>::const_iterator it = _buttons.begin();
			it != _buttons.end(); ++it) {
		if (opt->getValue( it->c_str()) != NULL || opt->getFlag( it->c_str() ) ) {
			_matched.push_back( *it );
		}
	}
	
	return (_matched.size() == 1);
}
void NCPA::RadioButtonTest::addStringParameter( const std::string newButton ) {
	std::string str = newButton;
	_buttons.push_back( str );
}
std::vector< std::string > NCPA::RadioButtonTest::lastMatched() const {
	std::vector< std::string > v( _matched );
	return v;
}
bool NCPA::RadioButtonTest::ready() const {
	return !( _buttons.empty() );
}





NCPA::IntegerGreaterThanTest::IntegerGreaterThanTest( const std::string optionName ) {
	_optName = optionName;
	_testedValue.clear();
	_value = 0;
	_ready = false;
}
std::string NCPA::IntegerGreaterThanTest::description() const {
	return _optName + " is greater than " + 
		this->valueString() + ".";
}
std::string NCPA::IntegerGreaterThanTest::failureMessage() const {
	return _optName + " (" + _testedValue + ") must be greater than " 
		+ this->valueString() + ".";
}
bool NCPA::IntegerGreaterThanTest::validate( AnyOption *opt )  {
	if (! this->ready() ) {
		throw new std::logic_error( _optName + ": no options defined." );
	}
	
	char *valStr = opt->getValue( _optName.c_str() );
	if (valStr == NULL) {
		_testedValue.clear();
		return true;
	}
	
	_testedValue = valStr;
	int val = std::stoi( valStr );
	return ( val > _value );
}
std::string NCPA::IntegerGreaterThanTest::valueString() const {
	return this->ready() ? std::to_string(_value) : "";
}
void NCPA::IntegerGreaterThanTest::addIntegerParameter( int param ) {
	_value = param;
	_ready = true;
}



NCPA::IntegerGreaterThanOrEqualToTest::IntegerGreaterThanOrEqualToTest( const std::string optionName ) {
	_optName = optionName;
	_testedValue.clear();
	_value = 0;
	_ready = false;
}
std::string NCPA::IntegerGreaterThanOrEqualToTest::description() const {
	return _optName + " is greater than or equal to " + 
		this->valueString() + ".";
}
std::string NCPA::IntegerGreaterThanOrEqualToTest::failureMessage() const {
	return _optName + " (" + _testedValue + ") must be greater than or equal to " 
		+ this->valueString() + ".";
}
bool NCPA::IntegerGreaterThanOrEqualToTest::validate( AnyOption *opt )  {
	if (! this->ready() ) {
		throw new std::logic_error( _optName + ": no options defined." );
	}
	
	char *valStr = opt->getValue( _optName.c_str() );
	if (valStr == NULL) {
		_testedValue.clear();
		return true;
	}
	
	_testedValue = valStr;
	int val = std::stoi( valStr );
	return ( val >= _value );
}
std::string NCPA::IntegerGreaterThanOrEqualToTest::valueString() const {
	return this->ready() ? std::to_string(_value) : "";
}
void NCPA::IntegerGreaterThanOrEqualToTest::addIntegerParameter( int param ) {
	_value = param;
	_ready = true;
}



NCPA::IntegerLessThanTest::IntegerLessThanTest( const std::string optionName ) {
	_optName = optionName;
	_testedValue.clear();
	_value = 0;
	_ready = false;
}
std::string NCPA::IntegerLessThanTest::description() const {
	return _optName + " is less than " + 
		this->valueString() + ".";
}
std::string NCPA::IntegerLessThanTest::failureMessage() const {
	return _optName + " (" + _testedValue + ") must be less than " 
		+ this->valueString() + ".";
}
bool NCPA::IntegerLessThanTest::validate( AnyOption *opt )  {
	if (! this->ready() ) {
		throw new std::logic_error( _optName + ": no options defined." );
	}
	
	char *valStr = opt->getValue( _optName.c_str() );
	if (valStr == NULL) {
		_testedValue.clear();
		return true;
	}
	
	_testedValue = valStr;
	int val = std::stoi( valStr );
	return ( val < _value );
}
std::string NCPA::IntegerLessThanTest::valueString() const {
	return this->ready() ? std::to_string(_value) : "";
}
void NCPA::IntegerLessThanTest::addIntegerParameter( int param ) {
	_value = param;
	_ready = true;
}



NCPA::IntegerLessThanOrEqualToTest::IntegerLessThanOrEqualToTest( const std::string optionName ) {
	_optName = optionName;
	_testedValue.clear();
	_value = 0;
	_ready = false;
}
std::string NCPA::IntegerLessThanOrEqualToTest::description() const {
	return _optName + " is less than or equal to " + 
		this->valueString() + ".";
}
std::string NCPA::IntegerLessThanOrEqualToTest::failureMessage() const {
	return _optName + " (" + _testedValue + ") must be less than or equal to " 
		+ this->valueString() + ".";
}
bool NCPA::IntegerLessThanOrEqualToTest::validate( AnyOption *opt )  {
	if (! this->ready() ) {
		throw new std::logic_error( _optName + ": no options defined." );
	}
	
	char *valStr = opt->getValue( _optName.c_str() );
	if (valStr == NULL) {
		_testedValue.clear();
		return true;
	}
	
	_testedValue = valStr;
	int val = std::stoi( valStr );
	return ( val <= _value );
}
std::string NCPA::IntegerLessThanOrEqualToTest::valueString() const {
	return this->ready() ? std::to_string(_value) : "";
}
void NCPA::IntegerLessThanOrEqualToTest::addIntegerParameter( int param ) {
	_value = param;
	_ready = true;
}





NCPA::IntegerEqualToTest::IntegerEqualToTest( const std::string optionName ) {
	_optName = optionName;
	_testedValue.clear();
	_value = 0;
	_ready = false;
}
std::string NCPA::IntegerEqualToTest::description() const {
	return _optName + " is equal to " + this->valueString() + ".";
}
std::string NCPA::IntegerEqualToTest::failureMessage() const {
	return _optName + " (" + _testedValue + ") must be equal to " 
		+ this->valueString() + ".";
}
bool NCPA::IntegerEqualToTest::validate( AnyOption *opt )  {
	if (! this->ready() ) {
		throw new std::logic_error( _optName + ": no options defined." );
	}
	
	char *valStr = opt->getValue( _optName.c_str() );
	if (valStr == NULL) {
		_testedValue.clear();
		return true;
	}
	
	_testedValue = valStr;
	int val = std::stoi( valStr );
	return ( val == _value );
}
std::string NCPA::IntegerEqualToTest::valueString() const {
	return this->ready() ? std::to_string(_value) : "";
}
void NCPA::IntegerEqualToTest::addIntegerParameter( int param ) {
	_value = param;
	_ready = true;
}



NCPA::IntegerNotEqualToTest::IntegerNotEqualToTest( const std::string optionName ) {
	_optName = optionName;
	_testedValue.clear();
	_value = 0;
	_ready = false;
}
std::string NCPA::IntegerNotEqualToTest::description() const {
	return _optName + " is not equal to " + this->valueString() + ".";
}
std::string NCPA::IntegerNotEqualToTest::failureMessage() const {
	return _optName + " (" + _testedValue + ") must not be equal to " 
		+ this->valueString() + ".";
}
bool NCPA::IntegerNotEqualToTest::validate( AnyOption *opt )  {
	if (! this->ready() ) {
		throw new std::logic_error( _optName + ": no options defined." );
	}
	
	char *valStr = opt->getValue( _optName.c_str() );
	if (valStr == NULL) {
		_testedValue.clear();
		return true;
	}
	
	_testedValue = valStr;
	int val = std::stoi( valStr );
	return ( val != _value );
}
std::string NCPA::IntegerNotEqualToTest::valueString() const {
	return this->ready() ? std::to_string(_value) : "";
}
void NCPA::IntegerNotEqualToTest::addIntegerParameter( int param ) {
	_value = param;
	_ready = true;
}






NCPA::FloatGreaterThanTest::FloatGreaterThanTest( const std::string optionName ) {
	_optName = optionName;
	_testedValue.clear();
	_value = 0.0;
	_ready = false;
}
std::string NCPA::FloatGreaterThanTest::description() const {
	return _optName + " is greater than " 
		 + this->valueString() + ".";
}
std::string NCPA::FloatGreaterThanTest::failureMessage() const {
	return _optName + " (" + _testedValue + ") must be greater than " 
		+ this->valueString() + ".";
}
bool NCPA::FloatGreaterThanTest::validate( AnyOption *opt )  {
	if (! this->ready() ) {
		throw new std::logic_error( _optName + ": no options defined." );
	}
	
	char *valStr = opt->getValue( _optName.c_str() );
	if (valStr == NULL) {
		_testedValue.clear();
		return true;
	}
	
	_testedValue = valStr;
	double val = std::stof( valStr );
	return ( val > _value );
}
std::string NCPA::FloatGreaterThanTest::valueString() const {
	return this->ready() ? std::to_string(_value) : "";
}
void NCPA::FloatGreaterThanTest::addFloatParameter( double param ) {
	_value = param;
	_ready = true;
}



NCPA::FloatGreaterThanOrEqualToTest::FloatGreaterThanOrEqualToTest( const std::string optionName ) {
	_optName = optionName;
	_testedValue.clear();
	_value = 0.0;
	_ready = false;
}
std::string NCPA::FloatGreaterThanOrEqualToTest::description() const {
	return _optName + " is greater than or equal to " 
		 + this->valueString() + ".";
}
std::string NCPA::FloatGreaterThanOrEqualToTest::failureMessage() const {
	return _optName + " (" + _testedValue + ") must be greater than or equal to " 
		+ this->valueString() + ".";
}
bool NCPA::FloatGreaterThanOrEqualToTest::validate( AnyOption *opt )  {
	if (! this->ready() ) {
		throw new std::logic_error( _optName + ": no options defined." );
	}
	
	char *valStr = opt->getValue( _optName.c_str() );
	if (valStr == NULL) {
		_testedValue.clear();
		return true;
	}
	
	_testedValue = valStr;
	double val = std::stof( valStr );
	return ( val >= _value );
}
std::string NCPA::FloatGreaterThanOrEqualToTest::valueString() const {
	return this->ready() ? std::to_string(_value) : "";
}
void NCPA::FloatGreaterThanOrEqualToTest::addFloatParameter( double param ) {
	_value = param;
	_ready = true;
}





NCPA::FloatLessThanTest::FloatLessThanTest( const std::string optionName ) {
	_optName = optionName;
	_testedValue.clear();
	_value = 0.0;
	_ready = false;
}
std::string NCPA::FloatLessThanTest::description() const {
	return _optName + " is less than " 
		 + this->valueString() + ".";
}
std::string NCPA::FloatLessThanTest::failureMessage() const {
	return _optName + " (" + _testedValue + ") must be less than " 
		+ this->valueString() + ".";
}
bool NCPA::FloatLessThanTest::validate( AnyOption *opt )  {
	if (! this->ready() ) {
		throw new std::logic_error( _optName + ": no options defined." );
	}
	
	char *valStr = opt->getValue( _optName.c_str() );
	if (valStr == NULL) {
		_testedValue.clear();
		return true;
	}
	
	_testedValue = valStr;
	double val = std::stof( valStr );
	return ( val < _value );
}
std::string NCPA::FloatLessThanTest::valueString() const {
	return this->ready() ? std::to_string(_value) : "";
}
void NCPA::FloatLessThanTest::addFloatParameter( double param ) {
	_value = param;
	_ready = true;
}



NCPA::FloatLessThanOrEqualToTest::FloatLessThanOrEqualToTest( const std::string optionName ) {
	_optName = optionName;
	_testedValue.clear();
	_value = 0.0;
	_ready = false;
}
std::string NCPA::FloatLessThanOrEqualToTest::description() const {
	return _optName + " is less than " 
		 + this->valueString() + ".";
}
std::string NCPA::FloatLessThanOrEqualToTest::failureMessage() const {
	return _optName + " (" + _testedValue + ") must be less than " 
		+ this->valueString() + ".";
}
bool NCPA::FloatLessThanOrEqualToTest::validate( AnyOption *opt )  {
	if (! this->ready() ) {
		throw new std::logic_error( _optName + ": no options defined." );
	}
	
	char *valStr = opt->getValue( _optName.c_str() );
	if (valStr == NULL) {
		_testedValue.clear();
		return true;
	}
	
	_testedValue = valStr;
	double val = std::stof( valStr );
	return ( val <= _value );
}
std::string NCPA::FloatLessThanOrEqualToTest::valueString() const {
	return this->ready() ? std::to_string(_value) : "";
}
void NCPA::FloatLessThanOrEqualToTest::addFloatParameter( double param ) {
	_value = param;
	_ready = true;
}




NCPA::FloatEqualToTest::FloatEqualToTest( const std::string optionName ) {
	_optName = optionName;
	_testedValue.clear();
	_value = 0.0;
	_ready = false;
}
std::string NCPA::FloatEqualToTest::description() const {
	return _optName + " is equal to " + this->valueString() + ".";
}
std::string NCPA::FloatEqualToTest::failureMessage() const {
	return _optName + " (" + _testedValue + ") must be equal to " 
		+ this->valueString() + ".";
}
bool NCPA::FloatEqualToTest::validate( AnyOption *opt )  {
	if (! this->ready() ) {
		throw new std::logic_error( _optName + ": no options defined." );
	}
	
	char *valStr = opt->getValue( _optName.c_str() );
	if (valStr == NULL) {
		_testedValue.clear();
		return true;
	}
	
	_testedValue = valStr;
	double val = std::stof( valStr );
	return ( val == _value );
}
std::string NCPA::FloatEqualToTest::valueString() const {
	return this->ready() ? std::to_string(_value) : "";
}
void NCPA::FloatEqualToTest::addFloatParameter( double param ) {
	_value = param;
	_ready = true;
}



NCPA::FloatNotEqualToTest::FloatNotEqualToTest( const std::string optionName ) {
	_optName = optionName;
	_testedValue.clear();
	_value = 0.0;
	_ready = false;
}
std::string NCPA::FloatNotEqualToTest::description() const {
	return _optName + " is not equal to " + this->valueString() + ".";
}
std::string NCPA::FloatNotEqualToTest::failureMessage() const {
	return _optName + " (" + _testedValue + ") must not be equal to " 
		+ this->valueString() + ".";
}
bool NCPA::FloatNotEqualToTest::validate( AnyOption *opt )  {
	if (! this->ready() ) {
		throw new std::logic_error( _optName + ": no options defined." );
	}
	
	char *valStr = opt->getValue( _optName.c_str() );
	if (valStr == NULL) {
		_testedValue.clear();
		return true;
	}
	
	_testedValue = valStr;
	double val = std::stof( valStr );
	return ( val != _value );
}
std::string NCPA::FloatNotEqualToTest::valueString() const {
	return this->ready() ? std::to_string(_value) : "";
}
void NCPA::FloatNotEqualToTest::addFloatParameter( double param ) {
	_value = param;
	_ready = true;
}






NCPA::StringMinimumLengthTest::StringMinimumLengthTest( const std::string optionName ) {
	_optName = optionName;
	_testedValue.clear();
	_value = 0;
	_ready = false;
}
std::string NCPA::StringMinimumLengthTest::description() const {
	return _optName + " is at least " + this->valueString() + " characters.";
}
std::string NCPA::StringMinimumLengthTest::failureMessage() const {
	return _optName + " (\"" + _testedValue + "\") must be at least " 
		+ this->valueString() + " characters long.";
}
bool NCPA::StringMinimumLengthTest::validate( AnyOption *opt )  {
	if (! this->ready() ) {
		throw new std::logic_error( _optName + ": no options defined." );
	}
	
	char *valStr = opt->getValue( _optName.c_str() );
	if (valStr == NULL) {
		_testedValue.clear();
		return true;
	}
	
	_testedValue = valStr;

	return ( _testedValue.length() >= _value );
}
std::string NCPA::StringMinimumLengthTest::valueString() const {
	return this->ready() ? std::to_string(_value) : "";
}
void NCPA::StringMinimumLengthTest::addIntegerParameter( int param ) {
	if (param < 0) {
		throw std::range_error( "String length must not be negative" );
	}
	_value = (unsigned int)param;
	_ready = true;
}




NCPA::StringMaximumLengthTest::StringMaximumLengthTest( const std::string optionName ) {
	_optName = optionName;
	_testedValue.clear();
	_value = 0;
	_ready = false;
}
std::string NCPA::StringMaximumLengthTest::description() const {
	return _optName + " is at most " + this->valueString() + " characters.";
}
std::string NCPA::StringMaximumLengthTest::failureMessage() const {
	return _optName + " (\"" + _testedValue + "\") must be at most " 
		+ this->valueString() + " characters long.";
}
bool NCPA::StringMaximumLengthTest::validate( AnyOption *opt )  {
	if (! this->ready() ) {
		throw new std::logic_error( _optName + ": no options defined." );
	}
	
	char *valStr = opt->getValue( _optName.c_str() );
	if (valStr == NULL) {
		_testedValue.clear();
		return true;
	}
	
	_testedValue = valStr;

	return ( _testedValue.length() <= _value );
}
std::string NCPA::StringMaximumLengthTest::valueString() const {
	return this->ready() ? std::to_string(_value) : "";
}
void NCPA::StringMaximumLengthTest::addIntegerParameter( int param ) {
	if (param < 0) {
		throw std::range_error( "String length must not be negative" );
	}
	_value = (unsigned int)param;
	_ready = true;
}




NCPA::StringSetTest::StringSetTest( const std::string optionName ) {
	_choices.clear();
	_optName = optionName;
}
std::string NCPA::StringSetTest::description() const {
	return _optName + " must be in " + this->valueString() + ".";
}
std::string NCPA::StringSetTest::failureMessage() const {
	return _optName + ": " + '"' + _testedValue + '"' + " is not in " + this->valueString() + ".";
}
std::string NCPA::StringSetTest::valueString() const {
	ostringstream oss;
	oss << "{ ";
	for (std::vector<std::string>::const_iterator it = _choices.begin();
			it != _choices.end(); ++it) {
		if (it != _choices.begin()) {
			oss << ", ";
		}
		oss << '"' << *it << '"';
	}
	oss << " }";
	return oss.str();
}
bool NCPA::StringSetTest::validate( AnyOption *opt )  {
	if (! this->ready() ) {
		throw new std::logic_error( _optName + ": no options defined." );
	} else {
		std::cout << "Ready to test value " << _optName.c_str() << std::endl;
	}
	
	if (opt->getValue( _optName.c_str() ) == NULL) {
		return true;
	}
	_testedValue = opt->getValue( _optName.c_str() );
	//std::cout << "Tested value is " << _testedValue << std::endl;
	std::vector< std::string >::const_iterator it = std::find( _choices.begin(), _choices.end(), _testedValue );
	return ( it != _choices.end() );
}
void NCPA::StringSetTest::addStringParameter( std::string newChoice ) {
	_choices.push_back( newChoice );
}
bool NCPA::StringSetTest::ready() const {
	return ! ( _choices.empty() );
}