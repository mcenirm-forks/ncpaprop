#include "units.h"
#include "util.h"
#include <map>
#include <utility>
#include <stdexcept>
#include <cstring>
#include <iostream>
#include <iomanip>

// initialize static members
conversion_map_t NCPA::Units::map_ = conversion_map_t();
// conversion_map_t NCPA::Units::map_d1_ = conversion_map_t();
// conversion_map_t NCPA::Units::map_d2_ = conversion_map_t();
units_to_string_map_t NCPA::Units::enum_to_string_map_ = units_to_string_map_t();
units_to_string_map_t NCPA::Units::enum_to_abbr_map_ = units_to_string_map_t();
string_to_units_map_t NCPA::Units::string_to_enum_map_ = string_to_units_map_t();


void NCPA::Units::list_recognized_strings( std::ostream& os ) {
	if ( ! NCPA::Units::ready_() ) {
		NCPA::Units::initialize_();
	}

	int maxwidth = 0;
	string_to_units_map_t::const_iterator i;
	for ( i = NCPA::Units::string_to_enum_map_.cbegin(); i != NCPA::Units::string_to_enum_map_.cend(); ++i ) {
		maxwidth = NCPA::max( maxwidth, (int)(*i).first.size() );
	}

	os << "Note: strings are not case-sensitive" << std::endl;
	os << std::setw( maxwidth ) << std::right << "String" << " : " << "Units" << std::endl;
	os << std::setw( maxwidth ) << std::right << "------" << " : " << "-----" << std::endl;
	for ( i = NCPA::Units::string_to_enum_map_.cbegin(); i != NCPA::Units::string_to_enum_map_.cend(); ++i ) {
		os << std::setw( maxwidth ) << std::right << (*i).first << " | " << NCPA::Units::toString( (*i).second ) << std::endl;
	}
}

NCPA::units_t NCPA::Units::fromString( std::string s ) {

	if ( ! NCPA::Units::ready_() ) {
		NCPA::Units::initialize_();
	}

	try {
		return NCPA::Units::string_to_enum_map_.at( NCPA::toUpperCase( s ) );
	} catch (std::out_of_range e) {
		throw std::out_of_range( "Unrecognized units string: " + s );
	}
}

std::string NCPA::Units::toString( units_t u ) {

	if ( ! NCPA::Units::ready_() ) {
		NCPA::Units::initialize_();
	}

	try {
		return NCPA::Units::enum_to_string_map_.at( u );
	} catch (std::out_of_range e) {
		throw std::out_of_range( "Unrecognized units type" );
	}

	// switch (u) {
	// 	case UNITS_NONE:
	// 		return "No units";
	// 	case UNITS_TEMPERATURE_KELVIN:
	// 		return "degrees Kelvin";
	// 	case UNITS_TEMPERATURE_CELSIUS:
	// 		return "degrees Celsius";
	// 	case UNITS_TEMPERATURE_FAHRENHEIT:
	// 		return "degrees Fahrenheit";
	// 	case UNITS_DISTANCE_METERS:
	// 		return "meters";
	// 	case UNITS_DISTANCE_KILOMETERS:
	// 		return "kilometers";
	// 	case UNITS_SPEED_METERS_PER_SECOND:
	// 		return "meters per second";
	// 	case UNITS_SPEED_KILOMETERS_PER_SECOND:
	// 		return "kilometers per second";
	// 	case UNITS_PRESSURE_PASCALS:
	// 		return "Pascals";
	// 	case UNITS_PRESSURE_MILLIBARS:
	// 		return "millibars";
	// 	case UNITS_DENSITY_KILOGRAMS_PER_CUBIC_METER:
	// 		return "kilograms per cubic meter";
	// 	case UNITS_DENSITY_GRAMS_PER_CUBIC_CENTIMETER:
	// 		return "grams per cubic centimeter";
	// 	case UNITS_DIRECTION_DEGREES_CLOCKWISE_FROM_NORTH:
	// 		return "degrees clockwise from North";
	// 	case UNITS_DIRECTION_DEGREES_COUNTERCLOCKWISE_FROM_EAST:
	// 		return "degrees counterclockwise from East";
	// 	case UNITS_ANGLE_DEGREES:
	// 		return "degrees";
	// 	case UNITS_ANGLE_RADIANS:
	// 		return "radians";
	// 	default:
	// 		throw std::out_of_range( "Unrecognized units type" );
	// }
}


std::string NCPA::Units::toStr( units_t u ) {

	if ( ! NCPA::Units::ready_() ) {
		NCPA::Units::initialize_();
	}

	try {
		return NCPA::Units::enum_to_abbr_map_.at( u );
	} catch (std::out_of_range e) {
		throw std::out_of_range( "Unrecognized units type" );
	}

	// switch (u) {
	// 	case UNITS_NONE:
	// 		return "N/A";
	// 	case UNITS_TEMPERATURE_KELVIN:
	// 		return "K";
	// 	case UNITS_TEMPERATURE_CELSIUS:
	// 		return "C";
	// 	case UNITS_TEMPERATURE_FAHRENHEIT:
	// 		return "F";
	// 	case UNITS_DISTANCE_METERS:
	// 		return "m";
	// 	case UNITS_DISTANCE_KILOMETERS:
	// 		return "km";
	// 	case UNITS_SPEED_METERS_PER_SECOND:
	// 		return "m/s";
	// 	case UNITS_SPEED_KILOMETERS_PER_SECOND:
	// 		return "km/s";
	// 	case UNITS_PRESSURE_PASCALS:
	// 		return "Pa";
	// 	case UNITS_PRESSURE_MILLIBARS:
	// 		return "mbar";
	// 	case UNITS_DENSITY_KILOGRAMS_PER_CUBIC_METER:
	// 		return "kg/m3";
	// 	case UNITS_DENSITY_GRAMS_PER_CUBIC_CENTIMETER:
	// 		return "g/cm3";
	// 	case UNITS_DIRECTION_DEGREES_CLOCKWISE_FROM_NORTH:
	// 		return "deg CW from N";
	// 	case UNITS_DIRECTION_DEGREES_COUNTERCLOCKWISE_FROM_EAST:
	// 		return "deg CCW from E";
	// 	case UNITS_ANGLE_DEGREES:
	// 		return "deg";
	// 	case UNITS_ANGLE_RADIANS:
	// 		return "rad";
	// 	default:
	// 		throw std::out_of_range( "Unrecognized units type" );
	// }
}


