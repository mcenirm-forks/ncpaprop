#include <complex>
#include <stdexcept>
#include <gsl/gsl_interp.h>
#include <gsl/gsl_spline.h>

#include "modes.h"
#include "Atmosphere1D.h"
#include "util.h"
#include "slepceps.h"
#include "slepcst.h"

#define MAX_MODES 4000 

using namespace NCPA;
using namespace std;

//
// constructor
//
NCPA::ModeSolver::ModeSolver( NCPA::ParameterSet *param, NCPA::Atmosphere1D *atm_profile )
{
	setParams( param, atm_profile );           
}

NCPA::ModeSolver::~ModeSolver() {
	delete[] Hgt;
	delete[] zw;
	delete[] mw;
	delete[] T;
	delete[] rho;
	delete[] Pr;
	delete[] c_eff;
	delete[] alpha;
	atm_profile->remove_property( "_WS_" );
	atm_profile->remove_property( "_WD_" );
	atm_profile->remove_property( "_C0_" );
	atm_profile->remove_property( "_ALPHA_" );
}


// setParams() prototype
void NCPA::ModeSolver::setParams( NCPA::ParameterSet *param, NCPA::Atmosphere1D *atm_prof )
{		

	// obtain the parameter values from the user's options
	atmosfile 			= param->getString( "atmosfile" );
	gnd_imp_model 		= param->getString( "ground_impedence_model" );
	usrattfile 			= param->getString( "use_attn_file" );
	modstartfile 		= param->getString( "modal_starter_file" );
  	z_min 				= param->getFloat( "zground_km" ) * 1000.0;    // meters
  	freq 				= param->getFloat( "freq" );
  	maxrange 			= param->getFloat( "maxrange_km" ) * 1000.0;
  	maxheight 			= param->getFloat( "maxheight_km" ) * 1000.0;      // @todo fix elsewhere that m is required
  	sourceheight 		= param->getFloat( "sourceheight_km" );
  	receiverheight 		= param->getFloat( "receiverheight_km" );
  	tol 				= 1.0e-8;
  	Nz_grid 			= param->getInteger( "Nz_grid" );
  	Nrng_steps 			= param->getInteger( "Nrng_steps" );
  	Lamb_wave_BC 		= param->getBool( "Lamb_wave_BC" );
  	write_2D_TLoss  	= param->getBool( "write_2D_TLoss" );
  	write_phase_speeds 	= param->getBool( "write_phase_speeds" );
  	write_speeds 		= param->getBool( "write_speeds" );
  	write_modes 		= param->getBool( "write_modes" );
  	write_dispersion 	= param->getBool( "write_dispersion" );
  	Nby2Dprop 			= param->getBool( "Nby2Dprop" );
  	turnoff_WKB 		= param->getBool( "turnoff_WKB" );
  	z_min_specified     = param->wasFound( "zground_km" );

	// default values for c_min, c_max and wvnum_filter_flg
	c_min = 0.0;
	c_max = 0.0;

	// set c_min, c_max if wavenumber filtering is on
	wvnum_filter_flg  	= param->getBool( "wvnum_filter" );  
	if (wvnum_filter_flg==1) {
		c_min = param->getFloat( "c_min" );
		c_max = param->getFloat( "c_max" );
	};


	if (write_phase_speeds || write_speeds || write_2D_TLoss 
			       || write_modes || write_dispersion) {
		turnoff_WKB = 1; // don't use WKB least phase speed estim. when saving any of the above values
	}
  
	Naz = 1; // Number of azimuths: default is a propagation along a single azimuth
	if (Nby2Dprop) {
		azi  		= param->getFloat( "azimuth_start" );
		azi_max 	= param->getFloat( "azimuth_end" );
		azi_step 	= param->getFloat( "azimuth_step" );
		Naz      = (int) ((azi_max - azi)/azi_step) + 1;
	} else {
		azi 				= param->getFloat( "azimuth" );
	}

	azi_min     = azi;
	atm_profile = atm_prof;

	// get Hgt, zw, mw, T, rho, Pr in SI units; they are freed in computeModes()
	// @todo write functions to allocate and free these, it shouldn't be hidden
	// Note units: height in m, wind speeds in m/s, pressure in Pa(?), density in
	// kg/m3
	Hgt   = new double [Nz_grid];
	zw    = new double [Nz_grid];
	mw    = new double [Nz_grid];
	T     = new double [Nz_grid];
	rho   = new double [Nz_grid];
	Pr    = new double [Nz_grid];
	c_eff = new double [Nz_grid]; // to be filled in getModalTrace(), depends on azimuth
	alpha = new double [Nz_grid];

	// Set up units of atmospheric profile object
	atm_profile->convert_altitude_units( Units::fromString( "m" ) );
	atm_profile->convert_property_units( "Z0", Units::fromString( "m" ) );
	atm_profile->convert_property_units( "U", Units::fromString( "m/s" ) );
	atm_profile->convert_property_units( "V", Units::fromString( "m/s" ) );
	atm_profile->convert_property_units( "T", Units::fromString( "K" ) );
	atm_profile->convert_property_units( "P", Units::fromString( "Pa" ) );
	atm_profile->convert_property_units( "RHO", Units::fromString( "kg/m3" ) );
  
	//
	// !!! ensure maxheight is less than the max height covered by the provided atm profile
	// may avoid some errors asociated with the code thinking that it goes above 
	// the max height when in fact the height may only differ from max height by
	// a rounding error. This should be revisited.
	// @todo revisit this
	// @todo add max_valid_height to AtmosphericProfile class
	
	if (maxheight >= atm_profile->get_maximum_altitude()) {
		maxheight = atm_profile->get_maximum_altitude() - 1e-6;
		cout << endl << "maxheight adjusted to: " << maxheight
			 << " m (max available in profile file)" << endl;
	}
  
	// fill and convert to SI units
	double dz       = (maxheight - z_min)/(Nz_grid - 1);	// the z-grid spacing
	//double z_min_km = z_min / 1000.0;
	//double dz_km    = dz / 1000.0;
  
	// Note: the rho, Pr, T, zw, mw are computed wrt ground level i.e.
	// the first value is at the ground level e.g. rho[0] = rho(z_min)
	// @todo make fill_vector( zvec ) methods in AtmosphericProfile()
	// @todo add underscores to internally calculated parameter keys
	atm_profile->calculate_sound_speed_from_pressure_and_density( "_C0_", "P", "RHO", Units::fromString( "m/s" ) );
	atm_profile->calculate_wind_speed( "_WS_", "U", "V" );
	atm_profile->calculate_wind_direction( "_WD_", "U", "V" );
	atm_profile->calculate_attenuation( "_ALPHA_", "T", "P", "RHO", freq );

	for (int i=0; i<Nz_grid; i++) {
		Hgt[i] = z_min + i*dz; // Hgt[0] = zground MSL
		rho[i] = atm_profile->get( "RHO", Hgt[i]);
		Pr[i]  = atm_profile->get( "P", Hgt[i] );
		T[i]   = atm_profile->get( "T", Hgt[i] );
		zw[i]  = atm_profile->get( "U", Hgt[i] );
		mw[i]  = atm_profile->get( "V", Hgt[i] );
		alpha[i]   = atm_profile->get( "_ALPHA_", Hgt[i] );
	}
}


// utility to print the parameters to the screen
void NCPA::ModeSolver::printParams() {
	printf(" Normal Modes Solver Parameters:\n");
	printf("                   freq : %g\n", freq);
	if (!Nby2Dprop) {
		printf("                azimuth : %g\n", azi);
	}
	else {
		printf("     azimuth_start (deg): %g\n", azi_min);
		printf("       azimuth_end (deg): %g\n", azi_max);
		printf("      azimuth_step (deg): %g\n", azi_step);
	}
	printf("                Nz_grid : %d\n", Nz_grid);
	printf("      z_min (meters MSL): %g\n", z_min);
	printf("      maxheight_km (MSL): %g\n", maxheight/1000.0);
	printf("   sourceheight_km (AGL): %g\n", sourceheight/1000.0);
	printf(" receiverheight_km (AGL): %g\n", receiverheight/1000.0);   
	printf("             Nrng_steps : %d\n", Nrng_steps);
	printf("            maxrange_km : %g\n", maxrange/1000.0); 
	printf("          gnd_imp_model : %s\n", gnd_imp_model.c_str());
	printf("Lamb wave boundary cond : %d\n", Lamb_wave_BC);
	printf("  SLEPc tolerance param : %g\n", tol);
	printf("    write_2D_TLoss flag : %d\n", write_2D_TLoss);
	printf("write_phase_speeds flag : %d\n", write_phase_speeds);
	printf("      write_speeds flag : %d\n", write_speeds);
	printf("  write_dispersion flag : %d\n", write_dispersion);
	printf("       write_modes flag : %d\n", write_modes);
	printf("         Nby2Dprop flag : %d\n", Nby2Dprop);
	printf("       turnoff_WKB flag : %d\n", turnoff_WKB);
	printf("    atmospheric profile : %s\n", atmosfile.c_str());
	if (!usrattfile.empty()) {
		printf("  User attenuation file : %s\n", usrattfile.c_str());
	}
	if (!modstartfile.empty()) {
		printf(" modal starter saved in : %s\n", modstartfile.c_str());
	}

	printf("       wvnum_filter_flg : %d\n", wvnum_filter_flg);
	if (wvnum_filter_flg==1) {
		printf("                  c_min : %g m/s\n", c_min);
		printf("                  c_max : %g m/s\n", c_max);
	}
}


