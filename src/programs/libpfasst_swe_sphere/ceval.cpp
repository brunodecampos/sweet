#include <iomanip>
#include <math.h>
#include <string>

#include <benchmarks_sphere/SphereBenchmarksCombined.hpp>
#include <sweet/SimulationVariables.hpp>
#include <sweet/sphere/SphereData.hpp>
#include <sweet/sphere/SphereOperators.hpp>

#include "SWE_Sphere_TS_l_irk.hpp"
#include "SWE_Sphere_TS_l_erk_n_erk.hpp"
#include "SWE_Sphere_TS_l_erk.hpp"

#include "ceval.hpp"
#include "cencap.hpp"

/**
 * Write file to data and return string of file name
 */
std::string write_file(
		       SphereDataCtx  &i_ctx,
		       const SphereData &i_sphereData,
		       const char* i_name	///< name of output variable
		       )
{
  char buffer[1024];
  
  // get the pointer to the Simulation Variables object
  SimulationVariables* simVars = i_ctx.get_simulation_variables();

  // Write the data into the file
  const char* filename_template = simVars->misc.output_file_name_prefix.c_str();
  sprintf(buffer, 
	  filename_template, 
	  i_name, 
	  simVars->timecontrol.current_timestep_size);
  i_sphereData.physical_file_write(buffer);
  
  return buffer;
}