/* 
Constructor for the UnitConverter class.
Internally the class uses a std::map to associate a 
std::pair< units_t, units_t > key with a function pointer that performs the
indicated conversion.  The constructor populates this map with all defined
conversions.  No memory is dynamically allocated within the constructor so no
explicit destructor is required.  Also initializes string <---> enum maps.
*/
void NCPA::Units::initialize_() {
	
	map_.clear();
	// map_d1_.clear();
	// map_d2_.clear();
	enum_to_string_map_.clear();
	enum_to_abbr_map_.clear();
	string_to_enum_map_.clear();

	// conversion functions.  Functions must take in one double, put out one double, and not depend on any external variables.
	map_ = {
		{ get_unit_pair_( UNITS_TEMPERATURE_CELSIUS, UNITS_TEMPERATURE_FAHRENHEIT ), []( double in ) { return ( in * 1.8 ) + 32.0; } },
		{ get_unit_pair_( UNITS_TEMPERATURE_CELSIUS, UNITS_TEMPERATURE_KELVIN ), []( double in ) { return in + 273.15; } },
		{ get_unit_pair_( UNITS_TEMPERATURE_KELVIN, UNITS_TEMPERATURE_FAHRENHEIT ), []( double in ) { return ( ( in - 273.15 ) * 1.8 ) + 32.0; } },
		{ get_unit_pair_( UNITS_TEMPERATURE_KELVIN, UNITS_TEMPERATURE_CELSIUS ), []( double in ) { return in - 273.15; } },
		{ get_unit_pair_( UNITS_TEMPERATURE_FAHRENHEIT, UNITS_TEMPERATURE_KELVIN ), []( double in ) { return ( ( in - 32.0 ) * 5.0 / 9.0 ) + 273.15; } },
		{ get_unit_pair_( UNITS_TEMPERATURE_FAHRENHEIT, UNITS_TEMPERATURE_CELSIUS ), []( double in ) { return ( in - 32.0 ) * 5.0 / 9.0; } },
		{ get_unit_pair_( UNITS_DISTANCE_METERS, UNITS_DISTANCE_KILOMETERS ), []( double in ) { return in * 0.001; } },
		{ get_unit_pair_( UNITS_DISTANCE_KILOMETERS, UNITS_DISTANCE_METERS ), []( double in ) { return in * 1000.0; } },
		{ get_unit_pair_( UNITS_SPEED_METERS_PER_SECOND, UNITS_SPEED_KILOMETERS_PER_SECOND ), []( double in ) { return in * 0.001; } },
		{ get_unit_pair_( UNITS_SPEED_KILOMETERS_PER_SECOND, UNITS_SPEED_METERS_PER_SECOND ), []( double in ) { return in * 1000.0; } },
		{ get_unit_pair_( UNITS_PRESSURE_PASCALS, UNITS_PRESSURE_MILLIBARS ), []( double in ) { return in * 0.01; } },
		{ get_unit_pair_( UNITS_PRESSURE_MILLIBARS, UNITS_PRESSURE_PASCALS ), []( double in ) { return in * 100.0; } },
		{ get_unit_pair_( UNITS_DENSITY_KILOGRAMS_PER_CUBIC_METER, UNITS_DENSITY_GRAMS_PER_CUBIC_CENTIMETER ), []( double in ) { return in * 0.001; } },
		{ get_unit_pair_( UNITS_DENSITY_GRAMS_PER_CUBIC_CENTIMETER, UNITS_DENSITY_KILOGRAMS_PER_CUBIC_METER ), []( double in ) { return in * 1000.0; } },
		{ get_unit_pair_( UNITS_DIRECTION_DEGREES_CLOCKWISE_FROM_NORTH, UNITS_DIRECTION_DEGREES_COUNTERCLOCKWISE_FROM_EAST ), []( double in ) { 
			double out = 90.0 - in; 
			while (out < 0) {
				out += 360.0;
			}
			while (out >= 360.0) {
				out -= 360.0;
			}
			return out;
		} },
		{ get_unit_pair_( UNITS_DIRECTION_DEGREES_COUNTERCLOCKWISE_FROM_EAST, UNITS_DIRECTION_DEGREES_CLOCKWISE_FROM_NORTH ), []( double in ) { 
			double out = 90.0 - in; 
			while (out < 0) {
				out += 360.0;
			}
			while (out >= 360.0) {
				out -= 360.0;
			}
			return out;
		} }
	};

	// unit first derivative conversions.  Since most conversions are linear scaling, so are most derivative conversions, but some have constant offsets that
	// must be zeroed out.  Note: direction derivatives are not smart, as in they won't take into account phase wrapping
	// map_d1_ = {
	// 	{ get_unit_pair_( UNITS_TEMPERATURE_CELSIUS, UNITS_TEMPERATURE_FAHRENHEIT ), []( double in ) { return ( in * 1.8 ); } },
	// 	{ get_unit_pair_( UNITS_TEMPERATURE_CELSIUS, UNITS_TEMPERATURE_KELVIN ), []( double in ) { return in; } },
	// 	{ get_unit_pair_( UNITS_TEMPERATURE_KELVIN, UNITS_TEMPERATURE_FAHRENHEIT ), []( double in ) { return in * 1.8; } },
	// 	{ get_unit_pair_( UNITS_TEMPERATURE_KELVIN, UNITS_TEMPERATURE_CELSIUS ), []( double in ) { return in; } },
	// 	{ get_unit_pair_( UNITS_TEMPERATURE_FAHRENHEIT, UNITS_TEMPERATURE_KELVIN ), []( double in ) { return in * 5.0 / 9.0; } },
	// 	{ get_unit_pair_( UNITS_TEMPERATURE_FAHRENHEIT, UNITS_TEMPERATURE_CELSIUS ), []( double in ) { return in * 5.0 / 9.0; } },
	// 	{ get_unit_pair_( UNITS_DISTANCE_METERS, UNITS_DISTANCE_KILOMETERS ), []( double in ) { return in * 0.001; } },
	// 	{ get_unit_pair_( UNITS_DISTANCE_KILOMETERS, UNITS_DISTANCE_METERS ), []( double in ) { return in * 1000.0; } },
	// 	{ get_unit_pair_( UNITS_SPEED_METERS_PER_SECOND, UNITS_SPEED_KILOMETERS_PER_SECOND ), []( double in ) { return in * 0.001; } },
	// 	{ get_unit_pair_( UNITS_SPEED_KILOMETERS_PER_SECOND, UNITS_SPEED_METERS_PER_SECOND ), []( double in ) { return in * 1000.0; } },
	// 	{ get_unit_pair_( UNITS_PRESSURE_PASCALS, UNITS_PRESSURE_MILLIBARS ), []( double in ) { return in * 0.01; } },
	// 	{ get_unit_pair_( UNITS_PRESSURE_MILLIBARS, UNITS_PRESSURE_PASCALS ), []( double in ) { return in * 100.0; } },
	// 	{ get_unit_pair_( UNITS_DENSITY_KILOGRAMS_PER_CUBIC_METER, UNITS_DENSITY_GRAMS_PER_CUBIC_CENTIMETER ), []( double in ) { return in * 0.001; } },
	// 	{ get_unit_pair_( UNITS_DENSITY_GRAMS_PER_CUBIC_CENTIMETER, UNITS_DENSITY_KILOGRAMS_PER_CUBIC_METER ), []( double in ) { return in * 1000.0; } },
	// 	{ get_unit_pair_( UNITS_DIRECTION_DEGREES_CLOCKWISE_FROM_NORTH, UNITS_DIRECTION_DEGREES_COUNTERCLOCKWISE_FROM_EAST ), []( double in ) { return in; } },
	// 	{ get_unit_pair_( UNITS_DIRECTION_DEGREES_COUNTERCLOCKWISE_FROM_EAST, UNITS_DIRECTION_DEGREES_CLOCKWISE_FROM_NORTH ), []( double in ) { return in; } }
	// };

	// unit second derivative conversions.  Since most conversions are linear scaling, so are most derivative conversions, but some have constant offsets that
	// must be zeroed out.  There are currently no second-order conversions, so we use the same derivative conversions as the first-order ones.
	// map_d1_ = {
	// 	{ get_unit_pair_( UNITS_TEMPERATURE_CELSIUS, UNITS_TEMPERATURE_FAHRENHEIT ), []( double in ) { return ( in * 1.8 ); } },
	// 	{ get_unit_pair_( UNITS_TEMPERATURE_CELSIUS, UNITS_TEMPERATURE_KELVIN ), []( double in ) { return in; } },
	// 	{ get_unit_pair_( UNITS_TEMPERATURE_KELVIN, UNITS_TEMPERATURE_FAHRENHEIT ), []( double in ) { return in * 1.8; } },
	// 	{ get_unit_pair_( UNITS_TEMPERATURE_KELVIN, UNITS_TEMPERATURE_CELSIUS ), []( double in ) { return in; } },
	// 	{ get_unit_pair_( UNITS_TEMPERATURE_FAHRENHEIT, UNITS_TEMPERATURE_KELVIN ), []( double in ) { return in * 5.0 / 9.0; } },
	// 	{ get_unit_pair_( UNITS_TEMPERATURE_FAHRENHEIT, UNITS_TEMPERATURE_CELSIUS ), []( double in ) { return in * 5.0 / 9.0; } },
	// 	{ get_unit_pair_( UNITS_DISTANCE_METERS, UNITS_DISTANCE_KILOMETERS ), []( double in ) { return in * 0.001; } },
	// 	{ get_unit_pair_( UNITS_DISTANCE_KILOMETERS, UNITS_DISTANCE_METERS ), []( double in ) { return in * 1000.0; } },
	// 	{ get_unit_pair_( UNITS_SPEED_METERS_PER_SECOND, UNITS_SPEED_KILOMETERS_PER_SECOND ), []( double in ) { return in * 0.001; } },
	// 	{ get_unit_pair_( UNITS_SPEED_KILOMETERS_PER_SECOND, UNITS_SPEED_METERS_PER_SECOND ), []( double in ) { return in * 1000.0; } },
	// 	{ get_unit_pair_( UNITS_PRESSURE_PASCALS, UNITS_PRESSURE_MILLIBARS ), []( double in ) { return in * 0.01; } },
	// 	{ get_unit_pair_( UNITS_PRESSURE_MILLIBARS, UNITS_PRESSURE_PASCALS ), []( double in ) { return in * 100.0; } },
	// 	{ get_unit_pair_( UNITS_DENSITY_KILOGRAMS_PER_CUBIC_METER, UNITS_DENSITY_GRAMS_PER_CUBIC_CENTIMETER ), []( double in ) { return in * 0.001; } },
	// 	{ get_unit_pair_( UNITS_DENSITY_GRAMS_PER_CUBIC_CENTIMETER, UNITS_DENSITY_KILOGRAMS_PER_CUBIC_METER ), []( double in ) { return in * 1000.0; } },
	// 	{ get_unit_pair_( UNITS_DIRECTION_DEGREES_CLOCKWISE_FROM_NORTH, UNITS_DIRECTION_DEGREES_COUNTERCLOCKWISE_FROM_EAST ), []( double in ) { return in; } },
	// 	{ get_unit_pair_( UNITS_DIRECTION_DEGREES_COUNTERCLOCKWISE_FROM_EAST, UNITS_DIRECTION_DEGREES_CLOCKWISE_FROM_NORTH ), []( double in ) { return in; } }
	// };

	// take an enum value, output it's string value (full version)
	enum_to_string_map_ = {
		{ UNITS_NONE, "" },
		{ UNITS_TEMPERATURE_KELVIN, "degrees Kelvin" },
		{ UNITS_TEMPERATURE_CELSIUS, "degrees Celsius" },
		{ UNITS_TEMPERATURE_FAHRENHEIT, "degrees Fahrenheit" },
		{ UNITS_DISTANCE_METERS, "meters" },
		{ UNITS_DISTANCE_KILOMETERS, "kilometers" },
		{ UNITS_SPEED_METERS_PER_SECOND, "meters per second" },
		{ UNITS_SPEED_KILOMETERS_PER_SECOND, "kilometers per second" },
		{ UNITS_PRESSURE_PASCALS, "Pascals" },
		{ UNITS_PRESSURE_MILLIBARS, "millibars" },
		{ UNITS_DENSITY_KILOGRAMS_PER_CUBIC_METER, "kilograms per cubic meter" },
		{ UNITS_DENSITY_GRAMS_PER_CUBIC_CENTIMETER, "grams per cubic centimeter" },
		{ UNITS_DIRECTION_DEGREES_CLOCKWISE_FROM_NORTH, "degrees clockwise from North" },
		{ UNITS_DIRECTION_DEGREES_COUNTERCLOCKWISE_FROM_EAST, "degrees counterclockwise from East" },
		{ UNITS_ANGLE_DEGREES, "degrees" },
		{ UNITS_ANGLE_RADIANS, "radians" },
	};

	enum_to_abbr_map_ = {
		{ UNITS_NONE, "" },
		{ UNITS_TEMPERATURE_KELVIN, "K" },
		{ UNITS_TEMPERATURE_CELSIUS, "C" },
		{ UNITS_TEMPERATURE_FAHRENHEIT, "F" },
		{ UNITS_DISTANCE_METERS, "m" },
		{ UNITS_DISTANCE_KILOMETERS, "km" },
		{ UNITS_SPEED_METERS_PER_SECOND, "m/s" },
		{ UNITS_SPEED_KILOMETERS_PER_SECOND, "km/s" },
		{ UNITS_PRESSURE_PASCALS, "Pa" },
		{ UNITS_PRESSURE_MILLIBARS, "mbar" },
		{ UNITS_DENSITY_KILOGRAMS_PER_CUBIC_METER, "kg/m3" },
		{ UNITS_DENSITY_GRAMS_PER_CUBIC_CENTIMETER, "g/cm3" },
		{ UNITS_DIRECTION_DEGREES_CLOCKWISE_FROM_NORTH, "deg CW from N" },
		{ UNITS_DIRECTION_DEGREES_COUNTERCLOCKWISE_FROM_EAST, "deg CCW from E" },
		{ UNITS_ANGLE_DEGREES, "deg" },
		{ UNITS_ANGLE_RADIANS, "rad" },
	};

	string_to_enum_map_ = {
		{ "K", UNITS_TEMPERATURE_KELVIN },
		{ "DEGK", UNITS_TEMPERATURE_KELVIN },
		{ "DEG K", UNITS_TEMPERATURE_KELVIN },
		{ "DEGREES K", UNITS_TEMPERATURE_KELVIN },
		{ "C", UNITS_TEMPERATURE_CELSIUS },
		{ "DEGC", UNITS_TEMPERATURE_CELSIUS },
		{ "DEG C", UNITS_TEMPERATURE_CELSIUS },
		{ "DEGREES C", UNITS_TEMPERATURE_CELSIUS },
		{ "F", UNITS_TEMPERATURE_CELSIUS },
		{ "DEGF", UNITS_TEMPERATURE_CELSIUS },
		{ "DEG F", UNITS_TEMPERATURE_CELSIUS },
		{ "DEGREES F", UNITS_TEMPERATURE_CELSIUS },
		{ "M", UNITS_DISTANCE_METERS },
		{ "METERS", UNITS_DISTANCE_METERS },
		{ "KM", UNITS_DISTANCE_KILOMETERS },
		{ "KILOMETERS", UNITS_DISTANCE_KILOMETERS },
		{ "M/S", UNITS_SPEED_METERS_PER_SECOND },
		{ "MPS", UNITS_SPEED_METERS_PER_SECOND },
		{ "MPERS", UNITS_SPEED_METERS_PER_SECOND },
		{ "M PER S", UNITS_SPEED_METERS_PER_SECOND },
		{ "METERS PER SECOND", UNITS_SPEED_METERS_PER_SECOND },
		{ "KM/S", UNITS_SPEED_KILOMETERS_PER_SECOND },
		{ "KMPS", UNITS_SPEED_KILOMETERS_PER_SECOND },
		{ "KMPERS", UNITS_SPEED_KILOMETERS_PER_SECOND },
		{ "KM PER S", UNITS_SPEED_KILOMETERS_PER_SECOND },
		{ "KILOMETERS PER SECOND", UNITS_SPEED_KILOMETERS_PER_SECOND },
		{ "PA", UNITS_PRESSURE_PASCALS },
		{ "PASCAL", UNITS_PRESSURE_PASCALS },
		{ "PASCALS", UNITS_PRESSURE_PASCALS },
		{ "MBAR", UNITS_PRESSURE_MILLIBARS },
		{ "MILLIBAR", UNITS_PRESSURE_MILLIBARS },
		{ "MILLIBARS", UNITS_PRESSURE_MILLIBARS },
		{ "KG/M3", UNITS_DENSITY_KILOGRAMS_PER_CUBIC_METER },
		{ "KGPM3", UNITS_DENSITY_KILOGRAMS_PER_CUBIC_METER },
		{ "KILOGRAMS PER CUBIC METER", UNITS_DENSITY_KILOGRAMS_PER_CUBIC_METER },
		{ "G/CM3", UNITS_DENSITY_GRAMS_PER_CUBIC_CENTIMETER },
		{ "GPCM3", UNITS_DENSITY_GRAMS_PER_CUBIC_CENTIMETER },
		{ "GRAMS PER CUBIC CENTIMETER", UNITS_DENSITY_GRAMS_PER_CUBIC_CENTIMETER },
		{ "DEGREES CLOCKWISE FROM NORTH", UNITS_DIRECTION_DEGREES_CLOCKWISE_FROM_NORTH },
		{ "DEG CW FROM N", UNITS_DIRECTION_DEGREES_CLOCKWISE_FROM_NORTH },
		{ "AZIMUTH", UNITS_DIRECTION_DEGREES_CLOCKWISE_FROM_NORTH },
		{ "DEG CCW FROM E", UNITS_DIRECTION_DEGREES_COUNTERCLOCKWISE_FROM_EAST },
		{ "DEG", UNITS_ANGLE_DEGREES },
		{ "DEGREES", UNITS_ANGLE_DEGREES },
		{ "RAD", UNITS_ANGLE_RADIANS },
		{ "RADIANS", UNITS_ANGLE_RADIANS }
	};

	// map_ = {
	// 	{ get_unit_pair_( UNITS_TEMPERATURE_CELSIUS, UNITS_TEMPERATURE_FAHRENHEIT ), convert_temperature_c_to_f_ },
	// 	{ get_unit_pair_( UNITS_TEMPERATURE_CELSIUS, UNITS_TEMPERATURE_KELVIN ), convert_temperature_c_to_k_ },
	// 	{ get_unit_pair_( UNITS_TEMPERATURE_KELVIN, UNITS_TEMPERATURE_FAHRENHEIT ), convert_temperature_k_to_f_ },
	// 	{ get_unit_pair_( UNITS_TEMPERATURE_KELVIN, UNITS_TEMPERATURE_CELSIUS ), convert_temperature_k_to_c_ },
	// 	{ get_unit_pair_( UNITS_TEMPERATURE_FAHRENHEIT, UNITS_TEMPERATURE_KELVIN ), convert_temperature_f_to_k_ },
	// 	{ get_unit_pair_( UNITS_TEMPERATURE_FAHRENHEIT, UNITS_TEMPERATURE_CELSIUS ), convert_temperature_f_to_c_ },
	// 	{ get_unit_pair_( UNITS_DISTANCE_METERS, UNITS_DISTANCE_KILOMETERS ), convert_distance_m_to_km_ },
	// 	{ get_unit_pair_( UNITS_DISTANCE_KILOMETERS, UNITS_DISTANCE_METERS ), convert_temperature_f_to_c_ },
	// 	{ get_unit_pair_( UNITS_SPEED_METERS_PER_SECOND, UNITS_SPEED_KILOMETERS_PER_SECOND ), convert_speed_mps_to_kmps_ },
	// 	{ get_unit_pair_( UNITS_SPEED_KILOMETERS_PER_SECOND, UNITS_SPEED_METERS_PER_SECOND ), convert_speed_kmps_to_mps_ },
	// 	{ get_unit_pair_( UNITS_PRESSURE_PASCALS, UNITS_PRESSURE_MILLIBARS ), convert_pressure_pa_to_mbar_ },
	// 	{ get_unit_pair_( UNITS_PRESSURE_MILLIBARS, UNITS_PRESSURE_PASCALS ), convert_pressure_mbar_to_pa_ },
	// 	{ get_unit_pair_( UNITS_DENSITY_KILOGRAMS_PER_CUBIC_METER, UNITS_DENSITY_GRAMS_PER_CUBIC_CENTIMETER ), convert_density_kgpm3_to_gpcm3_ },
	// 	{ get_unit_pair_( UNITS_DENSITY_GRAMS_PER_CUBIC_CENTIMETER, UNITS_DENSITY_KILOGRAMS_PER_CUBIC_METER ), convert_density_gpcm3_to_kgpm3_ },
	// 	{ get_unit_pair_( UNITS_DIRECTION_DEGREES_CLOCKWISE_FROM_NORTH, UNITS_DIRECTION_DEGREES_COUNTERCLOCKWISE_FROM_EAST ), convert_direction_geo_to_math_ },
	// 	{ get_unit_pair_( UNITS_DIRECTION_DEGREES_COUNTERCLOCKWISE_FROM_EAST, UNITS_DIRECTION_DEGREES_CLOCKWISE_FROM_NORTH ), convert_direction_math_to_geo_ }
	// };


	
	// // set up valid unit pairs: temperature conversion
	// map_[ get_unit_pair_( UNITS_TEMPERATURE_CELSIUS, UNITS_TEMPERATURE_FAHRENHEIT ) ] 
	// 	= convert_temperature_c_to_f_;
	// map_[ get_unit_pair_( UNITS_TEMPERATURE_CELSIUS, UNITS_TEMPERATURE_KELVIN ) ] 
	// 	= convert_temperature_c_to_k_;
	// map_[ get_unit_pair_( UNITS_TEMPERATURE_KELVIN, UNITS_TEMPERATURE_FAHRENHEIT ) ] 
	// 	= convert_temperature_k_to_f_;
	// map_[ get_unit_pair_( UNITS_TEMPERATURE_KELVIN, UNITS_TEMPERATURE_CELSIUS ) ]
	// 	= convert_temperature_k_to_c_;
	// map_[ get_unit_pair_( UNITS_TEMPERATURE_FAHRENHEIT, UNITS_TEMPERATURE_KELVIN ) ] 
	// 	= convert_temperature_f_to_k_;
	// map_[ get_unit_pair_( UNITS_TEMPERATURE_FAHRENHEIT, UNITS_TEMPERATURE_CELSIUS ) ]
	// 	= convert_temperature_f_to_c_;
	
	// // length conversion
	// map_[ get_unit_pair_( UNITS_DISTANCE_METERS, UNITS_DISTANCE_KILOMETERS ) ] 
	// 	= convert_distance_m_to_km_;
	// map_[ get_unit_pair_( UNITS_DISTANCE_KILOMETERS, UNITS_DISTANCE_METERS ) ] 
	// 	= convert_distance_km_to_m_;
	
	// // speed conversion
	// map_[ get_unit_pair_( UNITS_SPEED_METERS_PER_SECOND, UNITS_SPEED_KILOMETERS_PER_SECOND ) ] 
	// 	= convert_speed_mps_to_kmps_;
	// map_[ get_unit_pair_( UNITS_SPEED_KILOMETERS_PER_SECOND, UNITS_SPEED_METERS_PER_SECOND ) ] 
	// 	= convert_speed_kmps_to_mps_;
	
	// // pressure conversion
	// map_[ get_unit_pair_( UNITS_PRESSURE_PASCALS, UNITS_PRESSURE_MILLIBARS ) ] 
	// 	= convert_pressure_pa_to_mbar_;
	// map_[ get_unit_pair_( UNITS_PRESSURE_MILLIBARS, UNITS_PRESSURE_PASCALS ) ] 
	// 	= convert_pressure_mbar_to_pa_;
	
	// // density conversion
	// map_[ get_unit_pair_( UNITS_DENSITY_KILOGRAMS_PER_CUBIC_METER, UNITS_DENSITY_GRAMS_PER_CUBIC_CENTIMETER ) ] 
	// 	= convert_density_kgpm3_to_gpcm3_;
	// map_[ get_unit_pair_( UNITS_DENSITY_GRAMS_PER_CUBIC_CENTIMETER, UNITS_DENSITY_KILOGRAMS_PER_CUBIC_METER ) ] 
	// 	= convert_density_gpcm3_to_kgpm3_;
	
	// // direction conversion
	// map_[ get_unit_pair_( UNITS_DIRECTION_DEGREES_CLOCKWISE_FROM_NORTH, UNITS_DIRECTION_DEGREES_COUNTERCLOCKWISE_FROM_EAST ) ] 
	// 	= convert_direction_geo_to_math_;
	// map_[ get_unit_pair_( UNITS_DIRECTION_DEGREES_COUNTERCLOCKWISE_FROM_EAST, UNITS_DIRECTION_DEGREES_CLOCKWISE_FROM_NORTH ) ] 
	// 	= convert_direction_math_to_geo_;


	// // First derivatives
	// // set up valid unit pairs: temperature conversion
	// map_d1_[ get_unit_pair_( UNITS_TEMPERATURE_CELSIUS, UNITS_TEMPERATURE_FAHRENHEIT ) ] 
	// 	= convert_temperature_c_to_f_deriv_;
	// map_d1_[ get_unit_pair_( UNITS_TEMPERATURE_CELSIUS, UNITS_TEMPERATURE_KELVIN ) ] 
	// 	= convert_temperature_c_to_k_deriv_;
	// map_d1_[ get_unit_pair_( UNITS_TEMPERATURE_KELVIN, UNITS_TEMPERATURE_FAHRENHEIT ) ] 
	// 	= convert_temperature_k_to_f_deriv_;
	// map_d1_[ get_unit_pair_( UNITS_TEMPERATURE_KELVIN, UNITS_TEMPERATURE_CELSIUS ) ]
	// 	= convert_temperature_k_to_c_deriv_;
	// map_d1_[ get_unit_pair_( UNITS_TEMPERATURE_FAHRENHEIT, UNITS_TEMPERATURE_KELVIN ) ] 
	// 	= convert_temperature_f_to_k_deriv_;
	// map_d1_[ get_unit_pair_( UNITS_TEMPERATURE_FAHRENHEIT, UNITS_TEMPERATURE_CELSIUS ) ]
	// 	= convert_temperature_f_to_c_deriv_;
	
	// // length conversion
	// map_d1_[ get_unit_pair_( UNITS_DISTANCE_METERS, UNITS_DISTANCE_KILOMETERS ) ] 
	// 	= convert_distance_m_to_km_deriv_;
	// map_d1_[ get_unit_pair_( UNITS_DISTANCE_KILOMETERS, UNITS_DISTANCE_METERS ) ] 
	// 	= convert_distance_km_to_m_deriv_;
	
	// // speed conversion
	// map_d1_[ get_unit_pair_( UNITS_SPEED_METERS_PER_SECOND, UNITS_SPEED_KILOMETERS_PER_SECOND ) ] 
	// 	= convert_speed_mps_to_kmps_deriv_;
	// map_d1_[ get_unit_pair_( UNITS_SPEED_KILOMETERS_PER_SECOND, UNITS_SPEED_METERS_PER_SECOND ) ] 
	// 	= convert_speed_kmps_to_mps_deriv_;
	
	// // pressure conversion
	// map_d1_[ get_unit_pair_( UNITS_PRESSURE_PASCALS, UNITS_PRESSURE_MILLIBARS ) ] 
	// 	= convert_pressure_pa_to_mbar_deriv_;
	// map_d1_[ get_unit_pair_( UNITS_PRESSURE_MILLIBARS, UNITS_PRESSURE_PASCALS ) ] 
	// 	= convert_pressure_mbar_to_pa_deriv_;
	
	// // density conversion
	// map_d1_[ get_unit_pair_( UNITS_DENSITY_KILOGRAMS_PER_CUBIC_METER, UNITS_DENSITY_GRAMS_PER_CUBIC_CENTIMETER ) ] 
	// 	= convert_density_kgpm3_to_gpcm3_deriv_;
	// map_d1_[ get_unit_pair_( UNITS_DENSITY_GRAMS_PER_CUBIC_CENTIMETER, UNITS_DENSITY_KILOGRAMS_PER_CUBIC_METER ) ] 
	// 	= convert_density_gpcm3_to_kgpm3_deriv_;
	
	// // direction conversion
	// map_d1_[ get_unit_pair_( UNITS_DIRECTION_DEGREES_CLOCKWISE_FROM_NORTH, UNITS_DIRECTION_DEGREES_COUNTERCLOCKWISE_FROM_EAST ) ] 
	// 	= convert_direction_geo_to_math_deriv_;
	// map_d1_[ get_unit_pair_( UNITS_DIRECTION_DEGREES_COUNTERCLOCKWISE_FROM_EAST, UNITS_DIRECTION_DEGREES_CLOCKWISE_FROM_NORTH ) ] 
	// 	= convert_direction_math_to_geo_deriv_;

	// // Second derivatives
	// // set up valid unit pairs: temperature conversion
	// map_d2_[ get_unit_pair_( UNITS_TEMPERATURE_CELSIUS, UNITS_TEMPERATURE_FAHRENHEIT ) ] 
	// 	= convert_temperature_c_to_f_deriv2_;
	// map_d2_[ get_unit_pair_( UNITS_TEMPERATURE_CELSIUS, UNITS_TEMPERATURE_KELVIN ) ] 
	// 	= convert_temperature_c_to_k_deriv2_;
	// map_d2_[ get_unit_pair_( UNITS_TEMPERATURE_KELVIN, UNITS_TEMPERATURE_FAHRENHEIT ) ] 
	// 	= convert_temperature_k_to_f_deriv2_;
	// map_d2_[ get_unit_pair_( UNITS_TEMPERATURE_KELVIN, UNITS_TEMPERATURE_CELSIUS ) ]
	// 	= convert_temperature_k_to_c_deriv2_;
	// map_d2_[ get_unit_pair_( UNITS_TEMPERATURE_FAHRENHEIT, UNITS_TEMPERATURE_KELVIN ) ] 
	// 	= convert_temperature_f_to_k_deriv2_;
	// map_d2_[ get_unit_pair_( UNITS_TEMPERATURE_FAHRENHEIT, UNITS_TEMPERATURE_CELSIUS ) ]
	// 	= convert_temperature_f_to_c_deriv2_;
	
	// // length conversion
	// map_d2_[ get_unit_pair_( UNITS_DISTANCE_METERS, UNITS_DISTANCE_KILOMETERS ) ] 
	// 	= convert_distance_m_to_km_deriv2_;
	// map_d2_[ get_unit_pair_( UNITS_DISTANCE_KILOMETERS, UNITS_DISTANCE_METERS ) ] 
	// 	= convert_distance_km_to_m_deriv2_;
	
	// // speed conversion
	// map_d2_[ get_unit_pair_( UNITS_SPEED_METERS_PER_SECOND, UNITS_SPEED_KILOMETERS_PER_SECOND ) ] 
	// 	= convert_speed_mps_to_kmps_deriv2_;
	// map_d2_[ get_unit_pair_( UNITS_SPEED_KILOMETERS_PER_SECOND, UNITS_SPEED_METERS_PER_SECOND ) ] 
	// 	= convert_speed_kmps_to_mps_deriv2_;
	
	// // pressure conversion
	// map_d2_[ get_unit_pair_( UNITS_PRESSURE_PASCALS, UNITS_PRESSURE_MILLIBARS ) ] 
	// 	= convert_pressure_pa_to_mbar_deriv2_;
	// map_d2_[ get_unit_pair_( UNITS_PRESSURE_MILLIBARS, UNITS_PRESSURE_PASCALS ) ] 
	// 	= convert_pressure_mbar_to_pa_deriv2_;
	
	// // density conversion
	// map_d2_[ get_unit_pair_( UNITS_DENSITY_KILOGRAMS_PER_CUBIC_METER, UNITS_DENSITY_GRAMS_PER_CUBIC_CENTIMETER ) ] 
	// 	= convert_density_kgpm3_to_gpcm3_deriv2_;
	// map_d2_[ get_unit_pair_( UNITS_DENSITY_GRAMS_PER_CUBIC_CENTIMETER, UNITS_DENSITY_KILOGRAMS_PER_CUBIC_METER ) ] 
	// 	= convert_density_gpcm3_to_kgpm3_deriv2_;
	
	// // direction conversion
	// map_d2_[ get_unit_pair_( UNITS_DIRECTION_DEGREES_CLOCKWISE_FROM_NORTH, UNITS_DIRECTION_DEGREES_COUNTERCLOCKWISE_FROM_EAST ) ] 
	// 	= convert_direction_geo_to_math_deriv2_;
	// map_d2_[ get_unit_pair_( UNITS_DIRECTION_DEGREES_COUNTERCLOCKWISE_FROM_EAST, UNITS_DIRECTION_DEGREES_CLOCKWISE_FROM_NORTH ) ] 
	// 	= convert_direction_math_to_geo_deriv2_;
}