int NCPA::ModeSolver::computeModessModes() {
	//
	// Declarations related to Slepc computations
	//
	Mat            A;           // problem matrix
	EPS            eps;         // eigenproblem solver context
	ST             stx;
	KSP            kspx;
	PC             pcx;
	EPSType        type;        // CHH 191022: removed const qualifier
	PetscReal      re, im;
	PetscScalar    kr, ki, *xr_;
	Vec            xr, xi;
	PetscInt       Istart, Iend, col[3], its, maxit, nconv;
	PetscBool      FirstBlock=PETSC_FALSE, LastBlock=PETSC_FALSE;
	PetscScalar    value[3];	
	PetscErrorCode ierr;
	PetscMPIInt    rank, size;

	int    i, j, select_modes, nev, it;
	double dz, admittance, h2, rng_step, z_min_km;
	double k_min, k_max;			
	double *diag, *k2, *k_s, **v, **v_s;
	complex<double> *k_pert;

	diag   = new double [Nz_grid];
  
	k2     = new double [MAX_MODES];
	k_s    = new double [MAX_MODES];
	k_pert = new complex<double> [MAX_MODES];
	v      = dmatrix(Nz_grid,MAX_MODES);
	v_s    = dmatrix(Nz_grid,MAX_MODES); 			

	nev   = 0;
	k_min = 0; 
	k_max = 0;  	

	rng_step = maxrange/Nrng_steps;         		// range step [meters]
	dz       = (maxheight - z_min)/(Nz_grid - 1);	// the z-grid spacing
	h2       = dz*dz;
	z_min_km = z_min / 1000.0;

	//
	// loop over azimuths (if not (N by 2D) it's only one azimuth)
	//
	for (it=0; it<Naz; it++) {
  
		azi = azi_min + it*azi_step; // degrees (not radians)
		cout << endl << "Now processing azimuth = " << azi << " (" << it+1 << " of " 
			 << Naz << ")" << endl;

		atm_profile->calculate_wind_component("_WC_", "_WS_", "_WD_", azi );
		atm_profile->calculate_effective_sound_speed( "_CE_", "_C0_", "_WC_" );
		atm_profile->get_property_vector( "_CE_", c_eff );
		atm_profile->add_property( "_AZ_", azi, NCPA::UNITS_DIRECTION_DEGREES_CLOCKWISE_FROM_NORTH );


		//
		// ground impedance model
		//
		// at the ground the BC is: Phi' = (a - 1/2*dln(rho)/dz)*Phi
		// for a rigid ground a=0; and the BC is the Lamb Wave BC:
		// admittance = -1/2*dln(rho)/dz
		//  
		admittance = 0.0;   // default
		if ( gnd_imp_model.compare( "rigid" ) == 0) {
			// Rigid ground, check for Lamb wave BC
			if (Lamb_wave_BC) {
				admittance = -atm_profile->get_first_derivative( "RHO", z_min ) 
					/ atm_profile->get( "RHO", z_min ) / 2.0;
				cout << "Admittance = " << admittance << endl;
			} else {
				admittance = 0.0;
			}
		} else {
			throw invalid_argument( 
				"This ground impedance model is not implemented yet: " + gnd_imp_model );
		}

		//
		// Get the main diagonal and the number of modes
		//		
		i = getModalTrace_Modess(Nz_grid, z_min, sourceheight, receiverheight, dz, atm_profile, 
				admittance, freq, azi, diag, &k_min, &k_max, turnoff_WKB, c_eff);
		
		// if wavenumber filtering is on, redefine k_min, k_max
		if (wvnum_filter_flg) {
			k_min = 2 * PI * freq / c_max;
			k_max = 2 * PI * freq / c_min;
		}

		i = getNumberOfModes(Nz_grid,dz,diag,k_min,k_max,&nev);
		
		printf ("______________________________________________________________________\n\n");
		printf (" -> Normal mode solution at %5.3f Hz and %5.2f deg (%d modes)...\n", freq, azi, nev);
		printf (" -> Discrete spectrum: %5.2f m/s to %5.2f m/s\n", 2*PI*freq/k_max, 2*PI*freq/k_min);
    
    	// Initialize Slepc
		SlepcInitialize(PETSC_NULL,PETSC_NULL,(char*)0,PETSC_NULL); /* @todo move out of loop? */
		ierr = MPI_Comm_rank(PETSC_COMM_WORLD,&rank); CHKERRQ(ierr);
		ierr = MPI_Comm_size(PETSC_COMM_WORLD,&size); CHKERRQ(ierr);  

		// Create the matrix A to use in the eigensystem problem: Ak=kx
		ierr = MatCreate(PETSC_COMM_WORLD,&A); CHKERRQ(ierr);
		ierr = MatSetSizes(A,PETSC_DECIDE,PETSC_DECIDE,Nz_grid,Nz_grid); CHKERRQ(ierr);
		ierr = MatSetFromOptions(A); CHKERRQ(ierr);
    
		// the following Preallocation call is needed in PETSc version 3.3
		ierr = MatSeqAIJSetPreallocation(A, 3, PETSC_NULL); CHKERRQ(ierr);
		// or use: ierr = MatSetUp(A); 

		/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
		Compute the operator matrix that defines the eigensystem, Ax=kx
		- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
		// Make matrix A 
		ierr = MatGetOwnershipRange(A,&Istart,&Iend);CHKERRQ(ierr);
		if (Istart==0) 
			FirstBlock=PETSC_TRUE;
		if (Iend==Nz_grid) 		/* @todo check if should be Nz_grid-1 */
			LastBlock=PETSC_TRUE;    
    
		value[0]=1.0/h2; 
		value[2]=1.0/h2;
		for( i=(FirstBlock? Istart+1: Istart); i<(LastBlock? Iend-1: Iend); i++ ) {
			value[1] = -2.0/h2 + diag[i];
			col[0]=i-1; 
			col[1]=i; 
			col[2]=i+1;
			ierr = MatSetValues(A,1,&i,3,col,value,INSERT_VALUES); CHKERRQ(ierr);
		}
		if (LastBlock) {
			i=Nz_grid-1; 
			col[0]=Nz_grid-2; 
			col[1]=Nz_grid-1;
			ierr = MatSetValues(A,1,&i,2,col,value,INSERT_VALUES); CHKERRQ(ierr);
		}
		if (FirstBlock) {
			i=0; 
			col[0]=0; 
			col[1]=1; 
			value[0]=-2.0/h2 + diag[0]; 
			value[1]=1.0/h2;
			ierr = MatSetValues(A,1,&i,2,col,value,INSERT_VALUES); CHKERRQ(ierr);
		}

		ierr = MatAssemblyBegin(A,MAT_FINAL_ASSEMBLY); CHKERRQ(ierr);
		ierr = MatAssemblyEnd(A,MAT_FINAL_ASSEMBLY); CHKERRQ(ierr);

		// CHH 191022: MatGetVecs() is deprecated, changed to MatCreateVecs()
		ierr = MatCreateVecs(A,PETSC_NULL,&xr); CHKERRQ(ierr);
		ierr = MatCreateVecs(A,PETSC_NULL,&xi); CHKERRQ(ierr);

		/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
		Create the eigensolver and set various options
		- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
		/* 
		Create eigensolver context
		*/
		ierr = EPSCreate(PETSC_COMM_WORLD,&eps); CHKERRQ(ierr);

		/* 
		Set operators. In this case, it is a standard eigenvalue problem
		*/
		ierr = EPSSetOperators(eps,A,PETSC_NULL); CHKERRQ(ierr);
		ierr = EPSSetProblemType(eps,EPS_HEP); CHKERRQ(ierr);

		/*
		Set solver parameters at runtime
		*/
		ierr = EPSSetFromOptions(eps);CHKERRQ(ierr);
		ierr = EPSSetType(eps,"krylovschur"); CHKERRQ(ierr);
		ierr = EPSSetDimensions(eps,10,PETSC_DECIDE,PETSC_DECIDE); CHKERRQ(ierr); // leaving this line in speeds up the code; better if this is computed in chunks of 10? - consult Slepc manual
		ierr = EPSSetTolerances(eps,tol,PETSC_DECIDE); CHKERRQ(ierr);

		ierr = EPSGetST(eps,&stx); CHKERRQ(ierr);
		ierr = STGetKSP(stx,&kspx); CHKERRQ(ierr);
		ierr = KSPGetPC(kspx,&pcx); CHKERRQ(ierr);
		ierr = STSetType(stx,"sinvert"); CHKERRQ(ierr);
		ierr = KSPSetType(kspx,"preonly");
		ierr = PCSetType(pcx,"cholesky");
		ierr = EPSSetInterval(eps,pow(k_min,2),pow(k_max,2)); CHKERRQ(ierr);
		ierr = EPSSetWhichEigenpairs(eps,EPS_ALL); CHKERRQ(ierr);
		
		/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
		Solve the eigensystem
		- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
		ierr = EPSSolve(eps);CHKERRQ(ierr);
		/*
		Optional: Get some information from the solver and display it
		*/
		ierr = EPSGetIterationNumber(eps,&its);CHKERRQ(ierr);
		ierr = EPSGetType(eps,&type);CHKERRQ(ierr);
		ierr = EPSGetDimensions(eps,&nev,PETSC_NULL,PETSC_NULL);CHKERRQ(ierr);
		ierr = EPSGetTolerances(eps,&tol,&maxit);CHKERRQ(ierr);
		
		/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
		Display solution and clean up
		- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
		/* 
		Get number of converged approximate eigenpairs
		*/
		ierr = EPSGetConverged(eps,&nconv);CHKERRQ(ierr);
		
		if (nconv>0) {
			for (i=0;i<nconv;i++) {
				ierr = EPSGetEigenpair(eps,i,&kr,&ki,xr,xi);CHKERRQ(ierr);
#if defined(PETSC_USE_COMPLEX)
				re = PetscRealPart(kr);
				im = PetscImaginaryPart(kr);
#else
				re = kr;
				im = ki;
#endif 
				k2[nconv-i-1] = re; // proper count of modes
				ierr = VecGetArray(xr,&xr_);CHKERRQ(ierr);
				for (j = 0; j < Nz_grid; j++) {
					v[j][nconv-i-1] = xr_[j]/sqrt(dz); //per Slepc the 2-norm of xr_ is=1; we need sum(v^2)*dz=1 hence the scaling xr_/sqrt(dz)
				}
				ierr = VecRestoreArray(xr,&xr_);CHKERRQ(ierr);
			}
		}	

		// select modes and do perturbation
		doSelect_Modess(Nz_grid,nconv,k_min,k_max,k2,v,k_s,v_s,&select_modes);
		doPerturb(Nz_grid, z_min, dz, select_modes, freq, k_s, v_s, alpha, k_pert);
		
		//
		// Output data  
		//
		if (Nby2Dprop) { // if (N by 2D is requested)
			getTLoss1DNx2(azi, select_modes, dz, Nrng_steps, rng_step, sourceheight, 
				      receiverheight, rho, k_pert, v_s, Nby2Dprop, it,
				      "Nby2D_tloss_1d.nm", "Nby2D_tloss_1d.lossless.nm" );
		}
		else {
			cout << "Writing to file: 1D transmission loss at the ground..." << endl;
			getTLoss1D(select_modes, dz, Nrng_steps, rng_step, sourceheight, receiverheight, rho, 
				   k_pert, v_s, "tloss_1d.nm", "tloss_1d.lossless.nm" );

			if ( !modstartfile.empty() ) {
				cout << "Writing to file: modal starter" << endl;
				
				// Modal starter - DV 20151014
				// Modification to apply the sqrt(k0) factor to agree with Jelle's getModalStarter; 
				// this in turn will make 'pape' agree with modess output
				getModalStarter(Nz_grid, select_modes, dz, freq, sourceheight, receiverheight, 
						rho, k_pert, v_s, modstartfile);       
			}

			if (write_2D_TLoss) {
				cout << "Writing to file: 2D transmission loss...\n";
				getTLoss2D(Nz_grid,select_modes,dz,Nrng_steps,rng_step,sourceheight,rho, 
					   k_pert,v_s, "tloss_2d.nm" );
			}

			if (write_phase_speeds) {
				cout << "Writing to file: phase speeds...\n";
				writePhaseSpeeds(select_modes,freq,k_pert);
			}

			if (write_modes) {
				cout << "Writing to file: the modes and the phase and group speeds...\n";
				writeEigenFunctions(Nz_grid,select_modes,dz,v_s);
				writePhaseAndGroupSpeeds(Nz_grid, dz, select_modes, freq, k_pert, v_s, c_eff);
			}
        
			if ((write_speeds) &&  !(write_modes)) {
				cout << "Writing to file: the modal phase speeds and the group speeds...\n";
				writePhaseAndGroupSpeeds(Nz_grid, dz, select_modes, freq, k_pert, v_s, c_eff);
			}        

			if (write_dispersion) {
				printf ("Writing to file: dispersion at freq = %8.3f Hz...\n", freq);
				writeDispersion(select_modes,dz,sourceheight,receiverheight,freq,k_pert,v_s, rho);
			}
		}
    
		// Free work space
		ierr = EPSDestroy(&eps);CHKERRQ(ierr);
		ierr = MatDestroy(&A);  CHKERRQ(ierr);
		ierr = VecDestroy(&xr); CHKERRQ(ierr);
		ierr = VecDestroy(&xi); CHKERRQ(ierr); 

		// Clean up azimuth-specific atmospheric properties before starting next run
		atm_profile->remove_property( "_WC_" );
		atm_profile->remove_property( "_CE_" );
		atm_profile->remove_property( "_AZ_" );
		
  
	} // end loop by azimuths
  
	// Finalize Slepc
	ierr = SlepcFinalize();CHKERRQ(ierr);
  
	// free the rest of locally dynamically allocated space
	delete[] diag;
	delete[] k2;
	delete[] k_s;
	delete[] k_pert;
	free_dmatrix(v, Nz_grid, MAX_MODES);
	free_dmatrix(v_s, Nz_grid, MAX_MODES);
  
	return 0;
} // end of computeModes()


