#ifndef NCPAPROP_ATMOSPHERE1D_H_INCLUDED
#define NCPAPROP_ATMOSPHERE1D_H_INCLUDED

#include "AtmosphericModel.h"
#include "AtmosphericProperty1D.h"
#include "geographic.h"
#include "units.h"
#include <string>
#include <map>
#include <stack>
#include <fstream>

namespace NCPA {

	/**
	A 1-dimensional atmospheric profile.
	A class that represents a 1-dimensional atmospheric model with properties that are either scalars
	or vectors that depend on altitude.  This allows for storage of, for example, a reference 
	ground altitude that allows zero to be used for sea level and, therefore, multiple profiles
	to be compared to one another using a common datum.  All properties have associated units
	that can be tracked and converted internally.
	*/
	class Atmosphere1D : public AtmosphericModel {   // may take away the inheritance if we don't end up having multiples

	public:

		Atmosphere1D();

		/**
		Constructs an Atmosphere1D object using a vector of altitudes, to which corresponding
		properties can then be added.
		@brief Constructor from altitude vector.
		@param n_altitude_points The number of points in the altitude vector
		@param altitude_points A pointer to the first altitude point
		@param altitude_units A Units object indicating the distance units used.
		*/
		Atmosphere1D( size_t n_altitude_points, double *altitude_points, units_t altitude_units );

		/**
		Constructs an Atmosphere1D object using formatted input from an input stream, normally
		a std::ifstream representing a file.
		@brief Constructor from input stream.
		@param in The input stream to read the profile from.  Should already be open.
		*/
		Atmosphere1D( std::istream& in );

		/**
		Constructs an Atmosphere1D object using formatted input from a file.
		@brief Constructor from formatted file.
		@param filename The name of the file to be read from.
		*/
		Atmosphere1D( std::string filename );

		/**
		Copy constructor.
		@brief Copy constructor.
		@param source The Atmosphere1D object to be copied.
		*/
		Atmosphere1D( const Atmosphere1D &source );

		/**
		Cleans up all dynamically-allocated memory and calls its superclass destructor.
		@brief Destructor
		*/
		~Atmosphere1D();

		/**
		Reads a formatted profile from an input stream, normally a std::ifstream representing a file.
		@brief Read a formatted atmospheric profile from a stream.
		@param in The input stream to read the profile from.  Should already be open.
		*/
		void read_from_stream( std::istream& in );

		/**
		Adds a property, including units, that depends on the altitude to the atmosphere.
		@brief Adds a vector property to the atmosphere.
		@param key The name by which the property will be referred.  Needs to be unique.
		@param n_points The number of points in the property vector.
		@param quantity_points A pointer to the first value in the property vector.
		@param quantity_units The units of the quantity.
		@throws std::runtime_error if the key name is already in use
		@throws std::runtime_error if the number of points does not match the altitude vector
		*/
		void add_property( std::string key, size_t n_points, double *quantity_points, units_t quantity_units = NCPA::UNITS_NONE );

		/**
		Adds a scalar property, including units, to the atmosphere.
		@brief Adds a scalar property to the atmosphere.
		@param key The name by which the property will be referred.  Needs to be unique.
		@param value The value of the property.
		@param units The units of the property.
		@throws std::runtime_error if the key name is already in use
		*/
		void add_property( std::string key, double value, units_t units = NCPA::UNITS_NONE );

		void copy_vector_property( std::string old_key, std::string new_key );

		/**
		Removes a scalar or vector property from the atmosphere.  Has no effect if the property does not exist.
		@brief Removes a property.
		@param key The key for the property to be removed.
		*/
		void remove_property( std::string key );


		double get( std::string key ) const;    // scalars
		double get( std::string key, double altitude ) const;
		double get_first_derivative( std::string key, double altitude ) const;
		double get_second_derivative( std::string key, double altitude ) const;

		size_t get_basis_length() const;
		size_t nz() const;
		void get_altitude_vector( double *buffer, units_t *buffer_units ) const;
		void get_property_vector( std::string key, double *buffer, units_t *buffer_units ) const;
		void get_altitude_vector( double *buffer ) const;
		void get_property_vector( std::string key, double *buffer ) const;
		units_t get_altitude_units() const;
		units_t get_property_units( std::string key ) const;


		/**
		Returns the minimum valid altitude in the atmospheric model, i.e. the first value in the altitude
		vector, in the current units.
		@brief Returns the minimum valid altitude.
		@returns The minimum valid altitude in the current units.
		*/
		double get_minimum_altitude() const;
		double get_maximum_altitude() const;

		// derived quantities
		void calculate_sound_speed_from_temperature( std::string new_key, std::string temperature_key, units_t wind_units );
		void calculate_sound_speed_from_pressure_and_density( std::string new_key, std::string pressure_key, std::string density_key, units_t wind_units );
		void calculate_wind_speed( std::string new_key, std::string we_wind_speed_key, std::string sn_wind_speed_key );
		void calculate_wind_direction( std::string new_key, std::string we_wind_speed_key, std::string sn_wind_speed_key, 
			units_t direction_units = NCPA::UNITS_DIRECTION_DEGREES_CLOCKWISE_FROM_NORTH );
		void calculate_attenuation( std::string new_key, std::string temperature_key, std::string pressure_key, 
			std::string density_key, double freq, double tweak_factor = 1.0 );
		void calculate_wind_component( std::string new_key, std::string wind_speed_key, std::string wind_direction_key, 
			double azimuth );
		void calculate_effective_sound_speed( std::string new_key, std::string sound_speed_key, std::string wind_component_key );
		void read_attenuation_from_file( std::string new_key, std::string filename );
		

		void convert_altitude_units( units_t new_units );
		void convert_property_units( std::string key, units_t new_units );
		//units_t get_property_units( std::string key );

		void resample( double new_dz );

		std::vector< std::string > get_keys() const;
		bool contains_scalar( std::string key ) const;
		bool contains_vector( std::string key ) const;
		bool contains_key( std::string key ) const;

		void print_atmosphere( const std::vector< std::string >& columnorder, std::string altitude_key = "Z", std::ostream& os = std::cout );
		void print_atmosphere( std::string altitude_key = "Z", std::ostream& os = std::cout );

	protected:
		// internal storage
		std::map< std::string, NCPA::AtmosphericProperty1D * > contents_;
		std::map< std::string, NCPA::ScalarWithUnits * > scalar_contents_;
		
		//double *z_;
		//size_t nz_;
		//std::stack< units_t > z_units_;
		NCPA::VectorWithUnits *z_;

		void do_units_conversion_( size_t n_points, double *inplace, 
			NCPA::units_t fromUnits, NCPA::units_t toUnits );

		// @todo: location
	};

}

#endif