bool NCPA::Units::ready_() {
	return !(map_.empty());
}

double NCPA::Units::convert( double in, NCPA::units_t type_in, NCPA::units_t type_out ) {
	double out = 0.0;
	try {
		// Call the vector conversion with one-element vectors
		NCPA::Units::convert( &in, 1, type_in, type_out, &out );
	} catch (const std::out_of_range& oor) {
		// didn't find the requested conversion, kick it upstairs, user
		// will have been notified in the called function
		throw;
	}
	return out;
}

// double NCPA::Units::convert_first_derivative( double in, NCPA::units_t type_in, NCPA::units_t type_out ) {
// 	double out = 0.0;
// 	try {
// 		// Call the vector conversion with one-element vectors
// 		NCPA::Units::convert_first_derivative( &in, 1, type_in, type_out, &out );
// 	} catch (const std::out_of_range& oor) {
// 		// didn't find the requested conversion, kick it upstairs, user
// 		// will have been notified in the called function
// 		throw;
// 	}
// 	return out;
// }

// double NCPA::Units::convert_second_derivative( double in, NCPA::units_t type_in, NCPA::units_t type_out ) {
// 	double out = 0.0;
// 	try {
// 		// Call the vector conversion with one-element vectors
// 		NCPA::Units::convert_second_derivative( &in, 1, type_in, type_out, &out );
// 	} catch (const std::out_of_range& oor) {
// 		// didn't find the requested conversion, kick it upstairs, user
// 		// will have been notified in the called function
// 		throw;
// 	}
// 	return out;
// }