int NCPA::ModeSolver::computeWModModes() {
  //
  // Declarations related to Slepc computations
  // 
  Mat            A, B;       
  EPS            eps;  		// eigenproblem solver context      
  ST             stx;
  EPSType  type;		// CHH 191028: removed const qualifier
  PetscReal      re, im;
  PetscScalar    kr, ki, *xr_;
  Vec            xr, xi;
  PetscInt       Istart, Iend, col[3], its, maxit, nconv;
  PetscBool      FirstBlock=PETSC_FALSE, LastBlock=PETSC_FALSE;
  PetscScalar    value[3];
  PetscErrorCode ierr;
  PetscMPIInt    rank, size;
  Vec            V_SEQ;
  VecScatter     ctx;
  

  int    i, j, select_modes, nev, it;
  double dz, admittance, h2, rng_step, z_min_km;
  double k_min, k_max, sigma;			
  double /* *alpha, */ *diag, *kd, *md, *cd, *kH, *k_s, **v, **v_s;	
  complex<double> *k_pert;

  //alpha  = new double [Nz_grid];
  diag   = new double [Nz_grid];
  kd     = new double [Nz_grid];
  md     = new double [Nz_grid];
  cd     = new double [Nz_grid];
  kH     = new double [MAX_MODES];
  k_s    = new double [MAX_MODES];  
  k_pert = new complex<double> [MAX_MODES];
  v      = dmatrix(Nz_grid,MAX_MODES);
  v_s    = dmatrix(Nz_grid,MAX_MODES); 	

  nev   = 0;
  k_min = 0; 
  k_max = 0;

  rng_step = maxrange/Nrng_steps;  		// range step [meters]
  dz       = (maxheight - z_min)/Nz_grid;	// the z-grid spacing
  h2       = dz*dz;
  //dz_km    = dz/1000.0;
  z_min_km = z_min/1000.0;
  
  //
  // loop over azimuths (if not (N by 2D) it's only one azimuth
  //
  for (it=0; it<Naz; it++) {
    
    azi = azi_min + it*azi_step; // degrees (not radians)
    cout << endl << "Now processing azimuth = " << azi << " (" << it+1 << " of " 
         << Naz << ")" << endl;

    //atm_profile->setPropagationAzimuth(azi);
    atm_profile->calculate_wind_component("_WC_", "_WS_", "_WD_", azi );
    atm_profile->calculate_effective_sound_speed( "_CE_", "_C0_", "_WC_" );
    atm_profile->get_property_vector( "_CE_", c_eff );
    atm_profile->add_property( "_AZ_", azi, NCPA::UNITS_DIRECTION_DEGREES_CLOCKWISE_FROM_NORTH );

    // compute absorption
    //getAbsorption(Nz_grid, dz, atm_profile, freq, usrattfile, alpha);

    //
    // ground impedance model
    //
    // at the ground the BC is: Phi' = (a - 1/2*dln(rho)/dz)*Phi
    // for a rigid ground a=0; and the BC is the Lamb Wave BC:
    // admittance = -1/2*dln(rho)/dz
    //  
    /*
    admittance = 0.0; // default
    if ((gnd_imp_model.compare("rigid")==0) && Lamb_wave_BC) { //Lamb_wave_BC
        admittance = -atm_profile->drhodz(z_min/1000.0)/1000.0/atm_profile->rho(z_min/1000.0)/2.0; // SI units
    }
    else if (gnd_imp_model.compare("rigid")==0) {
        admittance = 0.0; // no Lamb_wave_BC
    }
    else {
        std::ostringstream es;
        es << "This ground impedance model is not implemented yet: " << gnd_imp_model;
        throw invalid_argument(es.str());
    }
    */
    admittance = 0.0;   // default
    if ( gnd_imp_model.compare( "rigid" ) == 0) {
      // Rigid ground, check for Lamb wave BC
      if (Lamb_wave_BC) {
        admittance = -atm_profile->get_first_derivative( "RHO", z_min ) 
          / atm_profile->get( "RHO", z_min ) / 2.0;
        cout << "Admittance = " << admittance << endl;
      } else {
        admittance = 0.0;
      }
    } else {
      throw invalid_argument( 
        "This ground impedance model is not implemented yet: " + gnd_imp_model );
    }

    //
    // Get the main diagonal and the number of modes
    //		
    i = getModalTrace_WMod(Nz_grid, z_min, sourceheight, receiverheight, dz, atm_profile, 
      admittance, freq, diag, kd, md, cd, &k_min, &k_max, turnoff_WKB);

    // if wavenumber filtering is on, redefine k_min, k_max
    if (wvnum_filter_flg) {
        k_min = 2*PI*freq/c_max;
        k_max = 2*PI*freq/c_min;
    }

    i = getNumberOfModes(Nz_grid,dz,diag,k_min,k_max,&nev);
    
    //
    // set up parameters for eigenvalue estimation; double dimension of problem for linearization
    //

    //sigma = pow(0.5*(k_min+k_max),2);
    //sigma = (k_min*k_min + k_max*k_max)/2; //dv: !!! is this sigma better?  
    sigma     = 0.5*(k_min+k_max);
    
    int nev_2 = nev*2;
    int n_2   = Nz_grid*2;	

    // Initialize Slepc
    SlepcInitialize(PETSC_NULL,PETSC_NULL,(char*)0,PETSC_NULL);
    ierr = MPI_Comm_rank(PETSC_COMM_WORLD,&rank); CHKERRQ(ierr);
    ierr = MPI_Comm_size(PETSC_COMM_WORLD,&size); CHKERRQ(ierr);

    if (rank == 0) {
        printf ("______________________________________________________________________\n\n");
        printf (" -> Solving wide-angle problem at %6.3f Hz and %6.2f deg (%d modes)...\n", freq, azi, nev_2);
        printf (" -> Discrete spectrum: %6.2f m/s to %6.2f m/s\n", 2*PI*freq/k_max, 2*PI*freq/k_min);
        printf (" -> Quadratic eigenvalue problem  - double dimensionality.\n");
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
    //   Compute the operator matrices that define the eigensystem, A*x = k.B*x
    //   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    ierr = MatCreate(PETSC_COMM_WORLD,&A);CHKERRQ(ierr);
    ierr = MatSetSizes(A,PETSC_DECIDE,PETSC_DECIDE,n_2,n_2);CHKERRQ(ierr);
    ierr = MatSetFromOptions(A);CHKERRQ(ierr);
    ierr = MatCreate(PETSC_COMM_WORLD,&B);CHKERRQ(ierr);
    ierr = MatSetSizes(B,PETSC_DECIDE,PETSC_DECIDE,n_2,n_2);CHKERRQ(ierr);
    ierr = MatSetFromOptions(B);CHKERRQ(ierr);
    
    // the following Preallocation calls are needed in PETSc version 3.3
    ierr = MatSeqAIJSetPreallocation(A, 3, PETSC_NULL);CHKERRQ(ierr);
    ierr = MatSeqAIJSetPreallocation(B, 2, PETSC_NULL);CHKERRQ(ierr);

    /*
    We solve the quadratic eigenvalue problem (Mk^2 + Ck +D)v = 0 where k are the 
    eigenvalues and v are the eigenvectors and M , C, A are N by N matrices.
    We linearize this by denoting u = kv and arrive at the following generalized
    eigenvalue problem:

     / -D  0 \  (v )      / C  M \  (v ) 
    |         | (  ) = k |        | (  )
     \ 0   M /  (kv)      \ M  0 /  (kv)
     
     ie. of the form
     A*x = k.B*x
     so now we double the dimensions of the matrices involved: 2N by 2N.
     
     Matrix D is tridiagonal and has the form
     Main diagonal     : -2/h^2 + omega^2/c(1:N)^2 + F(1:N)
     Upper diagonal    :  1/h^2
     Lower diagonal    :  1/h^2
     Boundary condition:  A(1,1) =  (1/(1+h*beta) - 2)/h^2 + omega^2/c(1)^2 + F(1)
     
     where 
     F = 1/2*rho_0"/rho_0 - 3/4*(rho_0')^2/rho_0^2 where rho_0 is the ambient
     stratified air density; the prime and double prime means first and second 
     derivative with respect to z.
     beta  = alpha - 1/2*rho_0'/rho_0
     alpha is given in
     Psi' = alpha*Psi |at z=0 (i.e. at the ground). Psi is the normal mode.
     If the ground is rigid then alpha is zero. 
     
     Matrix M is diagonal:
     Main diagonal  : u0(1:N)^2/c(1:N)^2 - 1
      u0 = scalar product of the wind velocity and the horizontal wavenumber k_H
      u0 = v0.k_H
      
     Matrix C is diagonal:
     Main diagonal: 2*omega*u0(1:N)/c(1:N)^2 
     
    */

    // Assemble the A matrix (2N by 2N)
    ierr = MatGetOwnershipRange(A,&Istart,&Iend);CHKERRQ(ierr);
    if (Istart==0) FirstBlock=PETSC_TRUE;
    if (Iend==n_2) LastBlock =PETSC_TRUE;

    // matrix -D is placed in the first N by N block
    // kd[i]=(omega/c_T)^2
    for( i=(FirstBlock? Istart+1: Istart); i<(LastBlock? (Iend/2)-1: Iend/2); i++ ) {
        value[0]=-1.0/h2; value[1] = 2.0/h2 - kd[i]; value[2]=-1.0/h2;
        col[0]=i-1; col[1]=i; col[2]=i+1;
        ierr = MatSetValues(A,1,&i,3,col,value,INSERT_VALUES);CHKERRQ(ierr);
    }
    if (LastBlock) {
        i=(n_2/2)-1; col[0]=(n_2/2)-2; col[1]=(n_2/2)-1; value[0]=-1.0/h2; value[1]=2.0/h2 - kd[(n_2/2)-1];
        ierr = MatSetValues(A,1,&i,2,col,value,INSERT_VALUES);CHKERRQ(ierr);
    }

    // boundary condition
    if (FirstBlock) {
        i=0; col[0]=0; col[1]=1; value[0]=2.0/h2 - kd[0]; value[1]=-1.0/h2;
        ierr = MatSetValues(A,1,&i,2,col,value,INSERT_VALUES);CHKERRQ(ierr);
    }

    // Insert matrix M into the lower N by N block of A
    // md is u0^2/c^2-1
    for ( i=(n_2/2); i<n_2; i++ ) {
        ierr = MatSetValue(A,i,i,md[i-(n_2/2)],INSERT_VALUES); CHKERRQ(ierr); 
    }

    // Assemble the B matrix
    for ( i=0; i<(n_2/2); i++ ) {
        col[0]=i; col[1]=i+(n_2/2); value[0]=cd[i]; value[1]=md[i];
        ierr = MatSetValues(B,1,&i,2,col,value,INSERT_VALUES);CHKERRQ(ierr);
    }
    for ( i=(n_2/2); i<n_2; i++ ) {
        ierr = MatSetValue(B,i,i-(n_2/2),md[i-(n_2/2)],INSERT_VALUES); CHKERRQ(ierr); 
    }

    ierr = MatAssemblyBegin(A,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
    ierr = MatAssemblyEnd  (A,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
    ierr = MatAssemblyBegin(B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
    ierr = MatAssemblyEnd  (B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);

    // CHH 191028: MatGetVecs is deprecated, changed to MatCreateVecs
    ierr = MatCreateVecs(A,PETSC_NULL,&xr);CHKERRQ(ierr);
    ierr = MatCreateVecs(A,PETSC_NULL,&xi);CHKERRQ(ierr);
    //ierr = MatGetVecs(A,PETSC_NULL,&xr);CHKERRQ(ierr);
    //ierr = MatGetVecs(A,PETSC_NULL,&xi);CHKERRQ(ierr);

    /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
                  Create the eigensolver and set various options
       - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
    /* 
       Create eigensolver context
    */
    ierr = EPSCreate(PETSC_COMM_WORLD,&eps);CHKERRQ(ierr);

    /* 
       Set operators. In this case, it is a quadratic eigenvalue problem
    */
    ierr = EPSSetOperators(eps,A,B);CHKERRQ(ierr);
    ierr = EPSSetProblemType(eps,EPS_GNHEP);CHKERRQ(ierr);

    /*
       Set solver parameters at runtime
    */
    ierr = EPSSetFromOptions(eps);CHKERRQ(ierr);
    ierr = EPSSetType(eps,"krylovschur"); CHKERRQ(ierr);
    ierr = EPSSetDimensions(eps,nev_2,PETSC_DECIDE,PETSC_DECIDE); CHKERRQ(ierr);
    ierr = EPSSetTarget(eps,sigma); CHKERRQ(ierr);
    ierr = EPSSetTolerances(eps,tol,PETSC_DECIDE); CHKERRQ(ierr);

    ierr = EPSGetST(eps,&stx); CHKERRQ(ierr);
    ierr = STSetType(stx,"sinvert"); CHKERRQ(ierr);
    ierr = EPSSetWhichEigenpairs(eps,EPS_TARGET_MAGNITUDE); CHKERRQ(ierr);
    /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
                        Solve the eigensystem
       - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

    ierr = EPSSolve(eps);CHKERRQ(ierr);
    /*
       Optional: Get some information from the solver and display it
    */
    ierr = EPSGetIterationNumber(eps,&its);CHKERRQ(ierr);
    ierr = PetscPrintf(PETSC_COMM_WORLD," Number of iterations of the method: %d\n",its);CHKERRQ(ierr);
    ierr = EPSGetType(eps,&type);CHKERRQ(ierr);
    ierr = PetscPrintf(PETSC_COMM_WORLD," Solution method: %s\n\n",type);CHKERRQ(ierr);
    ierr = EPSGetDimensions(eps,&nev_2,PETSC_NULL,PETSC_NULL);CHKERRQ(ierr);
    ierr = PetscPrintf(PETSC_COMM_WORLD," Number of requested eigenvalues: %d\n",nev_2);CHKERRQ(ierr);
    ierr = EPSGetTolerances(eps,&tol,&maxit);CHKERRQ(ierr);
    ierr = PetscPrintf(PETSC_COMM_WORLD," Stopping condition: tol=%.4g, maxit=%d\n",tol,maxit);CHKERRQ(ierr);

    /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
                      Display solution and clean up
       - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
    /* 
       Get number of converged approximate eigenpairs
    */
    ierr = EPSGetConverged(eps,&nconv); CHKERRQ(ierr);
    ierr = PetscPrintf(PETSC_COMM_WORLD," Number of converged eigenpairs: %d\n\n",nconv);CHKERRQ(ierr);

    if (nconv>0) {
        for( i=0; i<nconv; i++ ) {
            ierr = EPSGetEigenpair(eps,i,&kr,&ki,xr,xi);CHKERRQ(ierr);
            #ifdef PETSC_USE_COMPLEX
                re = PetscRealPart(kr);
                im = PetscImaginaryPart(kr);
            #else
                re = kr;
                im = ki;
            #endif 
            kH[i] = re;
            ierr = VecScatterCreateToAll(xr,&ctx,&V_SEQ);CHKERRQ(ierr);
            ierr = VecScatterBegin(ctx,xr,V_SEQ,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
            ierr = VecScatterEnd(ctx,xr,V_SEQ,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
            if (rank == 0) {
                ierr = VecGetArray(V_SEQ,&xr_);CHKERRQ(ierr);
                for (j = 0; j < Nz_grid; j++) {
                    v[j][i] = xr_[j]/sqrt(dz);
                }
                ierr = VecRestoreArray(V_SEQ,&xr_);CHKERRQ(ierr);
            }
            ierr = VecScatterDestroy(&ctx);CHKERRQ(ierr);
            ierr = VecDestroy(&V_SEQ);CHKERRQ(ierr);
        }
    }

    // select modes and do perturbation
    doSelect_WMod(Nz_grid, nconv, k_min, k_max, kH, v, k_s, v_s, &select_modes);  
    doPerturb(Nz_grid, z_min, dz, select_modes, freq, k_s, v_s, alpha, k_pert);
    //cout << "Found " << select_modes << " relevant modes" << endl;

    //
    // Output data  
    //
    if (Nby2Dprop) { // if (N by 2D is requested)
        getTLoss1DNx2(azi, select_modes, dz, Nrng_steps, rng_step, sourceheight, 
        	receiverheight, rho, k_pert, v_s, Nby2Dprop, it,
        	"Nby2D_wtloss_1d.nm", "Nby2D_wtloss_1d.lossless.nm");
    } 
    else {  
        cout << "Writing to file: 1D transmission loss at the ground..." << endl;
        getTLoss1D(select_modes, dz, Nrng_steps, rng_step, sourceheight, receiverheight, 
        	rho, k_pert, v_s, "wtloss_1d.nm", "wtloss_1d.lossless.nm");

        if (write_2D_TLoss) {
            cout << "Writing to file: 2D transmission loss...\n";
            getTLoss2D(Nz_grid,select_modes,dz,Nrng_steps,rng_step,sourceheight,rho,
            	k_pert,v_s, "wtloss_2d.nm" );
        }

        if (write_phase_speeds) {
            cout << "Writing to file: phase speeds...\n";
            writePhaseSpeeds(select_modes,freq,k_pert);
        }

        if (write_modes) {
            cout << "Writing to file: the modes...\n";
            writeEigenFunctions(Nz_grid,select_modes,dz,v_s);
        }

        if (write_dispersion) {
            printf ("Writing to file: dispersion at freq = %8.3f Hz...\n", freq);
            //writeDispersion(select_modes,dz,sourceheight,receiverheight,freq,k_pert,v_s);
            writeDispersion(select_modes,dz,sourceheight,receiverheight,freq,k_pert,v_s, rho);
        }
    }
    
    // Free work space
    ierr = EPSDestroy(&eps);CHKERRQ(ierr);
    ierr = MatDestroy(&A);  CHKERRQ(ierr);
    ierr = MatDestroy(&B);  CHKERRQ(ierr);
    ierr = VecDestroy(&xr); CHKERRQ(ierr);
    ierr = VecDestroy(&xi); CHKERRQ(ierr); 
    
    atm_profile->remove_property( "_WC_" );
    atm_profile->remove_property( "_CE_" );
    atm_profile->remove_property( "_AZ_" );
  
  } // end loop by azimuths  
  
  // Finalize Slepc
  ierr = SlepcFinalize();CHKERRQ(ierr);
    
  // free the rest of locally dynamically allocated space
  delete[] diag;
  delete[] kd;
  delete[] md;
  delete[] cd;
  delete[] kH;
  delete[] k_s;
  delete[] k_pert;
  free_dmatrix(v, Nz_grid, MAX_MODES);
  free_dmatrix(v_s, Nz_grid, MAX_MODES);

  return 0;
}





// DV 20130827: new getModalTrace() to output the effective sound speed 
int NCPA::ModeSolver::getModalTrace_Modess( int nz, double z_min, double sourceheight, double receiverheight, 
	double dz, Atmosphere1D *p, double admittance, double freq, double azi, double *diag,
	double *k_min, double *k_max, bool turnoff_WKB, double *ceffz) {
	
	// use atmospherics input for the trace of the matrix.
	// the vector diag can be used to solve the modal problem
	// also returns the bounds on the wavenumber spectrum, [k_min,k_max]
	int    i, top;
	double azi_rad, z_km, z_min_km, dz_km, omega, gamma, bnd_cnd; 
	double cz, windz, ceffmin, ceffmax, ceff_grnd, cefftop; 
	double kk, dkk, k_eff, k_gnd, k_max_full, wkbIntegral, wkbTerm;
	z_min_km = z_min / 1000.0;
	dz_km    = dz / 1000.0;
	omega    = 2*PI*freq;
  
	azi_rad  = NCPA::Units::convert( p->get( "_AZ_" ), NCPA::UNITS_ANGLE_DEGREES, UNITS_ANGLE_RADIANS );
  
	gamma = 1.4;  
	
	z_km      = z_min_km;
	
	cz        = p->get( "_C0_", z_min );
	windz     = p->get( "_WC_", z_min );
	ceff_grnd = p->get( "_CE_", z_min );
	ceffmin   = ceff_grnd;  // in m/s; initialize ceffmin
	ceffmax   = ceffmin;    // initialize ceffmax   
	for (i=0; i<nz; i++) {	     
		ceffz[ i ] = p->get( "_CE_", Hgt[ i ] );
		// we neglect rho_factor - it does not make sense to have in this approximation
		//rho_factor is 1/2*rho_0"/rho_0 - 3/4*(rho_0')^2/rho_0^2
		diag[i] = pow( omega / ceffz[ i ], 2 );

		if (ceffz[i] < ceffmin)
			ceffmin = ceffz[i];  // approximation to find minimum sound speed in problem		   		
		if (ceffz[i] > ceffmax)
			ceffmax = ceffz[i];

		z_km += dz_km;		
	}
  
	bnd_cnd = (1./(dz*admittance+1))/(pow(dz,2)); // bnd cnd assuming centered fd
	diag[0] = bnd_cnd + diag[0]; 
  
	// use WKB trick for ground to ground propagation.
	if ((fabs(sourceheight)<1.0e-3) && (fabs(receiverheight)<1.0e-3) && (!turnoff_WKB)) {
		//
		// WKB trick for ground to ground propagation. 
		// Cut off lower phasespeed (highest wavenumber) where tunneling 
		// is insignificant (freq. dependent)
		//
		k_max_full = omega/ceffmin; 
		k_gnd      = omega/ceff_grnd;
		dkk        = (pow(k_max_full,2) - pow(k_gnd,2)) / 100.0;
    
		//
		// dv: when ceffmin is at the ground dkk can be very small but non-zero (rounding errors?)
		// in that case we need to skip the next {for loop}; otherwise it will be executed
		// with the small increment dkk
		//
		kk = pow(k_gnd,2);   //initialization of kk in case the loop is skipped
		if (dkk >1.0e-10) {  // do this loop only if dkk is nonzero (significantly)
			for (kk = pow(k_gnd,2); kk < pow(k_max_full,2); kk=kk+dkk) {
				wkbIntegral = 0.0;
				wkbTerm     = 1.0;  
				i           = 0;
				z_km        = z_min_km;
				while (wkbTerm > dkk) {
					k_eff       = omega/ceffz[i];
					wkbTerm     = abs(kk - pow(k_eff,2));
					wkbIntegral = wkbIntegral + dz*sqrt(wkbTerm); // dz should be in meters					
					i++;
					z_km += dz_km;
				} 

				if (wkbIntegral >= 10.0) {
					printf("\nWKB fix: new phasevelocity minimum= %6.2f m/s (was %6.2f m/s)\n", \
						omega/sqrt(kk), omega/k_max_full); 
					break;
				}
			}
		}  

		*k_max = sqrt(kk);  // use this for ground-to-ground 1D Tloss 
		//(uses WKB trick to include only non-vanishing modes at the ground)			  
		// *k_max = k_max_full; 
	} else { // not ground-to-ground propagation
		*k_max = omega/ceffmin; // same as k_max_full
	}  

	top     = nz - ((int) nz/10);
	z_km    = z_min_km + (top+1)*dz_km;  
	cz      = p->get( "_C0_", Hgt[ top+1 ] );
	windz   = p->get( "_WC_", Hgt[ top+1 ] );
	cefftop = p->get( "_CE_", Hgt[ top+1 ] );
	*k_min  = omega/cefftop;

	// optional save ceff
	// @todo add flag to turn on/off
	if (0) {
		double *target, *zvec;
		size_t nz = p->nz();
		target = new double[ nz ];
		zvec   = new double[ nz ];
		p->get_property_vector( "_CE_", target );
		p->get_altitude_vector( zvec );
		
		FILE *fp = fopen("ceff.nm", "w");    
		for ( size_t ii = 0; ii < nz; ii++ ) {     
			z_km = zvec[ ii ];
			fprintf(fp, "%8.3f %15.6e\n", zvec[ ii ], target[ ii ]);
		}
		fclose(fp);
		printf("ceff saved in ceff.nm\n");
		delete [] target;
		delete [] zvec;
	}

	return 0;
}


int NCPA::ModeSolver::getNumberOfModes(int n, double dz, double *diag, double k_min, double k_max, int *nev)
{
	int nev_max, nev_min;
	sturmCount(n,dz,diag,k_max,&nev_max);
	sturmCount(n,dz,diag,k_min,&nev_min);
	*nev = nev_max - nev_min;
	return 0;
}


int NCPA::ModeSolver::sturmCount(int n, double dz, double *diag, double k, int *cnt)
{
	double kk,pot,cup0,cup1,cup2;
	double fd_d_val, fd_o_val;
	int i,pm;

	fd_d_val = -2./pow(dz,2);  // Finite Difference Coefficient on  diagonal
	fd_o_val =  1./pow(dz,2);  // Finite Difference Coefficient off diagonal

	pm   = 0;
	kk   = k*k;
	cup0 = fd_d_val + diag[n-1] - kk;
	pot  = fd_d_val + diag[n-2] - kk;
	cup1 = cup0*pot;
	if (cup0*cup1 < 0.0) { pm++; } 
	cup0=cup0/fabs(cup1);
	cup1=cup1/fabs(cup1);

	for (i=n-3; i>=0; i--) {
		pot  = fd_d_val + diag[i] - kk;
		cup2 = pot*cup1 - (pow(fd_o_val,2))*cup0;
		if (cup1*cup2 < 0.0) { pm++; }
		cup0=cup1/fabs(cup2);
		cup1=cup2/fabs(cup2);
	}
	*cnt = pm;
	return 0;
}


int NCPA::ModeSolver::doPerturb(int nz, double z_min, double dz, int n_modes, double freq, /*SampledProfile *p,*/ 
		double *k, double **v, double *alpha, complex<double> *k_pert) {
	int i, j;
	double absorption, gamma, c_T;
	double z_km, dz_km;
	double omega = 2*PI*freq;
	complex<double> I (0.0, 1.0);
	gamma = 1.4;
	
	dz_km = dz/1000.0;
	for (j=0; j<n_modes; j++) {
		absorption = 0.0;
		z_km=z_min/1000.0;
		for (i=0; i<nz; i++) {
			c_T = sqrt(gamma*Pr[i]/rho[i]); // in m/s
			absorption = absorption + dz*v[i][j]*v[i][j]*(omega/c_T)*alpha[i]*2;
			z_km = z_km + dz_km;
		}			
		k_pert[j] = sqrt(k[j]*k[j] + I*absorption);
	}
	return 0;
}


int NCPA::ModeSolver::doSelect_Modess(int nz, int n_modes, double k_min, double k_max, 
	double *k2, double **v, double *k_s, double **v_s, int *select_modes)
{
	int cnt = -1;
	int i, j;
	double k;
	for (j=0; j<n_modes; j++) {
		k = sqrt(k2[j]);
		if ((k >= k_min) && (k <= k_max)) {
			cnt++;
			for (i=0; i<nz; i++) {
				v_s[i][cnt] = v[i][j];
			}
			k_s[cnt] = k;
		}
	}
	*select_modes = cnt + 1; //+1 because cnt started from zero
	return 0;
}

int NCPA::ModeSolver::doSelect_WMod(int nz, int n_modes, double k_min, double k_max, 
	double *kH, double **v, double *k_s, double **v_s, int *select_modes)
{
  int cnt = -1;
  int i, j;

  for (j=0; j<n_modes; j++) {
      if ((kH[j] >= k_min) && (kH[j] <= k_max)) {
          cnt++;
          for (i=0; i<nz; i++) {
              v_s[i][cnt] = v[i][j];
          }
          k_s[cnt] = kH[j];
      }
  }
  *select_modes = cnt + 1; //+1 because cnt started from zero
  return 0;
}


// 20150603 DV added code to account for sqrt(rho_rcv/rho_src) if so chosen
int NCPA::ModeSolver::getTLoss1D(int select_modes, double dz, int n_r, double dr, double z_src, 
		double z_rcv, double *rho, complex<double> *k_pert, double **v_s, 
		std::string filename_lossy, std::string filename_lossless ) {

	// computes the transmissison loss from source to receiver.
	// The formula for pressure is:
	// p(r,z) = sqrt(rho(z)/rho(zs))*I*exp(-I*pi/4)/sqrt(8*pi*r)*sum(Vm(z)*Vm(zs)*exp(I*k_m*r)/sqrt(k_m))
	// where Vm are the modes computed in this code. Note that Vm(z) = Psi_m(z)/sqrt(rho(z)), 
	// where Psi_m(z) are the modes defined in Ocean Acoustics, 1994 ed. pg. 274.
	// Note that we usually save the TL corresponding to
	// the reduced pressure formula: p_red(r,z) = p(r,z)/sqrt(rho(z))
	// check below to see which formula is actually implemented.
			
	int i, m;
	int n_zsrc = (int) ceil(z_src/dz);
	int n_zrcv = (int) ceil(z_rcv/dz);
	double r, sqrtrho_ratio;
	double modal_sum_i, modal_sum_i_ll; // for incoherent sum if desired
	complex<double> modal_sum_c, modal_sum_c_ll;
	complex<double> I (0.0, 1.0);
	
	// the 4*PI factor ensures that the modal sum below ends up being the actual TL
	complex<double> expov8pi =  4*PI*I*exp(-I*PI*0.25)/sqrt(8.0*PI); 
  
	sqrtrho_ratio  = sqrt(rho[n_zrcv]/rho[n_zsrc]);
  
	FILE *tloss_1d    = fopen( filename_lossy.c_str(), "w");
	FILE *tloss_ll_1d = fopen( filename_lossless.c_str(), "w");
	// @todo check that files were opened properly

	for (i=0; i<n_r; i++) {
		r = (i+1)*dr;
		modal_sum_c    = 0.;
		modal_sum_c_ll = 0;
		modal_sum_i    = 0;      
		modal_sum_i_ll = 0;
      
		// Use the commented lines if the modes must be scaled by sqrt(rho)
		// @todo under what conditions will this be the case?  If never, we should remove entirely
		/*
		if (1) {
		//cout << sqrtrho_ratio << endl;
		for (m=0; m<select_modes; m++) {
		modal_sum_c    = modal_sum_c + v_s[n_zsrc][m]*v_s[n_zrcv][m]*sqrtrho_ratio*exp(I*k_pert[m]*r)/sqrt(k_pert[m]);
		modal_sum_c_ll = modal_sum_c_ll + v_s[n_zsrc][m]*v_s[n_zrcv][m]*sqrtrho_ratio*exp(I*real(k_pert[m])*r)/sqrt(real(k_pert[m]));
          
		modal_sum_i    = modal_sum_i + pow(v_s[n_zsrc][m]*v_s[n_zrcv][m]*sqrtrho_ratio,2)*exp(-2*imag(k_pert[m])*r)/abs(k_pert[m]);
		modal_sum_i_ll = modal_sum_i_ll + pow(v_s[n_zsrc][m]*v_s[n_zrcv][m]*sqrtrho_ratio,2)/real(k_pert[m]);
		}
		}
		*/

		// here we save the modal sum; the reduced pressure is: 
		//     p_red(r,z) =modal_sum/(4*pi*sqrt(rho(zs)))= p(r,z)/sqrt(rho(z))
		for (m=0; m<select_modes; m++) {    
			modal_sum_c    = modal_sum_c + v_s[n_zsrc][m] * v_s[n_zrcv][m]
					 * exp(I*k_pert[m]*r) / sqrt(k_pert[m]);
			modal_sum_c_ll = modal_sum_c_ll + v_s[n_zsrc][m] * v_s[n_zrcv][m]
					 * exp(I*real(k_pert[m])*r) / sqrt(real(k_pert[m]));
			modal_sum_i    = modal_sum_i + pow(v_s[n_zsrc][m]*v_s[n_zrcv][m],2) 
					 * exp(-2*imag(k_pert[m])*r) / abs(k_pert[m]);
			modal_sum_i_ll = modal_sum_i_ll + pow(v_s[n_zsrc][m]*v_s[n_zrcv][m],2)
					 / real(k_pert[m]);
		}
      
		// no sqrt(rho[n_zrcv]/rho[n_zsrc]) factor
		if (1) {
			modal_sum_c    = expov8pi*modal_sum_c/sqrt(r);
			modal_sum_c_ll = expov8pi*modal_sum_c_ll/sqrt(r);
			modal_sum_i    = 4*PI*sqrt(modal_sum_i)*sqrt(1./8./PI/r);
			modal_sum_i_ll = 4*PI*sqrt(modal_sum_i_ll)*sqrt(1./8./PI/r);
		}
      
		// sqrt(rho[n_zrcv]/rho[n_zsrc]) factor added
		if (0) {
			modal_sum_c    = sqrtrho_ratio*expov8pi*modal_sum_c/sqrt(r);
			modal_sum_c_ll = sqrtrho_ratio*expov8pi*modal_sum_c_ll/sqrt(r);
			modal_sum_i    = 4*PI*sqrtrho_ratio*sqrt(modal_sum_i)*sqrt(1./8./PI/r);
			modal_sum_i_ll = 4*PI*sqrtrho_ratio*sqrt(modal_sum_i_ll)*sqrt(1./8./PI/r);
		}      

		fprintf(tloss_1d,"%f %20.12e %20.12e %20.12e\n", r/1000.0, real(modal_sum_c), 
			imag(modal_sum_c), modal_sum_i);
		fprintf(tloss_ll_1d,"%f %20.12e %20.12e %20.12e\n", r/1000.0, real(modal_sum_c_ll), 
			imag(modal_sum_c_ll), modal_sum_i_ll);
	}
	fclose(tloss_1d);
	fclose(tloss_ll_1d);
	printf("           file %s created\n", filename_lossy.c_str() );
	printf("           file %s created\n", filename_lossless.c_str() );
	return 0;
}

// Modal starter - DV 20151014
// Modification to apply the sqrt(k0) factor to agree with Jelle's getModalStarter; 
// this in turn will make 'pape' agree with modess output
void NCPA::ModeSolver::getModalStarter(int nz, int select_modes, double dz, double freq,  
	double z_src, double z_rcv, double *rho, complex<double> *k_pert, double **v_s, 
	string modstartfile) {

	// computes the modal starter field useful to ingest in a subsequent PE run.
	// The formula for the modal starter as in Ocean Acoustics, 1994 ed. eq. 6.72, pg. 361:
	// where Psi_m(z) are the modes defined in (Ocean Acoustics, 1994 ed. pg. 274.)
	// Eq. 6.72 is the "normalized" modal field which in this case means
	// that it is divided by (1/(4*PI)) i.e. multiplied by (4*PI)
	// the abs(pressure of point source in free space).
	// We do not need this normalization so we'll use formula 6.72 / (4*PI)
  
  
	// Psi_starter(z) = sqrt(2*PI)/(4*PI)*sqrt(rho(z))/sqrt(rho(zs))*sum( Vm(z)*Vm(zs)/sqrt(k_m) )
	// where Vm are the modes computed in this code. Note that Vm(z) = Psi_m(z)/sqrt(rho(z)), 
	// 

	int j, m;
	int n_zsrc = (int) ceil(z_src/dz);
	double z;
	complex<double> modal_sum;
  
	double k0 = (2*PI*freq/340.0); // reference wavenumber
	int z_cnd = int ((340.0/freq)/10/dz);
	FILE *mstfile = fopen(modstartfile.c_str(),"w");

	for (j=0; j<nz; j=j+z_cnd) {
		z = (j)*dz;
		modal_sum = 0.0;
		
		// Use the commented lines if the modes must be scaled by sqrt(rho)
		for (m=0; m<select_modes; m++) {
			modal_sum = modal_sum + v_s[n_zsrc][m]*v_s[j][m]/sqrt(real(k_pert[m]));
		}     
      
		//modal_sum = factor/twoPI*sqrtTwo*sqrtrhoj*modal_sum*sqrt(k0); // modal starter at particular z
      
		modal_sum = modal_sum * PI*sqrt(k0); // Jelle - needs the sqrt(k0)
		//modal_sum = factor/twoPI*sqrtTwo*sqrtrhoj*modal_sum; // modal starter at particular z
               
		//for (m=0; m<select_modes; m++) {
		//    modal_sum = modal_sum + v_s[n_zsrc][m]*v_s[j][m]/sqrt(k_pert[m]);
		//}
		//modal_sum = sqrt2PI/rhos*modal_sum; // modal starter at particular z
		//modal_sum = factor/rhos*modal_sum; // modal starter at particular z
      
      
		fprintf(mstfile,"%10.3f   %16.12e   %16.12e\n", z/1000.0, real(modal_sum), imag(modal_sum));
	}
	fprintf(mstfile,"\n");
	fclose(mstfile);
	printf("           file %s created\n", modstartfile.c_str());  

}


// 20150603 DV added code to account for sqrt(rho_rcv/rho_src) if so chosen
int NCPA::ModeSolver::getTLoss1DNx2(double azimuth, int select_modes, double dz, int n_r, double dr, 
	double z_src, double z_rcv, double *rho, complex<double> *k_pert, double **v_s, 
	bool Nx2, int iter, std::string filename_lossy, std::string filename_lossless ) {

	// !!! note the modes assumed here are the modes in Oc. Acoust. divided by sqrt(rho(z))
	// Thus the formula for (physical) pressure is:
	// p(r,z) = i*exp(-i*pi/4)/sqrt(8*pi*r)*1/rho(zs)* sum[ v_s(zs)*sqrt(rho(zs))*v_s(z)*sqrt(rho(z))
	// *exp(ikr)/sqrt(k) ]
	// We usually save the TL corresponding to the reduced pressure: p_red(r,z) = p(r,z)/sqrt(rho(z))

	int i, m;
	int n_zsrc = (int) ceil(z_src/dz);
	int n_zrcv = (int) ceil(z_rcv/dz);
	double r, sqrtrho_ratio;
	double modal_sum_i, modal_sum_i_ll;
	complex<double> modal_sum_c, modal_sum_c_ll;
	complex<double> I (0.0, 1.0);
	
	// the 4*PI factor ensures that the modal sum below ends up being the actual TL
	complex<double> expov8pi =  4*PI*I*exp(-I*PI*0.25)/sqrt(8.0*PI); 
	FILE *tloss_1d, *tloss_ll_1d;
  
	sqrtrho_ratio = sqrt(rho[n_zrcv]/rho[n_zsrc]);
	
	if (iter==0) {
		tloss_1d    = fopen(filename_lossy.c_str(),"w");
		tloss_ll_1d = fopen(filename_lossless.c_str(),"w");
	}
	else {  // append
		tloss_1d    = fopen(filename_lossy.c_str(),"a");
		tloss_ll_1d = fopen(filename_lossless.c_str(),"a");
	}
	// @todo make sure files were opened properly

	for (i=0; i<n_r; i++) {
		r = (i+1)*dr;
		modal_sum_c = 0.;
		modal_sum_i = 0.;
		modal_sum_c_ll = 0;
		modal_sum_i_ll = 0;
      
		// Use the commented lines if the modes must be scaled by sqrt(rho)
		// @todo under what circumstances should we activate these?
		/*
		for (m=0; m<select_modes; m++) {
		modal_sum_c    = modal_sum_c + v_s[n_zsrc][m]*v_s[n_zrcv][m]*sqrtrho_ratio*exp(I*k_pert[m]*r)/sqrt(k_pert[m]);
		modal_sum_i    = modal_sum_i + pow(v_s[n_zsrc][m]*v_s[n_zrcv][m]*sqrtrho_ratio,2)*exp(-2*imag(k_pert[m])*r)/abs(k_pert[m]);
		modal_sum_c_ll = modal_sum_c_ll + v_s[n_zsrc][m]*v_s[n_zrcv][m]*sqrtrho_ratio*exp(I*real(k_pert[m])*r)/sqrt(real(k_pert[m]));
		modal_sum_i_ll = modal_sum_i_ll + pow(v_s[n_zsrc][m]*v_s[n_zrcv][m]*sqrtrho_ratio,2)/real(k_pert[m]);
		}
		*/
      
		// here we save the modal sum; the reduced pressure is: p_red(r,z) =modal_sum/(4*pi*sqrt(rho(zs)))= p(r,z)/sqrt(rho(z))
		for (m=0; m<select_modes; m++) {
			modal_sum_c    = modal_sum_c 
				+ (v_s[n_zsrc][m] * v_s[n_zrcv][m] * exp(I*k_pert[m]*r) / sqrt(k_pert[m]));
			modal_sum_c_ll = modal_sum_c_ll 
				+ v_s[n_zsrc][m]*v_s[n_zrcv][m]*exp(I*real(k_pert[m])*r)/sqrt(real(k_pert[m]));
			modal_sum_i    = modal_sum_i    
				+ pow(v_s[n_zsrc][m]*v_s[n_zrcv][m],2)*exp(-2*imag(k_pert[m])*r)/abs(k_pert[m]);
			modal_sum_i_ll = modal_sum_i_ll + pow(v_s[n_zsrc][m]*v_s[n_zrcv][m],2)/real(k_pert[m]);
		}
      
      
		// no sqrt(rho[n_zrcv]/rho[n_zsrc]) factor
		// @todo Need programmatic logic to determine which branch to take, or remove one
		if (1) {
			modal_sum_c    = expov8pi*modal_sum_c/sqrt(r);
			modal_sum_c_ll = expov8pi*modal_sum_c_ll/sqrt(r);
			modal_sum_i    = 4*PI*sqrt(modal_sum_i)*sqrt(1./8./PI/r);
			modal_sum_i_ll = 4*PI*sqrt(modal_sum_i_ll)*sqrt(1./8./PI/r);
		}
      
		// sqrt(rho[n_zrcv]/rho[n_zsrc]) factor added
		if (0) {
			modal_sum_c    = sqrtrho_ratio*expov8pi*modal_sum_c/sqrt(r);
			modal_sum_c_ll = sqrtrho_ratio*expov8pi*modal_sum_c_ll/sqrt(r);
			modal_sum_i    = 4*PI*sqrtrho_ratio*sqrt(modal_sum_i)*sqrt(1./8./PI/r);
			modal_sum_i_ll = 4*PI*sqrtrho_ratio*sqrt(modal_sum_i_ll)*sqrt(1./8./PI/r);
		}       
      
		fprintf(tloss_1d,"%10.3f %8.3f %20.12e %20.12e %20.12e\n", r/1000.0, 
			azimuth, real(modal_sum_c), imag(modal_sum_c), modal_sum_i);
		fprintf(tloss_ll_1d,"%10.3f %8.3f %20.12e %20.12e %20.12e\n", r/1000.0, 
			azimuth, real(modal_sum_c_ll), imag(modal_sum_c_ll), modal_sum_i_ll);
	}
	fprintf(tloss_1d, "\n");
	fprintf(tloss_ll_1d, "\n");
  
	fclose(tloss_1d);
	fclose(tloss_ll_1d);
	return 0;
}

// 20150603 DV added code to account for sqrt(rho_rcv/rho_src) if so chosen
int NCPA::ModeSolver::getTLoss2D(int nz, int select_modes, double dz, int n_r, double dr, 
	double z_src, double *rho, complex<double> *k_pert, double **v_s, 
	std::string filename_lossy) {

	// !!! note the modes assumed here are the modes in Oc. Acoust. divided by sqrt(rho(z))
	// Thus the formula for (physical) pressure is:
	// p(r,z) = i*exp(-i*pi/4)/sqrt(8*pi*r)*1/rho(zs)* sum[ v_s(zs)*sqrt(rho(zs))*v_s(z)*sqrt(rho(z))
	// *exp(ikr)/sqrt(k) ]
	// We usually save the TL corresponding to the reduced pressure: p_red(r,z) = p(r,z)/sqrt(rho(z))
  
	int i, j, m, stepj;
	int n_zsrc = (int) ceil(z_src/dz);
	double r, z, sqrtrhoj, rho_atzsrc;
	complex<double> modal_sum;
	complex<double> I (0.0, 1.0);
	
	// the 4*PI factor ensures that the modal sum below ends up being the actual TL
	complex<double> expov8pi = 4*PI*I*exp(-I*PI*0.25)/sqrt(8.0*PI); 
  
	rho_atzsrc = rho[n_zsrc];

	stepj = nz/500; // controls the vertical sampling of 2D data saved 
	if (stepj==0) {
		stepj = 10;	// ensure it's never 0; necessary for the loop below
	}

	FILE *tloss_2d = fopen(filename_lossy.c_str(),"w");

	for (i=0; i<n_r; i++) {
		r = (i+1)*dr;
		for (j=0; j<nz; j=j+stepj) {
			z = (j)*dz;
			sqrtrhoj = sqrt(rho[j]);
			modal_sum = 0.;
          
			// Use the commented lines if the modes must be scaled by sqrt(rho)
			// @todo do we need these?
			/*
			if (1) {
			for (m=0; m<select_modes; m++) {
			modal_sum = modal_sum + v_s[n_zsrc][m]*v_s[j][m]*sqrtrhoj*exp(I*k_pert[m]*r)/sqrt(k_pert[m]);
			}
			modal_sum = expov8pi*modal_sum/sqrt(r*rho_atzsrc);
			}
			*/
                  
			for (m=0; m<select_modes; m++) {
				modal_sum = modal_sum + v_s[n_zsrc][m]*v_s[j][m]*exp(I*k_pert[m]*r)/sqrt(k_pert[m]);
			}
			modal_sum = expov8pi*modal_sum/sqrt(r); // no sqrt(rho[n_zrcv]/rho[n_zsrc]) factor

			// @todo do we need this?
			if (0) {
				// sqrt(rho[n_zrcv]/rho[n_zsrc]) factor added
				modal_sum = sqrtrhoj/sqrt(rho_atzsrc)*modal_sum/sqrt(r);
			}           

			fprintf(tloss_2d,"%f %f %15.8e %15.8e\n", r/1000.0, z/1000.0, 
				real(modal_sum), imag(modal_sum));
		}
		fprintf(tloss_2d,"\n");
	}
	fclose(tloss_2d);
	printf("           file %s created\n", filename_lossy.c_str() );
	return 0;
}


// DV 20150409 - added rho as argument
int NCPA::ModeSolver::writeDispersion(int select_modes, double dz, double z_src, double z_rcv, 
	double freq, complex<double> *k_pert, double **v_s, double *rho) {
	int i;
	int n_zsrc = (int) ceil(z_src/dz);
	int n_zrcv = (int) ceil(z_rcv/dz);
	char dispersion_file[128];
  
	//printf("In writeDispersion(): n_zsrc=%d ; n_zrcv=%d; z_src=%g; z_rcv=%g\n", n_zsrc, n_zrcv, z_src, z_rcv);

	sprintf(dispersion_file,"dispersion_%e.nm", freq);
	FILE *dispersion = fopen(dispersion_file,"w");
	fprintf(dispersion,"%.12e   %d    %.12e", freq, select_modes, rho[n_zsrc]);
	for( i=0; i<select_modes; i++ ) {
		fprintf(dispersion,"   %.12e   %.12e",real(k_pert[i]),imag(k_pert[i]));
		//fprintf(dispersion,"   %.12e   %.12e",real(v_s[n_zsrc][i]),real(v_s[0][i]));
		fprintf(dispersion,"   %.12e   %.12e",(v_s[n_zsrc][i]),(v_s[n_zrcv][i]));
	}
	fprintf(dispersion,"\n");
	fclose(dispersion);
	printf("           file %s created\n", dispersion_file);
	return 0;
}
 

int NCPA::ModeSolver::writePhaseSpeeds(int select_modes, double freq, complex<double> *k_pert)
{
	int j;
	FILE *phasespeeds= fopen("phasespeeds.nm", "w");
	// @todo check to see if file is opened
	for (j=0; j< select_modes; j++) {
		fprintf(phasespeeds, "%d %f %15.8e\n", j, (2*PI*freq)/real(k_pert[j]), imag(k_pert[j]));
	}
	fclose(phasespeeds);
	printf("           file phasespeeds.nm created\n");
	return 0;
}



int NCPA::ModeSolver::writeEigenFunctions(int nz, int select_modes, double dz, double **v_s)
{
	char mode_output[40];
	int j, n;
	double chk;
	double dz_km = dz/1000.0;

	for (j=0; j<select_modes; j++) {
		sprintf(mode_output,"mode_%d.nm", j);
		FILE *eigenfunction= fopen(mode_output, "w");
		chk = 0.0;
		for (n=0; n<nz; n++) {
			fprintf(eigenfunction,"%f %15.8e\n", n*dz_km, v_s[n][j]);
			chk = chk + v_s[n][j]*v_s[n][j]*dz;
		}
		if (fabs(1.-chk) > 0.1) { 
			printf("Check if eigenfunction %d is normalized!\n", j); 
		}
		fclose(eigenfunction);
	}
	printf("           files mode_<mode_number> created (%d in total)\n", select_modes);  
	return 0;
}


// compute/save the group and phase speeds
int NCPA::ModeSolver::writePhaseAndGroupSpeeds(int nz, double dz, int select_modes, double freq, 
	complex<double> *k_pert, double **v_s, double *c_eff) {
	int j, n;
	double v_phase, v_group;
	double omega = 2*PI*freq;
  
	FILE *speeds= fopen("speeds.nm", "w");
	for (j=0; j< select_modes; j++) {
		v_phase = omega/real(k_pert[j]);
      
		// compute the group speed: vg = v_phase*int_0^z_max Psi_j^2/c_eff^2 dz_km
		v_group = 0.0;
		for (n=0; n<nz; n++) {
			v_group = v_group + v_s[n][j]*v_s[n][j]/(c_eff[n]*c_eff[n]);   
		}
		v_group = v_group*v_phase*dz;
		v_group = 1.0/v_group;
      
		fprintf(speeds, "%4d %9.3f %9.3f %15.8e\n", j+1, v_phase, v_group, imag(k_pert[j]));    
	}
	fclose(speeds);
	printf("           Phase and group speeds saved in file speeds.nm .\n");
	return 0;
}


int NCPA::ModeSolver::getModalTrace_WMod(int nz, double z_min, double sourceheight, double receiverheight, 
  double dz, Atmosphere1D *p, double admittance, double freq, double *diag, double *kd, double *md, 
  double *cd, double *k_min, double *k_max, bool turnoff_WKB)
{
  // DV Note: Claus's profile->ceff() computes ceff = sqrt(gamma*R*T) + wind; 
  // this version of getModalTrace computes ceff = sqrt(gamma*P/rho) + wind.  
    
  //     use atmospherics input for the trace of the matrix.
  //     the vector diag can be used to solve the modal problem
  //     also returns the bounds on the wavenumber spectrum, [k_min,k_max]

  int i, top;
  double azi_rad, gamma, omega, bnd_cnd, windz, ceffmin, ceffmax, ceff_grnd, cefftop, cz;
  double z_min_km, z_km, dz_km;
  double kk, dkk, k_eff, k_gnd, k_max_full, wkbIntegral, wkbTerm;
  //double rho_factor; 
  double *ceffz;
  ceffz = new double [nz];

  //FILE *profile= fopen("profile.int", "w");

  gamma    = 1.4;
  z_min_km = z_min/1000.0;
  dz_km    = dz/1000.0;
  omega    = 2*PI*freq;

  azi_rad  = NCPA::Units::convert( p->get( "_AZ_" ), NCPA::UNITS_ANGLE_DEGREES, UNITS_ANGLE_RADIANS );

  z_km      = z_min_km; 
  cz        = p->get( "_C0_", z_min );
  windz     = p->get( "_WC_", z_min );
  ceff_grnd = p->get( "_CE_", z_min );
  ceffmin   = ceff_grnd;  // in m/s; initialize ceffmin
  ceffmax   = ceffmin;    // initialize ceffmax   

  for (i=0; i<nz; i++) {
     cz         = p->get( "_C0_", Hgt[ i ] );
	    ceffz[ i ] = p->get( "_CE_", Hgt[ i ] );
      windz      = p->get( "_WC_", Hgt[ i ] );

	    // we neglect rho_factor - it does not make sense to have in this approximation
	    //rho_factor is 1/2*rho_0"/rho_0 - 3/4*(rho_0')^2/rho_0^2
	    //rho_factor = (-3/4*pow(p->drhodz(z_km)/p->rho(z_km),2) + 1/2*p->ddrhodzdz(z_km)/p->rho(z_km))*1e-6; // in meters^(-2)!
	    kd[i]      = pow(omega/cz,2); // + rho_factor;
	    md[i]      = pow(windz/cz,2) - 1;
	    cd[i]      = -2*omega*(windz/pow(cz,2));
	    diag[i]    = pow(omega/ceffz[ i ],2); // + rho_factor;
	    if (ceffz[ i ] < ceffmin)
	     		ceffmin = ceffz[ i ];     // approximation to find minimum sound speed in problem
	    if (ceffz[ i ] > ceffmax)
	     		ceffmax = ceffz[ i ];
	     
	    //z_km += dz_km; 
  }
  //fclose(profile);

  bnd_cnd = (1./(dz*admittance+1))/(pow(dz,2)); // bnd cnd assuming centered fd
  diag[0] = bnd_cnd + diag[0];
  kd[0]   = bnd_cnd + kd[0];


  // use WKB trick for ground to ground propagation.
  if ((fabs(sourceheight)<1.0e-3) && (fabs(receiverheight)<1.0e-3) && (!turnoff_WKB)) {
      //
      // WKB trick for ground to ground propagation. 
      // Cut off lower phasespeed (highest wavenumber) where tunneling 
      // is insignificant (freq. dependent)
      //
      k_max_full = omega/ceffmin;
      k_gnd      = omega/ceff_grnd;
      dkk        = (pow(k_max_full,2)-pow(k_gnd,2))/100.0;

      //
      // dv: when ceffmin is at the ground dkk can be very small but non-zero (rounding errors?)
      // in that case we need to skip the next for loop; otherwise it will be executed
      // with the small increment dkk
      //
      kk = pow(k_gnd,2); //initialization of kk in case the loop is skipped

      if (dkk >1e-10) { // do this loop only if dkk is nonzero (significantly)
		      for (kk = pow(k_gnd,2); kk < pow(k_max_full,2); kk=kk+dkk) {
              i           = 0;
              wkbIntegral = 0.0;
              wkbTerm     = 1.0; 
              z_km = z_min_km; 
              while (wkbTerm > dkk) {
                  k_eff = omega/ceffz[i];
                  wkbTerm = abs(kk - pow(k_eff,2));
                  wkbIntegral = wkbIntegral + dz*sqrt(wkbTerm); // dz should be in meters
                  i++;
                  z_km += dz_km;
              } 
              if (wkbIntegral >= 10.0) {
                  printf("\n WKB fix: new phasevelocity minimum: %6.2f m/s (was %6.2f m/s); \n WKBIntegral= %12.7f at z = %6.2f km\n", omega/sqrt(kk), omega/k_max_full, wkbIntegral, z_km);
                  break;
              }
          }
      }
      
      *k_max = sqrt(kk);   // use this for ground-to-ground 1D Tloss 
											     //(uses WKB trick to include only non-vanishing modes at the ground)
	    // *k_max = k_max_full;
  }
  else { // not ground-to-ground propagation
      *k_max = omega/ceffmin; // same as k_max_full
  } 

  top     = nz - ((int) nz/10);
  z_km    = z_min_km + (top+1)*dz_km;  
  cz      = p->get( "_C0_", Hgt[ top+1 ] );
  windz   = p->get( "_WC_", Hgt[ top+1 ] );
  cefftop = p->get( "_CE_", Hgt[ top+1 ] );
  //cz      = sqrt(gamma*Pr[top+1]/rho[top+1]); // in m/s
  //windz   = zw[top+1]*sin(azi_rad) + mw[top+1]*cos(azi_rad); 
  //cefftop = cz + windz;
  *k_min  = omega/cefftop;
  
  delete [] ceffz;

  return 0;
}