extern "C"
{
  // initialization of the variables (initial condition)
  void cinitial(
		SphereDataCtx *i_ctx, 
		double i_t,
		double i_dt,
		SphereDataVars *o_Y
		) 
  {
    SphereData& phi_Y  = o_Y->get_phi();
    SphereData& vort_Y = o_Y->get_vort();
    SphereData& div_Y  = o_Y->get_div();

    // set the time stepping params
    i_ctx->setup_time_steps(
			    i_t,
			    i_dt
			    );

    // get the SimulationVariables object from context
    SimulationVariables* simVars(i_ctx->get_simulation_variables());
    
    // initialize the variables
    phi_Y.physical_set_zero();
    vort_Y.physical_set_zero();
    div_Y.physical_set_zero();

    // get the configuration for this level
    SphereDataConfig* data_config = i_ctx->get_sphere_data_config(o_Y->get_level());

    // instantiate h, u, and v to get the initial condition
    SphereData h_Y(data_config);
    SphereData u_Y(data_config);
    SphereData v_Y(data_config);

    // get the operator for this level
    SphereOperators* op = i_ctx->get_sphere_operators(o_Y->get_level());

    // get the initial condition in h, u, and v
    SphereBenchmarksCombined::setupInitialConditions(h_Y, 
						     u_Y, 
						     v_Y, 
						     *simVars, 
						     *op);
    
    // convert u, v, and h to phi, vort and div
    phi_Y = h_Y*simVars->sim.gravitation;
    op->robert_uv_to_vortdiv(u_Y.getSphereDataPhysical(), 
			     v_Y.getSphereDataPhysical(), 
			     vort_Y, 
			     div_Y);
    
    // output the configuration
    simVars->outputConfig();

    double current_simulation_time = 0;
    int nsteps = 0;
    /*    
    
    // get the timestepper 
    SWE_Sphere_TS_l_erk_n_erk* timestepper = i_ctx->get_l_erk_n_erk_timestepper(o_Y->get_level());
    
    write_file(*i_ctx, phi_Y,  "prog_phi_init");
    write_file(*i_ctx, vort_Y, "prog_vort_init");
    write_file(*i_ctx, div_Y,  "prog_div_init");

    std::cout << "current_simulation_time = " << current_simulation_time 
	      << " i_t = " << i_t 
	      << std::endl;
    
    i_dt /= 10;
    std::cout << "i_dt = " << i_dt << std::endl;

    // compute the reference solution (i.e., obtained with the reference time stepper)
    while (current_simulation_time < i_t)
      {
	if (nsteps%100 == 0)
	  std::cout << "current_simulation_time = "
		    << current_simulation_time
		    << std::endl;

	// solve the implicit system using the Helmholtz solver
	timestepper->run_timestep(
				  phi_Y,
				  vort_Y,
				  div_Y,
				  i_dt,
				  simVars->timecontrol.max_simulation_time
				  );
	
	current_simulation_time += i_dt;
	nsteps += 1;
      }

    write_file(*i_ctx, phi_Y,  "prog_phi_ref");
    write_file(*i_ctx, vort_Y, "prog_vort_ref");
    write_file(*i_ctx, div_Y,  "prog_div_ref");

    FatalError("stop the simulation");
    */
  }

  // finalizes the time step when libpfasst is done 
  // currently does nothing else than outputting the solution
  void cfinal(
	      SphereDataCtx *i_ctx, 
	      SphereDataVars *i_Y,
	      int i_nnodes, 
	      int i_niters
	      ) 
  {
    const SphereData& phi_Y  = i_Y->get_phi();
    const SphereData& vort_Y = i_Y->get_vort();
    const SphereData& div_Y  = i_Y->get_div();

    // get the SimulationVariables object from context
    SimulationVariables* simVars(i_ctx->get_simulation_variables()); 
    
    std::string filename = "prog_phi_nnodes_"+std::to_string(i_nnodes)+"_niters_"+std::to_string(i_niters);
    write_file(*i_ctx, phi_Y, filename.c_str());
    
    filename = "prog_vort_nnodes_"+std::to_string(i_nnodes)+"_niters_"+std::to_string(i_niters);
    write_file(*i_ctx, vort_Y, filename.c_str());

    filename = "prog_div_nnodes_"+std::to_string(i_nnodes)+"_niters_"+std::to_string(i_niters);
    write_file(*i_ctx, div_Y, filename.c_str());
    
  }

  // evaluates the explicit (nonlinear) piece
  void ceval_f1(SphereDataVars *i_Y,
		double i_t,
		SphereDataCtx *i_ctx,
		SphereDataVars *o_F1
		)
  {       
    const SphereData& phi_Y  = i_Y->get_phi();
    const SphereData& vort_Y = i_Y->get_vort();
    const SphereData& div_Y  = i_Y->get_div();

    SphereData& phi_F1  = o_F1->get_phi();
    SphereData& vort_F1 = o_F1->get_vort();
    SphereData& div_F1  = o_F1->get_div();

    // get the time step parameters
    SimulationVariables* simVars = i_ctx->get_simulation_variables();

    /*
    // return immediately if no nonlinear terms
    if (simVars->pde.use_nonlinear_equations == 0)
      {
	phi_F1.physical_set_zero();
	vort_F1.physical_set_zero();
	div_F1.physical_set_zero();

	return;
      }
    */
 
    SWE_Sphere_TS_l_erk_n_erk* timestepper = i_ctx->get_l_erk_n_erk_timestepper(i_Y->get_level());
		  
    // compute the explicit nonlinear right-hand side
    timestepper->euler_timestep_update(
				       phi_Y, 
				       vort_Y,
				       div_Y,
				       phi_F1,
				       vort_F1,
				       div_F1, 
				       simVars->timecontrol.max_simulation_time
				       );
    
  }

  // evaluates the first implicit piece o_F2 = F2(i_Y)
  void ceval_f2 (
		 SphereDataVars *i_Y, 
		 double i_t, 
		 SphereDataCtx *i_ctx, 
		 SphereDataVars *o_F2
		 ) 
  {
    const SphereData& phi_Y  = i_Y->get_phi();
    const SphereData& vort_Y = i_Y->get_vort();
    const SphereData& div_Y  = i_Y->get_div();

    SphereData& phi_F2  = o_F2->get_phi();
    SphereData& vort_F2 = o_F2->get_vort();
    SphereData& div_F2  = o_F2->get_div();

    phi_F2.physical_set_zero();
    vort_F2.physical_set_zero();
    div_F2.physical_set_zero();

    // get the simulation variables
    SimulationVariables* simVars = i_ctx->get_simulation_variables();
    
    /*
    // get the explicit timestepper 
    SWE_Sphere_TS_l_erk_n_erk* timestepper = i_ctx->get_l_erk_n_erk_timestepper(i_Y->get_level());

    // compute the linear right-hand side
    timestepper->euler_timestep_update_linear(
					      phi_Y, 
					      vort_Y,
					      div_Y,
					      phi_F2,
					      vort_F2,
					      div_F2,
					      simVars->timecontrol.max_simulation_time
					      );
    */
  }

  // solves the first implicit system for io_Y
  // then updates o_F2 with the new value of F2(io_Y)
  void ccomp_f2 (
		 SphereDataVars *io_Y, 
		 double i_t, 
		 double i_dt, 
		 SphereDataVars *i_Rhs, 
		 SphereDataCtx *i_ctx, 
		 SphereDataVars *o_F2
		 ) 
  {
    SphereData& phi_Y  = io_Y->get_phi();
    SphereData& vort_Y = io_Y->get_vort();
    SphereData& div_Y  = io_Y->get_div();

    const SphereData& phi_Rhs  = i_Rhs->get_phi();
    const SphereData& vort_Rhs = i_Rhs->get_vort();
    const SphereData& div_Rhs  = i_Rhs->get_div();

    // get the simulation variables
    SimulationVariables* simVars = i_ctx->get_simulation_variables();

    // first copy the rhs into the solution vector
    // this is needed to call the SWEET function run_ts_timestep
    phi_Y  = phi_Rhs;
    vort_Y = vort_Rhs;
    div_Y  = div_Rhs;
    
    SWE_Sphere_TS_l_irk* timestepper = i_ctx->get_l_irk_timestepper(io_Y->get_level());
    /*
    // solve the implicit system using the Helmholtz solver
    timestepper->run_timestep(
			      phi_Y,
			      vort_Y,
			      div_Y,
			      i_dt,
			      simVars->timecontrol.max_simulation_time
			      );
    */
    // now recompute F2 with the new value of Y
    ceval_f2(
	     io_Y, 
	     i_t, 
	     i_ctx, 
	     o_F2
	     );
  }

  // evaluates the second implicit piece o_F3 = F3(i_Y)
  // currently contains the implicit artificial viscosity term
  void ceval_f3 (
		 SphereDataVars *i_Y, 
		 double i_t, 
		 int i_level,
		 SphereDataCtx *i_ctx, 
		 SphereDataVars *o_F3
		 ) 
  {
    const SphereData& phi_Y  = i_Y->get_phi();
    const SphereData& vort_Y = i_Y->get_vort();
    const SphereData& div_Y  = i_Y->get_div();

    SphereData& phi_F3  = o_F3->get_phi();
    SphereData& vort_F3 = o_F3->get_vort();
    SphereData& div_F3  = o_F3->get_div();

    // initialize F3 to zero in case no artificial viscosity
    c_sweet_data_setval(o_F3, 0.0);

    // // get the simulation variables
    // SimulationVariables* simVars = i_ctx->get_simulation_variables();

    // // no need to do anything if no artificial viscosity
    // if (simVars->sim.viscosity == 0)
    //   return;

    // // get the operators for this level
    // SphereOperators* op = i_ctx->get_sphere_operators(i_level);
    
    // // Get diffusion coefficients
    // // put visc of the order of the sdc scheme
    // SphereData diff = simVars->sim.viscosity * op->diffusion_coefficient(simVars->sim.viscosity_order);
    
    // // apply to data
    // phi_F3  = diff(phi_Y);
    // vort_F3 = diff(vort_Y);
    // div_F3  = diff(div_Y);
  }

  // solves the second implicit system for io_Y
  // then updates o_F3 with the new value o_F3 = F3(io_Y)
  // currently solve the implicit system formed with artificial viscosity term
  void ccomp_f3 (
		 SphereDataVars *io_Y, 
		 double i_t, 
		 double i_dt,
		 int i_level,
		 SphereDataVars *i_Rhs, 
		 SphereDataCtx *i_ctx, 
		 SphereDataVars *o_F3
		 ) 
  {
    SphereData& phi_Y  = io_Y->get_phi();
    SphereData& vort_Y = io_Y->get_vort();
    SphereData& div_Y  = io_Y->get_div();

    const SphereData& phi_Rhs  = i_Rhs->get_phi();
    const SphereData& vort_Rhs = i_Rhs->get_vort();
    const SphereData& div_Rhs  = i_Rhs->get_div();

    // initialize F3 to zero in case no artificial viscosity
    c_sweet_data_setval(o_F3, 0.0);
            
    // // get the simulation variables
    // SimulationVariables* simVars = i_ctx->get_simulation_variables();

    // // no need to do anything if no artificial viscosity
    // if (simVars->sim.viscosity == 0)
    //   return;

    // // get the operators for this level
    // SphereOperators* op = i_ctx->get_sphere_operators(i_level);

    // // solve the implicit system
    // phi_Y = op->implicit_diffusion(phi_Rhs, 
    // 				   i_dt*simVars->sim.viscosity, 
    // 				   simVars->sim.viscosity_order );
    // vort_Y = op->implicit_diffusion(vort_Rhs, 
    // 				    i_dt*simVars->sim.viscosity, 
    // 				    simVars->sim.viscosity_order );
    // div_Y = op->implicit_diffusion(div_Rhs, 
    // 				   i_dt*simVars->sim.viscosity, 
    // 				   simVars->sim.viscosity_order );

    // // now recompute F3 with the new value of Y
    // ceval_f3(
    // 	     io_Y, 
    // 	     i_t, 
    // 	     i_level,
    // 	     i_ctx, 
    // 	     o_F3
    // 	     );
    
  }

}