/*
Converts one or more double values from one unit to another.
*/
void NCPA::Units::convert( const double *in, unsigned int nSamples, 
	NCPA::units_t type_in, NCPA::units_t type_out, double *out ) {
		
	// Is the conversion from one type to the same type?
	if (type_in == type_out) {
		
		// If the identity conversion is in place, just return
		if (in == out) {
			return;
		}
		
		// Copy the old values to the new values with no conversion
		std::memcpy( out, in, nSamples*sizeof(double) );
		return;
	}
	
	if ( ! NCPA::Units::ready_() ) {
		NCPA::Units::initialize_();
	}
	
	// Create a pair with the requested in and out units
	conversion_pair cpair( type_in, type_out );
	
	// function will go here if found
	conversion_function fPtr;
	try {
		// see if the requested conversion exists in the map.  If not, it throws
		// an out_of_range exception that is caught later
		fPtr = NCPA::Units::map_.at( cpair );
		
		// perform the requested conversion
		for (unsigned int i = 0; i < nSamples; i++) {
			out[ i ] = fPtr( in[ i ] );
		}
		
	} catch (const std::out_of_range& oor) {
		// didn't find the conversion, notify the user and kick it upstairs
		throw std::out_of_range( "Undefined conversion requested from " 
			+ NCPA::Units::toString( type_in ) + " to " + NCPA::Units::toString( type_out ) );
	}
}

