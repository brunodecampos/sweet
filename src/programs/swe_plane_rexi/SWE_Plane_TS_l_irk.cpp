/*
 * SWE_Plane_TS_l_irk.cpp
 *
 *  Created on: 29 May 2017
 *      Author: Martin Schreiber <M.Schreiber@exeter.ac.uk>
 */

#include "SWE_Plane_TS_l_irk.hpp"

#include <sweet/plane/PlaneDataComplex.hpp>
#include <sweet/plane/Convert_PlaneData_to_PlaneDataComplex.hpp>
#include <sweet/plane/Convert_PlaneDataComplex_to_PlaneData.hpp>




/**
 * Solve SWE with implicit time stepping
 *
 * U_t = L U(0)
 *
 * (U(tau) - U(0)) / tau = L U(tau)
 *
 * <=> U(tau) - U(0) = L U(tau) tau
 *
 * <=> U(tau) - L tau U(tau) = U(0)
 *
 * <=> (1 - L tau) U(tau) = U(0)
 *
 * <=> (1/tau - L) U(tau) = U(0)/tau
 */
void SWE_Plane_TS_l_irk::run_timestep(
		PlaneData &io_h,	///< prognostic variables
		PlaneData &io_u,	///< prognostic variables
		PlaneData &io_v,	///< prognostic variables

		double i_dt,		///< if this value is not equal to 0, use this time step size instead of computing one
		double i_simulation_timestamp
)
{
	if (i_dt <= 0)
		FatalError("OSWE_Plane_TS_l_irk: nly constant time step size allowed");

#if SWEET_USE_PLANE_SPECTRAL_SPACE

	PlaneData &eta0 = io_h;
	PlaneData &u0 = io_u;
	PlaneData &v0 = io_v;

	double alpha = 1.0/i_dt;

	eta0 *= alpha;
	u0 *= alpha;
	v0 *= alpha;

	// load kappa (k)
	double kappa = alpha*alpha + simVars.sim.f0*simVars.sim.f0;

	double eta_bar = simVars.sim.h0;
	double g = simVars.sim.gravitation;

	PlaneData rhs =
			(kappa/alpha) * eta0
			- eta_bar*(op.diff_c_x(u0) + op.diff_c_y(v0))
			- (simVars.sim.f0*eta_bar/alpha) * (op.diff_c_x(v0) - op.diff_c_y(u0))
		;

	PlaneData lhs = (-g*eta_bar*(op.diff2_c_x + op.diff2_c_y)).spectral_addScalarAll(kappa);
	io_h = rhs.spectral_div_element_wise(lhs);

	PlaneData uh = u0 - g*op.diff_c_x(io_h);
	PlaneData vh = v0 - g*op.diff_c_y(io_h);

	io_u = alpha/kappa * uh     + simVars.sim.f0/kappa * vh;
	io_v =    -simVars.sim.f0/kappa * uh + alpha/kappa * vh;

#else

	PlaneDataComplex eta0 = Convert_PlaneData_To_PlaneDataComplex::physical_convert(io_h);
	PlaneDataComplex u0 = Convert_PlaneData_To_PlaneDataComplex::physical_convert(io_u);
	PlaneDataComplex v0 = Convert_PlaneData_To_PlaneDataComplex::physical_convert(io_v);

	double alpha = 1.0/i_dt;

	eta0 *= alpha;
	u0 *= alpha;
	v0 *= alpha;

	// load kappa (k)
	double kappa = alpha*alpha + simVars.sim.f0*simVars.sim.f0;

	double eta_bar = simVars.sim.h0;
	double g = simVars.sim.gravitation;

	PlaneDataComplex rhs =
			(kappa/alpha) * eta0
			- eta_bar*(opComplex.diff_c_x(u0) + opComplex.diff_c_y(v0))
			- (simVars.sim.f0*eta_bar/alpha) * (opComplex.diff_c_x(v0) - opComplex.diff_c_y(u0))
		;

	PlaneDataComplex lhs = (-g*eta_bar*(opComplex.diff2_c_x + opComplex.diff2_c_y)).spectral_addScalarAll(kappa);
	PlaneDataComplex eta = rhs.spectral_div_element_wise(lhs);

	PlaneDataComplex uh = u0 - g*opComplex.diff_c_x(eta);
	PlaneDataComplex vh = v0 - g*opComplex.diff_c_y(eta);

	PlaneDataComplex u1 = alpha/kappa * uh     + simVars.sim.f0/kappa * vh;
	PlaneDataComplex v1 =    -simVars.sim.f0/kappa * uh + alpha/kappa * vh;

	io_h = Convert_PlaneDataComplex_To_PlaneData::physical_convert(eta);
	io_u = Convert_PlaneDataComplex_To_PlaneData::physical_convert(u1);
	io_v = Convert_PlaneDataComplex_To_PlaneData::physical_convert(v1);
#endif
}



/*
 * Setup
 */
void SWE_Plane_TS_l_irk::setup(
		int i_order	///< order of RK time stepping method
)
{
	timestepping_order = i_order;

	if (timestepping_order != 1)
		FatalError("SWE_Plane_TS_l_irk: Only 1st order IRK is supported");
}



SWE_Plane_TS_l_irk::SWE_Plane_TS_l_irk(
		SimulationVariables &i_simVars,
		PlaneOperators &i_op
)	:
		simVars(i_simVars),
		op(i_op)
#if !SWEET_USE_PLANE_SPECTRAL_SPACE
		,
		opComplex(i_op.planeDataConfig, i_simVars.sim.domain_size)
#endif
{
	setup(1);
}



SWE_Plane_TS_l_irk::~SWE_Plane_TS_l_irk()
{
}