// void NCPA::Units::convert_first_derivative( const double *in, unsigned int nSamples, 
// 	NCPA::units_t type_in, NCPA::units_t type_out, double *out ) {
		
// 	// Is the conversion from one type to the same type?
// 	if (type_in == type_out) {
		
// 		// If the identity conversion is in place, just return
// 		if (in == out) {
// 			return;
// 		}
		
// 		// Copy the old values to the new values with no conversion
// 		std::memcpy( out, in, nSamples*sizeof(double) );
// 		return;
// 	}
	
// 	if ( ! NCPA::Units::ready_() ) {
// 		NCPA::Units::initialize_();
// 	}
	
// 	// Create a pair with the requested in and out units
// 	conversion_pair cpair( type_in, type_out );
	
// 	// function will go here if found
// 	conversion_function fPtr;
// 	try {
// 		// see if the requested conversion exists in the map.  If not, it throws
// 		// an out_of_range exception that is caught later
// 		fPtr = NCPA::Units::map_d1_.at( cpair );
		
// 		// perform the requested conversion
// 		for (unsigned int i = 0; i < nSamples; i++) {
// 			out[ i ] = fPtr( in[ i ] );
// 		}
		
// 	} catch (const std::out_of_range& oor) {
// 		// didn't find the conversion, notify the user and kick it upstairs
// 		throw std::out_of_range( "Undefined first derivative conversion requested from " 
// 			+ NCPA::Units::toString( type_in ) + " to " + NCPA::Units::toString( type_out ) );
// 	}
// }

// void NCPA::Units::convert_second_derivative( const double *in, unsigned int nSamples, 
// 	NCPA::units_t type_in, NCPA::units_t type_out, double *out ) {
		
// 	// Is the conversion from one type to the same type?
// 	if (type_in == type_out) {
		
// 		// If the identity conversion is in place, just return
// 		if (in == out) {
// 			return;
// 		}
		
// 		// Copy the old values to the new values with no conversion
// 		std::memcpy( out, in, nSamples*sizeof(double) );
// 		return;
// 	}
	
// 	if ( ! NCPA::Units::ready_() ) {
// 		NCPA::Units::initialize_();
// 	}
	
// 	// Create a pair with the requested in and out units
// 	conversion_pair cpair( type_in, type_out );
	
// 	// function will go here if found
// 	conversion_function fPtr;
// 	try {
// 		// see if the requested conversion exists in the map.  If not, it throws
// 		// an out_of_range exception that is caught later
// 		fPtr = NCPA::Units::map_d2_.at( cpair );
		
// 		// perform the requested conversion
// 		for (unsigned int i = 0; i < nSamples; i++) {
// 			out[ i ] = fPtr( in[ i ] );
// 		}
		
// 	} catch (const std::out_of_range& oor) {
// 		// didn't find the conversion, notify the user and kick it upstairs
// 		throw std::out_of_range( "Undefined second derivative conversion requested from " 
// 			+ NCPA::Units::toString( type_in ) + " to " + NCPA::Units::toString( type_out ) );
// 	}
// }


/*
Generates and returns a std::pair with the two unit types, for use as a map key.
*/
conversion_pair NCPA::Units::get_unit_pair_( NCPA::units_t t1, NCPA::units_t t2 ) {
	return std::make_pair( t1, t2 );
}